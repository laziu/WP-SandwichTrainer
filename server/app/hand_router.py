# ============================================================================
# 손 인식 라우터
# 역할: 클라이언트가 접속하면 CameraProcessor의 최신 JSON 결과를 계속 전송
# ============================================================================

from __future__ import annotations

import asyncio
import logging

from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from app.camera_processor import CameraProcessor

router = APIRouter()
logger = logging.getLogger(__name__)

camera_processor: CameraProcessor | None = None


def set_camera_processor(processor: CameraProcessor) -> None:
    global camera_processor
    camera_processor = processor


def _to_legacy_hand_payload(result: dict) -> dict:
    """
    구형 클라이언트(/ws/hand)와의 호환을 위한 페이로드 변환.
    """
    x = result.get("x")
    y = result.get("y")
    gesture = result.get("gesture", "none")

    return {
        "ok": True,
        "hand_detected": x is not None and y is not None and gesture != "none",
        "gesture": gesture,
        "action": "idle",
        "x": x,
        "y": y,
    }


@router.get("/hand/latest")
async def get_latest_hand_result():
    """
    디버깅용 HTTP 엔드포인트.
    웹소켓 없이도 현재 최신 손 인식 결과를 확인할 수 있습니다.
    """
    if camera_processor is None:
        return {"x": None, "y": None, "gesture": "none"}

    return camera_processor.get_latest_result()


@router.websocket("/ws/mediapipe")
async def websocket_mediapipe(websocket: WebSocket):
    """
    서버가 카메라와 MediaPipe를 직접 돌리고,
    클라이언트는 손 인식 결과 JSON만 받는 WebSocket 엔드포인트.
    """
    await websocket.accept()
    logger.info("[WebSocket] client connected: /ws/mediapipe")

    try:
        while True:
            if camera_processor is None:
                await websocket.send_json({
                    "x": None,
                    "y": None,
                    "gesture": "none",
                })
                await asyncio.sleep(0.5)
                continue

            result = camera_processor.get_latest_result()
            await websocket.send_json(result)

            # 약 12.5fps 전송 제한 (10~15fps 목표)
            await asyncio.sleep(0.08)

    except WebSocketDisconnect:
        logger.info("[WebSocket] client disconnected")
    except Exception as e:
        logger.exception("[WebSocket] error: %s", e)
    finally:
        try:
            await websocket.close()
        except Exception:
            pass
        logger.info("[WebSocket] closed")


@router.websocket("/ws/hand")
async def websocket_hand_legacy(websocket: WebSocket):
    """
    구형 클라이언트 호환용 WebSocket 엔드포인트.
    기존 /ws/hand 형식(ok, hand_detected, gesture, action)을 전송합니다.
    """
    await websocket.accept()
    logger.info("[WebSocket] client connected: /ws/hand")

    try:
        while True:
            if camera_processor is None:
                await websocket.send_json({
                    "ok": False,
                    "message": "camera processor not ready",
                    "hand_detected": False,
                    "gesture": "none",
                    "action": "idle",
                    "x": None,
                    "y": None,
                })
                await asyncio.sleep(0.5)
                continue

            result = camera_processor.get_latest_result()
            await websocket.send_json(_to_legacy_hand_payload(result))

            # 약 12.5fps 전송 제한 (10~15fps 목표)
            await asyncio.sleep(0.08)

    except WebSocketDisconnect:
        logger.info("[WebSocket] client disconnected")
    except Exception as e:
        logger.exception("[WebSocket] error: %s", e)
    finally:
        try:
            await websocket.close()
        except Exception:
            pass
        logger.info("[WebSocket] closed")