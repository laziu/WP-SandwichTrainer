# ============================================================================
# 서버 클라이언트 (REST API 방식)
# 역할: 서버에서 send받은 손 정보 JSON을 주기적으로 조회
# ============================================================================

import requests
import json
import logging
import time

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

SERVER_URL = "http://127.0.0.1:8888"
ENDPOINT_GET_DATA = f"{SERVER_URL}/data"
POLL_INTERVAL = 0.033  # 약 30fps


def client_simple_polling():
    """
    REST API를 통해 서버에서 손 정보를 주기적으로 조회합니다.
    
    사용법:
        python client_rest.py
    """
    logger.info(f"서버 주소: {SERVER_URL}")
    logger.info(f"엔드포인트: {ENDPOINT_GET_DATA}")
    logger.info("손 정보 조회 시작...")
    
    frame_count = 0
    
    try:
        while True:
            try:
                # 서버에서 최신 손 정보 조회
                response = requests.get(ENDPOINT_GET_DATA, timeout=5)
                
                if response.status_code == 200:
                    data = response.json()
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
                else:
                    logger.error(f"HTTP 에러: {response.status_code}")
                
            except requests.RequestException as e:
                logger.error(f"요청 실패: {e}")
            
            time.sleep(POLL_INTERVAL)
    
    except KeyboardInterrupt:
        logger.info("\n프로그램 종료")
    except Exception as e:
        logger.error(f"예상치 못한 에러: {e}")


if __name__ == "__main__":
    client_simple_polling()
