from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from LLM import config
from LLM.routers.ws import router as ws_router

app = FastAPI(title="Sandwich Trainer LLM Feedback", version="0.1.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(ws_router)


@app.get("/")
async def root():
    return {"service": "llm-feedback", "status": "ok", "ws": "/ws"}


if __name__ == "__main__":
    import uvicorn

    uvicorn.run("LLM.main:app", host=config.SERVER_HOST, port=config.SERVER_PORT)
