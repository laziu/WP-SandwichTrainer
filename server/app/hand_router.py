# ============================================================================
# 손 추적 라우터
# 역할: WebSocket 연결 관리, 클라이언트 데이터 검증, 액션 판정 로직
# ============================================================================

import logging
import time
from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from app.schemas import HandPayload, UnrealClientResponse, ActionData, Point2D

# 로깅 설정 (디버깅용)
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

router = APIRouter()

# ============================================================================
# Unreal 응답 변환 함수
# ============================================================================
def create_unreal_response(payload: dict, action: str) -> UnrealClientResponse:
    """
    MediaPipe 페이로드를 Unreal 클라이언트용 응답으로 변환합니다.
    
    Args:
        payload: MediaPipe 카메라 프로세서에서 생성한 페이로드
        action: 판정된 액션
    
    Returns:
        UnrealClientResponse: Unreal이 파싱하기 쉬운 형식의 응답
    """
    hand_detected = payload.get("hand_detected", False)
    
    # 손의 중심 위치 계산 (여러 랜드마크의 평균)
    hand_position = None
    if hand_detected and payload.get("landmarks"):
        landmarks = payload.get("landmarks", [])
        if landmarks:
            avg_x = sum(lm.get("x", 0) for lm in landmarks) / len(landmarks)
            avg_y = sum(lm.get("y", 0) for lm in landmarks) / len(landmarks)
            hand_position = Point2D(x=avg_x, y=avg_y)
    
    # 액션 데이터 생성
    action_data = ActionData(
        hand_position=hand_position,
        hand_confidence=0.9 if hand_detected else 0.0,  # 실제로는 MediaPipe 신뢰도 사용
        gesture=payload.get("gesture", "none"),
        is_left_hand=False  # 현재 한 손만 추적
    )
    
    # 응답 생성
    return UnrealClientResponse(
        ok=True,
        hand_detected=hand_detected,
        action=action,
        action_data=action_data,
        gesture=payload.get("gesture", "none"),
        index_position=Point2D(**payload["index_tip"]) if payload.get("index_tip") else None,
        thumb_position=Point2D(**payload["thumb_tip"]) if payload.get("thumb_tip") else None,
        timestamp=payload.get("timestamp", time.time()),
        message=None
    )

# ============================================================================
# 상수 정의
# ============================================================================
# 손가락 거리 임계값: 엄지-검지 거리가 이 값 이하면 "pinch" 상태
PINCH_DISTANCE_THRESHOLD = 0.05

# 이전 제스처 추적 (상태 유지용)
last_gesture: str = "none"


# ============================================================================
# 액션 판정 로직
# ============================================================================
def decide_action(data: HandPayload) -> str:
    """
    클라이언트로부터 받은 손 정보를 기반으로 수행할 액션을 결정합니다.
    
    매우 단순한 로직입니다. 위험한 임무를 수행할 때는 더 복잡한 로직이 필요합니다.
    
    Args:
        data (HandPayload): 클라이언트에서 전송한 손 정보
    
    Returns:
        str: 수행할 액션
            - "idle": 손이 감지되지 않음
            - "hover": 손이 열린 상태
            - "grab_start": 손가락을 찔렀을 때
            - "grab_hold": 손가락을 찌른 상태 유지
            - "drop": 손가락을 폈을 때
    """
    
    # 손이 감지되지 않으면 대기 상태
    if not data.hand_detected:
        return "idle"
    
    # 제스처별 액션 결정
    if data.gesture == "pinch":
        # 손가락을 찔렀을 때 (핸드폰 스크린 터치와 유사)
        # 이전에 open이었다면 grab_start (새로 잡기 시작)
        # pinch가 유지되고 있다면 grab_hold (계속 잡는 중)
        global last_gesture
        if last_gesture != "pinch":
            last_gesture = "pinch"
            return "grab_start"
        else:
            last_gesture = "pinch"
            return "grab_hold"
    
    elif data.gesture == "open":
        # 손이 열린 상태
        # 이전에 pinch였다면 drop (손을 폈을 때)
        if last_gesture == "pinch":
            last_gesture = "open"
            return "drop"
        else:
            last_gesture = "open"
            return "hover"
    
    # 기타 제스처는 대기 상태
    else:
        last_gesture = data.gesture
        return "idle"


# ============================================================================
# 건강 상태 체크
# ============================================================================
@router.get("/health")
async def health_check():
    """
    서버가 정상적으로 작동하는지 확인하는 엔드포인트
    
    Returns:
        dict: {"ok": True}
    """
    logger.info("Health check 요청 받음")
    return {"ok": True, "message": "Server is running"}


# ============================================================================
# 최신 손 정보 조회 (클라이언트용)
# ============================================================================
@router.get("/data")
async def get_hand_data():
    """
    서버의 카메라에서 인식한 최신 손 정보를 JSON으로 반환합니다.
    
    클라이언트는 이 엔드포인트를 주기적으로 호출하여 손 정보를 받습니다.
    
    Returns:
        dict: 손 정보 (MediaPipe 결과) 또는 에러 메시지
        
    예시 응답 (손 감지됨):
        {
            "ok": true,
            "hand_detected": true,
            "gesture": "open",
            "index_tip": {"x": 0.5, "y": 0.4},
            "thumb_tip": {"x": 0.48, "y": 0.42},
            "landmarks": [...],
            "timestamp": 1234567890.123,
            "action": "hover"
        }
    
    예시 응답 (손 미감지):
        {
            "ok": true,
            "hand_detected": false,
            "gesture": "none",
            "action": "idle"
        }
    """
    try:
        # 메인 모듈에서 camera_processor 접근
        from . import main
        
        if main.camera_processor is None:
            return UnrealClientResponse(
                ok=False,
                hand_detected=False,
                action="idle",
                message="카메라 처리기가 초기화되지 않았습니다",
                timestamp=time.time()
            )
        
        # 최신 손 정보 조회
        payload = main.camera_processor.get_latest_payload()
        
        if payload is None:
            return UnrealClientResponse(
                ok=True,
                hand_detected=False,
                action="idle",
                timestamp=time.time()
            )
        
        # 액션 판정
        if payload["hand_detected"]:
            hand_data = HandPayload(**payload)
            action = decide_action(hand_data)
        else:
            action = "idle"
        
        # Unreal 클라이언트용 응답 생성
        response = create_unreal_response(payload, action)
        return response
    
    except Exception as e:
        logger.error(f"손 정보 조회 중 에러: {e}")
        return UnrealClientResponse(
            ok=False,
            hand_detected=False,
            action="idle",
            message=str(e),
            timestamp=time.time()
        )


# ============================================================================
# WebSocket 엔드포인트 (스트리밍용)
# ============================================================================
@router.websocket("/ws/hand/stream")
async def websocket_hand_stream(websocket: WebSocket):
    """
    WebSocket을 통해 서버의 카메라 데이터를 클라이언트로 스트리밍합니다.
    
    클라이언트가 연결하면 서버가 계속 손 정보를 JSON으로 전송합니다.
    
    플로우:
    1. 클라이언트 연결 수용
    2. 서버 카메라에서 최신 손 정보 조회
    3. JSON으로 클라이언트에게 전송
    4. 반복 (약 30fps)
    
    오류 처리:
    - WebSocketDisconnect: 클라이언트 연결 끊김
    - 기타 예외: 에러 응답 전송
    """
    await websocket.accept()
    logger.info("✓ WebSocket 스트림 연결됨: /ws/hand/stream")
    
    connection_id = id(websocket)
    
    try:
        from . import main
        
        frame_count = 0
        
        while True:
            import asyncio
            
            # 약 30fps로 데이터 전송
            await asyncio.sleep(0.033)
            
            if main.camera_processor is None:
                await websocket.send_json({
                    "ok": False,
                    "message": "카메라 처리기가 초기화되지 않았습니다"
                })
                continue
            
            try:
                # 최신 손 정보 조회
                payload = main.camera_processor.get_latest_payload()
                
                if payload is None:
                    continue
                
                # 액션 판정
                if payload["hand_detected"]:
                    hand_data = HandPayload(**payload)
                    action = decide_action(hand_data)
                else:
                    action = "idle"
                
                # Unreal 클라이언트용 응답 생성 및 전송
                response = create_unreal_response(payload, action)
                await websocket.send_json(response.model_dump())
                
                frame_count += 1
                
                if frame_count % 30 == 0:
                    logger.debug(
                        f"[{connection_id}] 스트림 프레임 #{frame_count}: "
                        f"gesture={payload.get('gesture')}, action={action}"
                    )
            
            except Exception as e:
                logger.error(f"스트림 처리 중 에러: {e}")
                error_response = UnrealClientResponse(
                    ok=False,
                    hand_detected=False,
                    action="idle",
                    message=f"에러: {str(e)}",
                    timestamp=time.time()
                )
                await websocket.send_json(error_response.model_dump())
    
    except WebSocketDisconnect:
        logger.info(f"✗ WebSocket 스트림 연결 끝남 (프레임: {frame_count})")
    
    except Exception as e:
        logger.error(f"예상치 못한 에러: {type(e).__name__}: {str(e)}")
        try:
            await websocket.send_json({
                "ok": False,
                "message": f"서버 에러: {str(e)}",
                "action": "idle"
            })
        except Exception as send_error:
            logger.error(f"에러 응답 전송 실패: {str(send_error)}")


# ============================================================================
# 테스트 엔드포인트 - 예시 JSON 데이터 반환
# ============================================================================
import asyncio


@router.get("/test/data/{action}")
async def test_hand_data(action: str = "idle"):
    """
    테스트용 더미 손 정보 반환
    
    클라이언트에서 테스트할 때 사용 (실제 카메라 불필요)
    
    사용법:
        GET /test/data/idle        - 손 미감지
        GET /test/data/hover       - 손 열린 상태
        GET /test/data/grab_start  - 집기 시작
        GET /test/data/grab_hold   - 집기 유지
        GET /test/data/drop        - 놓기
    
    예시:
        curl http://localhost:8888/test/data/grab_start
        
    Unreal에서:
        GetTestData(TEXT("grab_start"));
    """
    
    test_gestures = {
        "idle": "none",
        "hover": "open",
        "grab_start": "pinch",
        "grab_hold": "pinch",
        "drop": "open",
    }
    
    gesture = test_gestures.get(action, "none")
    hand_detected = action != "idle"
    
    test_payload = {
        "hand_detected": hand_detected,
        "gesture": gesture,
        "index_tip": {"x": 0.52, "y": 0.41} if hand_detected else None,
        "thumb_tip": {"x": 0.50, "y": 0.42} if hand_detected else None,
        "landmarks": [
            {"x": 0.5 + i * 0.01, "y": 0.4 + i * 0.01}
            for i in range(21)
        ] if hand_detected else None,
        "timestamp": time.time(),
    }
    
    response = create_unreal_response(test_payload, action)
    logger.info(f"✓ 테스트 데이터 반환: action={action}")
    
    return response


# ============================================================================
# 기존 WebSocket 엔드포인트 (클라이언트에서 손 정보를 받는 기존 방식)
# [더 이상 사용되지 않음 - 서버에서 카메라를 처리하도록 변경됨]
# ============================================================================
# @router.websocket("/ws/hand")
# async def websocket_hand(websocket: WebSocket):
#     """기존 엔드포인트 - 클라이언트에서 손 정보를 받고 액션을 반환하던 방식"""
#     pass
