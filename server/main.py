# ============================================================================
# FastAPI 메인 서버 파일
# 역할: API 초기화, 라우터 등록, CORS 설정, 카메라 처리 시작/종료
# ============================================================================

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager
from app.hand_router import router as hand_router
from app.camera_processor import CameraProcessor

# 전역 카메라 처리기
camera_processor: CameraProcessor = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    FastAPI 앱의 시작/종료 이벤트를 처리합니다.
    - 시작: 카메라 처리 시작
    - 종료: 카메라 처리 중지
    """
    global camera_processor
    
    # 시작 이벤트
    camera_processor = CameraProcessor(camera_id=0)
    camera_processor.start()
    
    yield
    
    # 종료 이벤트
    if camera_processor:
        camera_processor.stop()


# FastAPI 인스턴스 생성
app = FastAPI(
    title="Sandwich Trainer API",
    description="MediaPipe Hands를 이용한 손 제스처 인식 서버",
    version="1.0.0",
    lifespan=lifespan
)

# ============================================================================
# CORS 설정
# 클라이언트에서 다른 도메인의 서버에 요청할 수 있도록 허용
# ============================================================================
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # 모든 도메인 허용 (프로덕션에서는 제한 권장)
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# ============================================================================
# 라우터 등록
# ============================================================================
app.include_router(hand_router, tags=["Hand Tracking"])

# ============================================================================
# 루트 엔드포인트
# ============================================================================
@app.get("/")
async def root():
    """
    API 정보 반환
    클라이언트가 서버 상태를 확인할 때 사용
    """
    return {
        "message": "Sandwich Trainer API is running",
        "version": "1.0.0",
        "description": "MediaPipe 기반 손 인식 서버 (서버에서 카메라 처리)",
        "endpoints": {
            "health": "GET /health",
            "hand_data": "GET /data (최신 손 정보 JSON)",
            "websocket_stream": "ws://localhost:8888/ws/hand/stream (손 정보 스트리밍)"
        },
        "clients": {
            "rest_client": "python client/client_rest.py",
            "websocket_client": "python client/client_websocket.py"
        }
    }


# ============================================================================
# 실행 방법
# uvicorn main:app --reload --port 8888
# 또는: python main.py
# ============================================================================

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8888)
