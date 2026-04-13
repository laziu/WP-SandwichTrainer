from fastapi import FastAPI
from app.hand_router import router as hand_router

app = FastAPI(title="Sandwich Trainer API")

app.include_router(hand_router)