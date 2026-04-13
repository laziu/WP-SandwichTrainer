# ============================================================================
# FastAPI 메인 서버 파일
# 역할: API 초기화, 라우터 등록, CORS 설정, 카메라 처리 시작/종료
# ============================================================================

from contextlib import asynccontextmanager
import logging
import os

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from app.camera_processor import CameraProcessor
from app.hand_router import router as hand_router, set_camera_processor

logger = logging.getLogger(__name__)

# 전역 카메라 처리기
camera_processor: CameraProcessor | None = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    FastAPI 앱의 시작/종료 이벤트를 처리합니다.
    - 시작: 카메라 처리 시작
    - 종료: 카메라 처리 중지
    """
    global camera_processor

    show_preview = os.getenv("SHOW_PREVIEW", "1") in {"1", "true", "True", "yes", "on"}
    camera_processor = CameraProcessor(camera_id=0, show_preview=show_preview)
    started = camera_processor.start()

    if not started:
        logger.error("CameraProcessor 시작 실패. 서버는 실행하지만 hand 결과는 none으로 유지됩니다.")

    # 라우터 쪽에서 같은 인스턴스를 참조할 수 있게 연결
    set_camera_processor(camera_processor)

    try:
        yield
    finally:
        if camera_processor:
            camera_processor.stop()


app = FastAPI(
    title="Sandwich Trainer API",
    description="MediaPipe Hands를 이용한 손 제스처 인식 서버",
    version="1.0.0",
    lifespan=lifespan,
)

# ============================================================================
# CORS 설정
# ============================================================================
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # 개발용. 배포 시 제한 권장
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# ============================================================================
# 라우터 등록
# ============================================================================
app.include_router(hand_router, tags=["Hand Tracking"])


@app.get("/")
async def root():
    return {
        "message": "Sandwich Trainer API is running",
        "ws_endpoint": "/ws/mediapipe",
    }


# ============================================================================
# 실행 방법
# uvicorn main:app --reload --port 8888
# ============================================================================
if __name__ == "__main__":
    import uvicorn

    uvicorn.run("main:app", host="0.0.0.0", port=8888, reload=True)