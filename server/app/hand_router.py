from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from app.schemas import HandPayload

router = APIRouter()


def decide_action(data: HandPayload) -> str:
    """
    최소 판정 로직.
    지금은 gesture만 보고 action을 정한다.
    나중에 좌표/영역 판정까지 붙이면 된다.
    """
    if not data.hand_detected:
        return "idle"

    if data.gesture == "pinch":
        return "grab_hold"

    if data.gesture == "open":
        return "hover"

    if data.gesture == "grab":
        return "grab_start"

    return "idle"


@router.get("/health")
async def health_check():
    return {"ok": True}


@router.websocket("/ws/hand")
async def websocket_hand(websocket: WebSocket):
    await websocket.accept()
    print("WebSocket 연결됨: /ws/hand")

    try:
        while True:
            raw = await websocket.receive_json()
            data = HandPayload(**raw)

            action = decide_action(data)

            await websocket.send_json(
                {
                    "ok": True,
                    "message": "hand payload received",
                    "action": action,
                }
            )

    except WebSocketDisconnect:
        print("클라이언트 연결 종료")
    except Exception as e:
        print(f"WebSocket 에러: {e}")
        await websocket.send_json(
            {
                "ok": False,
                "message": str(e),
                "action": "idle",
            }
        )
    finally:
        await websocket.close()
        print("WebSocket 연결 종료")