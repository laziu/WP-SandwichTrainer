import os
from pathlib import Path

from dotenv import load_dotenv

load_dotenv(Path(__file__).parent / ".env")

OPENAI_API_KEY: str = os.environ["OPENAI_API_KEY"]
LLM_MODEL: str = os.getenv("LLM_MODEL", "gpt-5-mini")
LLM_TEMPERATURE: float = float(os.getenv("LLM_TEMPERATURE", "0.8"))
SERVER_HOST: str = os.getenv("SERVER_HOST", "0.0.0.0")
SERVER_PORT: int = int(os.getenv("SERVER_PORT", "8000"))
