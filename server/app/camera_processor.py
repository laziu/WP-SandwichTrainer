# ============================================================================
# 카메라 처리기
# 역할: 서버가 카메라를 직접 읽고 MediaPipe를 처리한 뒤,
#       최소 JSON(x, y, gesture)만 최신 상태로 저장
# ============================================================================

from __future__ import annotations

import logging
import math
import os
import threading
import time
from pathlib import Path
from typing import Any

import cv2

try:
    import mediapipe as mp
    from mediapipe.tasks import python as mp_python
    from mediapipe.tasks.python import vision
except Exception:
    mp = None
    mp_python = None
    vision = None


logger = logging.getLogger(__name__)

# grab 판정을 위한 랜드마크 인덱스
THUMB_TIP = 4
INDEX_TIP = 8

# 손바닥 중심(palm center) 계산에 사용할 랜드마크 인덱스
PALM_CENTER_INDEXES = (0, 5, 9, 13, 17)

# 엄지 끝(4)과 검지 끝(8) 사이 2D 거리 임계값
GRAB_THRESHOLD = 0.05


class CameraProcessor:
    """서버 내부 단일 카메라 파이프라인.

    - 카메라는 이 클래스에서만 1회 오픈
    - 최신 결과는 최소 JSON으로 유지
    - 필요 시 로컬 OpenCV preview 창 표시
    """

    def __init__(self, camera_id: int = 0, show_preview: bool = False):
        self.camera_id = camera_id
        self.show_preview = show_preview

        self.cap: cv2.VideoCapture | None = None
        self.thread: threading.Thread | None = None
        self.running = False
        self.lock = threading.Lock()

        self.hand_landmarker: Any | None = None
        self.mp_init_error: str | None = None

        # 항상 동일한 최소 JSON 형태 유지
        self.latest_result: dict[str, float | str | None] = {
            "x": None,
            "y": None,
            "gesture": "none",
        }

        self._init_hand_landmarker()

    def start(self) -> bool:
        """카메라 처리 스레드를 시작합니다. 실패 시 False를 반환합니다."""
        if self.running:
            return True

        # Windows 호환성에서 CAP_DSHOW가 안정적인 경우가 많아 우선 사용
        self.cap = cv2.VideoCapture(self.camera_id, cv2.CAP_DSHOW)

        if not self.cap.isOpened():
            logger.error("카메라를 열 수 없습니다. camera_id=%s", self.camera_id)
            self.cap.release()
            self.cap = None
            return False

        self.running = True
        self.thread = threading.Thread(target=self._process_loop, daemon=True)
        self.thread.start()
        logger.info("[CameraProcessor] started (camera_id=%s, preview=%s)", self.camera_id, self.show_preview)
        return True

    def stop(self) -> None:
        """카메라 처리 스레드를 종료합니다."""
        self.running = False

        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=2.0)

        if self.cap:
            self.cap.release()
            self.cap = None

        if self.show_preview:
            try:
                cv2.destroyWindow("Server Camera Preview")
            except Exception:
                pass

        if self.hand_landmarker and hasattr(self.hand_landmarker, "close"):
            self.hand_landmarker.close()

        logger.info("[CameraProcessor] stopped")

    def get_latest_result(self) -> dict[str, float | str | None]:
        """최신 최소 JSON 결과를 안전하게 복사해 반환합니다."""
        with self.lock:
            return dict(self.latest_result)

    def _process_loop(self) -> None:
        """카메라 프레임을 반복 처리하고 최신 결과를 갱신합니다."""
        while self.running:
            if self.cap is None:
                self._set_no_hand()
                time.sleep(0.03)
                continue

            if self.hand_landmarker is None:
                # 모델이 없거나 초기화 실패해도 서버는 계속 살아있게 유지
                self._set_no_hand()
                time.sleep(0.10)
                continue

            ok, frame = self.cap.read()
            if not ok or frame is None:
                logger.warning("카메라 프레임 읽기 실패")
                self._set_no_hand()
                time.sleep(0.03)
                continue

            try:
                frame = cv2.flip(frame, 1)
                frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=frame_rgb)
                detection_result = self.hand_landmarker.detect(mp_image)

                parsed = self._build_minimal_result(detection_result)
                with self.lock:
                    self.latest_result = parsed

                if self.show_preview:
                    self._draw_preview(frame, parsed)

            except Exception as e:
                logger.exception("프레임 처리 중 에러: %s", e)
                self._set_no_hand()

            # 카메라/추론 루프 목표: 약 30fps
            time.sleep(0.03)

    def _build_minimal_result(self, detection_result: Any) -> dict[str, float | str | None]:
        """MediaPipe 결과를 최소 JSON으로 변환합니다.

        반환 형식:
        {
            "x": float | None,
            "y": float | None,
            "gesture": "grab" | "none"
        }
        """
        hand_list = getattr(detection_result, "hand_landmarks", None)
        if not hand_list:
            return {"x": None, "y": None, "gesture": "none"}

        hand = hand_list[0]
        if len(hand) <= INDEX_TIP:
            return {"x": None, "y": None, "gesture": "none"}

        # 손바닥 중심 = (0,5,9,13,17)의 x,y 평균
        sum_x = 0.0
        sum_y = 0.0
        for idx in PALM_CENTER_INDEXES:
            lm = hand[idx]
            sum_x += float(lm.x)
            sum_y += float(lm.y)

        center_x = sum_x / len(PALM_CENTER_INDEXES)
        center_y = sum_y / len(PALM_CENTER_INDEXES)

        # grab 판정: 엄지 끝(4) - 검지 끝(8) 거리
        thumb = hand[THUMB_TIP]
        index = hand[INDEX_TIP]
        dist = math.hypot(float(thumb.x) - float(index.x), float(thumb.y) - float(index.y))
        gesture = "grab" if dist < GRAB_THRESHOLD else "none"

        return {
            "x": round(center_x, 4),
            "y": round(center_y, 4),
            "gesture": gesture,
        }

    def _draw_preview(self, frame: Any, result: dict[str, float | str | None]) -> None:
        """서버 로컬 미리보기 창을 표시합니다. (원격 전송 없음)"""
        x = result.get("x")
        y = result.get("y")
        gesture = str(result.get("gesture", "none"))

        if isinstance(x, float) and isinstance(y, float):
            h, w = frame.shape[:2]
            cx = int(x * w)
            cy = int(y * h)
            cv2.circle(frame, (cx, cy), 8, (0, 255, 0), -1)

        cv2.putText(
            frame,
            f"Gesture: {gesture}",
            (20, 40),
            cv2.FONT_HERSHEY_SIMPLEX,
            1.0,
            (0, 255, 0) if gesture == "grab" else (200, 200, 200),
            2,
        )

        cv2.imshow("Server Camera Preview", frame)
        cv2.waitKey(1)

    def _init_hand_landmarker(self) -> None:
        """MediaPipe HandLandmarker를 초기화합니다."""
        if mp is None or mp_python is None or vision is None:
            self.mp_init_error = "mediapipe import 실패. requirements 설치 상태를 확인하세요."
            logger.error(self.mp_init_error)
            return

        model_path = self._resolve_model_path()
        if model_path is None:
            self.mp_init_error = (
                "hand_landmarker.task 파일을 찾을 수 없습니다. "
                "client/models 또는 환경변수 MEDIAPIPE_HAND_MODEL 경로를 확인하세요."
            )
            logger.error(self.mp_init_error)
            return

        options = vision.HandLandmarkerOptions(
            base_options=mp_python.BaseOptions(model_asset_path=str(model_path)),
            num_hands=1,
            min_hand_detection_confidence=0.7,
            min_hand_presence_confidence=0.5,
            min_tracking_confidence=0.5,
        )

        try:
            self.hand_landmarker = vision.HandLandmarker.create_from_options(options)
            logger.info("MediaPipe HandLandmarker 초기화 완료: %s", model_path)
        except Exception as e:
            self.mp_init_error = f"HandLandmarker 초기화 실패: {e}"
            logger.error(self.mp_init_error)

    def _resolve_model_path(self) -> Path | None:
        """가능한 경로에서 hand_landmarker.task를 찾습니다."""
        model_env = os.getenv("MEDIAPIPE_HAND_MODEL")
        if model_env:
            env_path = Path(model_env)
            if env_path.is_file():
                return env_path

        root = Path(__file__).resolve().parents[2]
        candidates = [
            root / "models" / "hand_landmarker.task",
            root / "client" / "models" / "hand_landmarker.task",
            root / "server" / "models" / "hand_landmarker.task",
        ]

        for path in candidates:
            if path.is_file():
                return path

        return None

    def _set_no_hand(self) -> None:
        """손 미검출 기본값으로 최신 결과를 갱신합니다."""
        with self.lock:
            self.latest_result = {
                "x": None,
                "y": None,
                "gesture": "none",
            }