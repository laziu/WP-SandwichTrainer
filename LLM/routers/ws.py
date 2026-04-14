import logging

from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from pydantic import ValidationError

from LLM.schemas.request import SandwichResult
from LLM.schemas.response import ErrorResponse
from LLM.services.feedback import generate_feedback

logger = logging.getLogger(__name__)
router = APIRouter()


@router.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    logger.info("WebSocket 연결됨")
    try:
        while True:
            data = await websocket.receive_json()
            session_id = data.get("session_id") if isinstance(data, dict) else None
            msg_type = data.get("type") if isinstance(data, dict) else None

            if msg_type != "sandwich_result":
                err = ErrorResponse(
                    session_id=session_id,
                    code="unsupported_type",
                    message=f"지원하지 않는 메시지 타입: {msg_type!r}",
                )
                await websocket.send_json(err.model_dump())
                continue

            try:
                result = SandwichResult(**data)
            except ValidationError as e:
                err = ErrorResponse(
                    session_id=session_id,
                    code="invalid_payload",
                    message=str(e),
                )
                await websocket.send_json(err.model_dump())
                continue

            try:
                feedback = await generate_feedback(result)
                await websocket.send_json(feedback.model_dump())
            except Exception as e:
                logger.exception("피드백 생성 실패")
                err = ErrorResponse(
                    session_id=result.session_id,
                    code="internal_error",
                    message=str(e),
                )
                await websocket.send_json(err.model_dump())
    except WebSocketDisconnect:
        logger.info("WebSocket 연결 해제")
