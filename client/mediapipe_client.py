# ============================================================================
# MediaPipe 클라이언트
# 역할: 웹캠에서 손 인식, 제스처 판정, 서버에 WebSocket으로 전송
# ============================================================================

import asyncio
import json
import logging
import math
import time
from typing import Optional, Dict, Any, List, Tuple

import cv2
import mediapipe as mp
import websockets
from websockets.exceptions import WebSocketException

# ============================================================================
# 로깅 설정
# ============================================================================
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)

# ============================================================================
# 상수 정의
# ============================================================================
WS_URL = "ws://127.0.0.1:8888/ws/hand"  # 서버 WebSocket 주소
PINCH_DISTANCE_THRESHOLD = 0.05  # 엄지-검지 거리 임계값
WINDOW_WIDTH = 1280
WINDOW_HEIGHT = 720
FPS_TARGET = 30

# MediaPipe 핸즈 모델 및 드로우 유틸 초기화
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

# 랜드마크 인덱스 정의 (MediaPipe Hands는 21개 점 반환)
THUMB_TIP = 4      # 엄지 끝
INDEX_TIP = 8      # 검지 끝
MIDDLE_TIP = 12    # 중지 끝
RING_TIP = 16      # 약지 끝
PINKY_TIP = 20     # 소지 끝
WRIST = 0          # 손목


# ============================================================================
# 함수: 거리 계산
# ============================================================================
def calculate_distance(
    point1: Tuple[float, float], 
    point2: Tuple[float, float]
) -> float:
    """
    두 점 사이의 유클리드 거리를 계산합니다.
    
    Args:
        point1: (x, y) 튜플
        point2: (x, y) 튜플
    
    Returns:
        float: 두 점 사이의 거리
        
    예:
        distance = calculate_distance((0, 0), (3, 4))
        # 리턴: 5.0
    """
    dx = point1[0] - point2[0]
    dy = point1[1] - point2[1]
    distance = math.sqrt(dx * dx + dy * dy)
    return distance


# ============================================================================
# 함수: 제스처 판정
# ============================================================================
def detect_gesture(hand_landmarks) -> str:
    """
    손의 랜드마크(21개 점)를 기반으로 제스처를 판정합니다.
    
    판정 로직:
    - 엄지와 검지의 거리가 매우 가까우면 "pinch" (손가락 모음)
    - 그 외에는 "open" (손가락 펼침)
    
    Args:
        hand_landmarks: MediaPipe가 반환한 손의 랜드마크
    
    Returns:
        str: "pinch" 또는 "open"
        
    참고:
        - 실제 제스처 인식은 더 복잡할 수 있습니다.
        - 예: 손가락 개수 세기, 손 방향 확인 등
    """
    
    # 엄지 끝과 검지 끝의 좌표 추출
    thumb_tip = hand_landmarks.landmark[THUMB_TIP]
    index_tip = hand_landmarks.landmark[INDEX_TIP]
    
    # 두 점의 거리 계산
    distance = calculate_distance(
        (thumb_tip.x, thumb_tip.y),
        (index_tip.x, index_tip.y)
    )
    
    # 거리에 따라 제스처 판정
    if distance < PINCH_DISTANCE_THRESHOLD:
        return "pinch"  # 손가락 모음
    else:
        return "open"   # 손가락 펼침


# ============================================================================
# 함수: 페이로드 추출
# ============================================================================
def extract_payload(result, frame_time: float) -> Dict[str, Any]:
    """
    MediaPipe 결과를 JSON 페이로드로 변환합니다.
    
    Args:
        result: MediaPipe Hands의 처리 결과
        frame_time: 프레임 처리 시간 (Unix 타임스탐프)
    
    Returns:
        dict: 서버로 전송할 JSON 페이로드
        
    예반응:
        {
            "hand_detected": False,
            "gesture": "none",
            "index_tip": None,
            "thumb_tip": None,
            "landmarks": None,
            "timestamp": 1234567890.123
        }
    """
    
    # 1. 손이 감지되지 않은 경우
    if not result.multi_hand_landmarks:
        return {
            "hand_detected": False,
            "gesture": "none",
            "index_tip": None,
            "thumb_tip": None,
            "landmarks": None,
            "timestamp": frame_time,
        }
    
    # 2. 손이 감지된 경우, 가장 첫 번째 손만 사용 (한 손 추적)
    hand = result.multi_hand_landmarks[0]
    
    # 3. 제스처 판정
    gesture = detect_gesture(hand)
    
    # 4. 엄지 끝과 검지 끝의 좌표 추출
    index_tip = hand.landmark[INDEX_TIP]
    thumb_tip = hand.landmark[THUMB_TIP]
    
    # 5. 모든 랜드마크 좌표 추출 (21개 점)
    landmarks = [
        {"x": lm.x, "y": lm.y}
        for lm in hand.landmark
    ]
    
    # 6. 페이로드 구성
    payload = {
        "hand_detected": True,
        "gesture": gesture,
        "index_tip": {"x": index_tip.x, "y": index_tip.y},
        "thumb_tip": {"x": thumb_tip.x, "y": thumb_tip.y},
        "landmarks": landmarks,
        "timestamp": frame_time,
    }
    
    return payload


# ============================================================================
# 함수: 프레임에 정보 그리기 (시각화)
# ============================================================================
def draw_frame_info(
    frame: cv2.Mat,
    result,
    payload: Dict[str, Any],
    server_response: Optional[Dict[str, Any]] = None
) -> cv2.Mat:
    """
    원본 프레임에 손의 랜드마크, 거리, 제스처 등의 정보를 그립니다.
    
    Args:
        frame: OpenCV 프레임
        result: MediaPipe Hands 결과
        payload: 추출한 손 정보 페이로드
        server_response: 서버로부터 받은 응답 (액션)
    
    Returns:
        cv2.Mat: 정보가 그려진 프레임
    """
    
    # 프레임 높이, 너비 추출
    h, w, c = frame.shape
    
    # 1. 손이 감지되지 않은 경우
    if not payload["hand_detected"]:
        cv2.putText(
            frame,
            "No hand detected",
            (50, 50),
            cv2.FONT_HERSHEY_SIMPLEX,
            1.5,
            (0, 0, 255),  # 빨간색
            3
        )
        return frame
    
    # 2. 손의 랜드마크 그리기
    if result.multi_hand_landmarks:
        hand = result.multi_hand_landmarks[0]
        
        # 손의 연결선과 점 그리기
        mp_draw.draw_landmarks(
            frame,
            hand,
            mp_hands.HAND_CONNECTIONS,
            mp_draw.DrawingSpec(color=(0, 255, 0), thickness=2, circle_radius=2),  # 점 (초록색)
            mp_draw.DrawingSpec(color=(255, 0, 0), thickness=1)  # 연결선 (파란색)
        )
        
        # 3. 엄지-검지 거리 그리기
        if payload["thumb_tip"] and payload["index_tip"]:
            thumb = payload["thumb_tip"]
            index = payload["index_tip"]
            
            # 정규화된 좌표를 픽셀 좌표로 변환
            thumb_pos = (int(thumb["x"] * w), int(thumb["y"] * h))
            index_pos = (int(index["x"] * w), int(index["y"] * h))
            
            # 선 그리기
            cv2.line(
                frame,
                thumb_pos,
                index_pos,
                (255, 255, 0),  # 청록색
                3
            )
            
            # 거리 계산 및 표시
            distance = calculate_distance(
                (thumb["x"], thumb["y"]),
                (index["x"], index["y"])
            )
            
            mid_x = (thumb_pos[0] + index_pos[0]) // 2
            mid_y = (thumb_pos[1] + index_pos[1]) // 2
            
            cv2.putText(
                frame,
                f"Distance: {distance:.3f}",
                (mid_x, mid_y),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.6,
                (255, 255, 0),  # 청록색
                2
            )
    
    # 4. 상태 정보 표시
    gesture = payload["gesture"]
    gesture_color = (0, 255, 0) if gesture == "open" else (0, 165, 255)  # 초록 또는 주황
    
    cv2.putText(
        frame,
        f"Gesture: {gesture}",
        (50, 50),
        cv2.FONT_HERSHEY_SIMPLEX,
        1.2,
        gesture_color,
        2
    )
    
    # 5. 서버 응답 액션 표시
    if server_response and "action" in server_response:
        action = server_response["action"]
        
        # 액션별 색상 지정
        action_colors = {
            "idle": (200, 200, 200),      # 회색
            "hover": (0, 255, 0),          # 초록
            "grab_start": (0, 165, 255),   # 주황
            "grab_hold": (255, 0, 0),      # 파란색
            "drop": (255, 0, 255),         # 분홍색
            "grip": (255, 100, 0),         # 파란-녹색
        }
        
        color = action_colors.get(action, (255, 255, 255))
        
        cv2.putText(
            frame,
            f"Action: {action}",
            (50, 100),
            cv2.FONT_HERSHEY_SIMPLEX,
            1.2,
            color,
            2
        )
    
    return frame


# ============================================================================
# 비동기 함수: 웹소켓 처리
# ============================================================================
async def handle_websocket(payload: Dict[str, Any]) -> Optional[Dict[str, Any]]:
    """
    WebSocket을 통해 서버에 손 정보를 전송하고 응답을 받습니다.
    
    Args:
        payload: 클라이언트가 생성한 손 정보 페이로드
    
    Returns:
        dict: 서버의 응답 (액션 등), 또는 None (연결 실패 시)
    """
    
    try:
        # WebSocket 연결 (비동기)
        async with websockets.connect(WS_URL) as websocket:
            # 페이로드를 JSON으로 변환해 서버에 전송
            await websocket.send(json.dumps(payload))
            
            # 서버의 응답 수신
            response_json = await websocket.recv()
            response = json.loads(response_json)
            
            return response
    
    except WebSocketException as we:
        logger.error(f"WebSocket 에러: {str(we)}")
        return None
    
    except json.JSONDecodeError as je:
        logger.error(f"JSON 디코딩 에러: {str(je)}")
        return None
    
    except Exception as e:
        logger.error(f"예상치 못한 에러: {type(e).__name__}: {str(e)}")
        return None


# ============================================================================
# 메인 루프
# ============================================================================
async def main():
    """
    주요 실행 루프:
    1. 웹캠 열기
    2. MediaPipe Hands 초기화
    3. 프레임 읽기 및 처리
    4. 서버와 통신
    5. 결과 표시
    """
    
    logger.info("=" * 60)
    logger.info("MediaPipe Hands 클라이언트 시작")
    logger.info(f"서버 주소: {WS_URL}")
    logger.info("=" * 60)
    
    # 1. 웹캠 열기
    cap = cv2.VideoCapture(0)  # 0번 카메라 (기본 웹캠)
    
    if not cap.isOpened():
        logger.error("웹캠을 열 수 없습니다!")
        return
    
    # 웹캠 설정
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, WINDOW_WIDTH)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, WINDOW_HEIGHT)
    cap.set(cv2.CAP_PROP_FPS, FPS_TARGET)
    
    logger.info(f"웹캠 해상도: {int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))}x{int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))}")
    
    # 2. MediaPipe Hands 초기화
    with mp_hands.Hands(
        static_image_mode=False,        # 비디오 모드
        max_num_hands=1,                # 최대 1개 손 (성능 향상)
        min_detection_confidence=0.7,   # 감지 신뢰도 70% 이상
        min_tracking_confidence=0.5     # 추적 신뢰도 50% 이상
    ) as hands:
        
        logger.info("MediaPipe Hands 초기화 완료")
        logger.info("'q' 키를 눌러 종료하세요")
        logger.info("-" * 60)
        
        frame_count = 0
        server_response = None
        
        # 3. 무한 루프: 프레임 처리
        while True:
            # 프레임 읽기
            success, frame = cap.read()
            
            if not success:
                logger.error("프레임 읽기 실패")
                break
            
            frame_count += 1
            frame_time = time.time()
            
            # 프레임을 RGB로 변환 (MediaPipe는 RGB 사용)
            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            
            # 4. MediaPipe 처리
            result = hands.process(frame_rgb)
            
            # 5. 페이로드 추출
            payload = extract_payload(result, frame_time)
            
            # 6. 서버와 통신 (비동기)
            server_response = await handle_websocket(payload)
            
            # 7. 프레임에 정보 그리기
            frame = draw_frame_info(frame, result, payload, server_response)
            
            # FPS 표시
            if frame_count % 10 == 0:
                logger.info(f"프레임 #{frame_count} 처리 완료")
            
            # 8. 화면에 표시
            cv2.imshow("MediaPipe Hands", frame)
            
            # 'q' 키 입력 확인 (1ms 대기)
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                logger.info("사용자가 종료 요청")
                break
    
    # 정리
    cap.release()
    cv2.destroyAllWindows()
    logger.info("=" * 60)
    logger.info("클라이언트 종료")
    logger.info("=" * 60)


# ============================================================================
# 프로그램 진입점
# ============================================================================
if __name__ == "__main__":
    try:
        # 비동기 이벤트 루프 실행
        asyncio.run(main())
    
    except KeyboardInterrupt:
        logger.info("KeyboardInterrupt 감지 - 종료합니다")
    
    except Exception as e:
        logger.error(f"예상치 못한 에러: {type(e).__name__}: {str(e)}")


    landmarks = [{"x": lm.x, "y": lm.y} for lm in hand.landmark]

    return {
        "hand_detected": True,
        "gesture": gesture,
        "index_tip": {"x": index_tip.x, "y": index_tip.y},
        "thumb_tip": {"x": thumb_tip.x, "y": thumb_tip.y},
        "landmarks": landmarks,
        "timestamp": time.time(),
    }


async def run_client():
    cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        raise RuntimeError("웹캠을 열 수 없습니다.")

    async with websockets.connect(WS_URL) as ws:
        with mp_hands.Hands(
            static_image_mode=False,
            max_num_hands=1,
            min_detection_confidence=0.5,
            min_tracking_confidence=0.5,
        ) as hands:
            while True:
                ok, frame = cap.read()
                if not ok:
                    break

                frame = cv2.flip(frame, 1)
                rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                result = hands.process(rgb)

                payload = extract_payload(result)

                await ws.send(json.dumps(payload))
                response = json.loads(await ws.recv())

                # 화면 표시
                if result.multi_hand_landmarks:
                    for hand_landmarks in result.multi_hand_landmarks:
                        mp_draw.draw_landmarks(
                            frame,
                            hand_landmarks,
                            mp_hands.HAND_CONNECTIONS
                        )

                gesture_text = payload["gesture"]
                action_text = response.get("action", "idle")

                cv2.putText(
                    frame,
                    f"gesture={gesture_text} action={action_text}",
                    (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.8,
                    (0, 255, 0),
                    2,
                )

                cv2.imshow("MediaPipe Client", frame)

                key = cv2.waitKey(1) & 0xFF
                if key == 27:  # ESC
                    break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    asyncio.run(run_client())