# ============================================================================
# 서버 클라이언트 (WebSocket 스트리밍 방식)
# 역할: WebSocket을 통해 서버에서 지속적으로 손 정보 JSON 수신
# ============================================================================

import asyncio
import json
import logging
import websockets

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

SERVER_URL = "ws://127.0.0.1:8888"
ENDPOINT_STREAM = f"{SERVER_URL}/ws/hand"


async def client_websocket_stream():
    """
    WebSocket을 통해 서버에서 손 정보를 지속적으로 수신합니다.
    
    사용법:
        python -m asyncio
        >>> from client_websocket import client_websocket_stream
        >>> await client_websocket_stream()
    
    또는:
        python client_websocket.py
    """
    logger.info(f"서버 주소: {SERVER_URL}")
    logger.info(f"엔드포인트: {ENDPOINT_STREAM}")
    logger.info("WebSocket 연결 시도...")
    
    frame_count = 0
    
    try:
        async with websockets.connect(ENDPOINT_STREAM) as websocket:
            logger.info("✓ WebSocket 연결됨")
            
            while True:
                try:
                    # 서버에서 JSON 데이터 수신
                    message = await websocket.recv()
                    data = json.loads(message)
                    frame_count += 1
                    
                    if data.get("ok"):
                        hand_detected = data.get("hand_detected", False)
                        gesture = data.get("gesture", "none")
                        action = data.get("action", "idle")
                        
                        if frame_count % 10 == 0:
                            logger.info(
                                f"Frame #{frame_count}: "
                                f"hand={hand_detected}, "
                                f"gesture={gesture}, "
                                f"action={action}"
                            )
                    else:
                        logger.warning(f"서버 에러: {data.get('message')}")
                
                except json.JSONDecodeError as e:
                    logger.error(f"JSON 파싱 실패: {e}")
                except Exception as e:
                    logger.error(f"데이터 수신 중 에러: {e}")
                    break
    
    except websockets.exceptions.WebSocketException as e:
        logger.error(f"WebSocket 에러: {e}")
    except KeyboardInterrupt:
        logger.info("\n프로그램 종료")
    except Exception as e:
        logger.error(f"예상치 못한 에러: {e}")


if __name__ == "__main__":
    asyncio.run(client_websocket_stream())
