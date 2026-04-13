# ============================================================================
# 카메라 및 MediaPipe 처리 모듈
# 역할: 별도 스레드에서 카메라 프레임 읽기, MediaPipe 손 인식, 결과 저장
# ============================================================================

import cv2
import mediapipe as mp
import logging
import math
import time
from typing import Dict, Any, Optional, Tuple
from threading import Thread, Lock
import numpy as np

logger = logging.getLogger(__name__)

# MediaPipe 설정
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

# 랜드마크 인덱스
THUMB_TIP = 4      # 엄지 끝
INDEX_TIP = 8      # 검지 끝
MIDDLE_TIP = 12    # 중지 끝
RING_TIP = 16      # 약지 끝
PINKY_TIP = 20     # 소지 끝
WRIST = 0          # 손목

# 임계값
PINCH_DISTANCE_THRESHOLD = 0.05


class CameraProcessor:
    """
    카메라 프레임을 읽고 MediaPipe로 손을 인식하는 클래스
    별도 스레드에서 실행되며, 최신 손 정보를 저장합니다.
    """
    
    def __init__(self, camera_id: int = 0):
        """
        CameraProcessor 초기화
        
        Args:
            camera_id: 사용할 카메라 ID (기본값: 0 = 웹캠)
        """
        self.camera_id = camera_id
        self.cap = None
        self.hands = None
        self.is_running = False
        self.thread = None
        
        # 최신 손 정보 저장 (스레드 안전성을 위해 Lock 사용)
        self.lock = Lock()
        self.latest_payload: Optional[Dict[str, Any]] = None
        self.frame_count = 0
        
    def start(self):
        """카메라 처리 시작"""
        if self.is_running:
            logger.warning("CameraProcessor 이미 실행 중입니다.")
            return
        
        try:
            # 카메라 초기화
            self.cap = cv2.VideoCapture(self.camera_id)
            if not self.cap.isOpened():
                logger.error(f"카메라 {self.camera_id} 열기 실패")
                return
            
            # 카메라 설정
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
            self.cap.set(cv2.CAP_PROP_FPS, 30)
            
            # MediaPipe 초기화
            self.hands = mp_hands.Hands(
                static_image_mode=False,
                max_num_hands=1,
                min_detection_confidence=0.7,
                min_tracking_confidence=0.5
            )
            
            self.is_running = True
            
            # 별도 스레드에서 실행
            self.thread = Thread(target=self._process_frames, daemon=True)
            self.thread.start()
            
            logger.info("✓ CameraProcessor 시작됨")
            
        except Exception as e:
            logger.error(f"CameraProcessor 시작 실패: {e}")
            self.is_running = False
    
    def stop(self):
        """카메라 처리 중지"""
        self.is_running = False
        
        if self.thread:
            self.thread.join(timeout=5)
        
        if self.cap:
            self.cap.release()
        
        if self.hands:
            self.hands.close()
        
        logger.info("✓ CameraProcessor 중지됨")
    
    def _process_frames(self):
        """
        별도 스레드에서 카메라 프레임을 계속 처리합니다.
        """
        logger.info("프레임 처리 시작")
        
        while self.is_running:
            try:
                ret, frame = self.cap.read()
                if not ret:
                    logger.error("프레임 읽기 실패")
                    break
                
                # BGR을 RGB로 변환 (MediaPipe는 RGB 사용)
                rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                
                # MediaPipe 처리
                result = self.hands.process(rgb_frame)
                
                # 페이로드 추출
                frame_time = time.time()
                payload = self._extract_payload(result, frame_time)
                
                # 스레드 안전하게 저장
                with self.lock:
                    self.latest_payload = payload
                    self.frame_count += 1
                
                # 시각화 (선택사항 - 불필요하면 제거 가능)
                # annotated_frame = self._draw_frame_info(frame, result, payload)
                
            except Exception as e:
                logger.error(f"프레임 처리 중 에러: {e}")
        
        logger.info("프레임 처리 종료")
    
    def get_latest_payload(self) -> Optional[Dict[str, Any]]:
        """
        최신 손 정보를 반환합니다.
        
        Returns:
            dict: 손 정보 페이로드 또는 None
        """
        with self.lock:
            return self.latest_payload
    
    def _extract_payload(self, result, frame_time: float) -> Dict[str, Any]:
        """
        MediaPipe 결과를 JSON 페이로드로 변환합니다.
        
        Args:
            result: MediaPipe Hands 처리 결과
            frame_time: 프레임 타임스탐프
        
        Returns:
            dict: 손 정보 페이로드
        """
        # 손이 감지되지 않은 경우
        if not result.multi_hand_landmarks:
            return {
                "hand_detected": False,
                "gesture": "none",
                "index_tip": None,
                "thumb_tip": None,
                "landmarks": None,
                "timestamp": frame_time,
            }
        
        # 첫 번째 손만 사용 (한 손 추적)
        hand = result.multi_hand_landmarks[0]
        
        # 제스처 판정
        gesture = self._detect_gesture(hand)
        
        # 엄지 끝과 검지 끝의 좌표
        index_tip = hand.landmark[INDEX_TIP]
        thumb_tip = hand.landmark[THUMB_TIP]
        
        # 모든 랜드마크 좌표 (21개 점)
        landmarks = [
            {"x": lm.x, "y": lm.y}
            for lm in hand.landmark
        ]
        
        # 페이로드 구성
        payload = {
            "hand_detected": True,
            "gesture": gesture,
            "index_tip": {"x": index_tip.x, "y": index_tip.y},
            "thumb_tip": {"x": thumb_tip.x, "y": thumb_tip.y},
            "landmarks": landmarks,
            "timestamp": frame_time,
        }
        
        return payload
    
    def _detect_gesture(self, hand_landmarks) -> str:
        """
        손의 랜드마크를 기반으로 제스처를 판정합니다.
        
        Args:
            hand_landmarks: MediaPipe 손 랜드마크
        
        Returns:
            str: "pinch" 또는 "open"
        """
        thumb_tip = hand_landmarks.landmark[THUMB_TIP]
        index_tip = hand_landmarks.landmark[INDEX_TIP]
        
        # 거리 계산
        distance = self._calculate_distance(
            (thumb_tip.x, thumb_tip.y),
            (index_tip.x, index_tip.y)
        )
        
        # 거리에 따라 제스처 판정
        if distance < PINCH_DISTANCE_THRESHOLD:
            return "pinch"
        else:
            return "open"
    
    @staticmethod
    def _calculate_distance(point1: Tuple[float, float], point2: Tuple[float, float]) -> float:
        """두 점 사이의 거리 계산"""
        dx = point1[0] - point2[0]
        dy = point1[1] - point2[1]
        return math.sqrt(dx * dx + dy * dy)
    
    def _draw_frame_info(self, frame, result, payload: Dict[str, Any]):
        """프레임에 손 정보 시각화 (선택사항)"""
        if not payload["hand_detected"]:
            return frame
        
        h, w, c = frame.shape
        
        if result.multi_hand_landmarks:
            hand = result.multi_hand_landmarks[0]
            mp_draw.draw_landmarks(
                frame,
                hand,
                mp_hands.HAND_CONNECTIONS,
                mp_draw.DrawingSpec(color=(0, 255, 0), thickness=2, circle_radius=2),
                mp_draw.DrawingSpec(color=(255, 0, 0), thickness=1)
            )
        
        cv2.putText(
            frame,
            f"Gesture: {payload['gesture']}",
            (50, 50),
            cv2.FONT_HERSHEY_SIMPLEX,
            1.2,
            (0, 255, 0),
            2
        )
        
        return frame
