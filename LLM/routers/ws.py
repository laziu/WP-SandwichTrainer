import json
import logging

from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from schemas.request import SandwichResult
from services.feedback import generate_feedback

logger = logging.getLogger(__name__)
router = APIRouter()


@router.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    logger.info("WebSocket 연결됨")
    try:
        while True:
            data = await websocket.receive_json()
            try:
                result = SandwichResult(**data)
                feedback = await generate_feedback(result)
                await websocket.send_json(feedback.model_dump())
            except Exception as e:
                logger.error(f"피드백 생성 실패: {e}")
                await websocket.send_json(
                    {"type": "error", "message": str(e)}
                )
    except WebSocketDisconnect:
        logger.info("WebSocket 연결 해제")
