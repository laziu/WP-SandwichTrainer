from enum import Enum
from typing import Literal

from pydantic import BaseModel


class ErrorType(str, Enum):
    WRONG_ORDER = "wrong_order"
    MISSING_INGREDIENT = "missing_ingredient"
    WRONG_INGREDIENT = "wrong_ingredient"
    TIME_EXCEEDED = "time_exceeded"


class SandwichError(BaseModel):
    type: ErrorType
    detail: str


class SandwichResult(BaseModel):
    type: Literal["sandwich_result"] = "sandwich_result"
    session_id: str
    recipe_id: str
    score: int
    time_seconds: int
    ingredients_used: list[str]
    ingredients_order: list[str]
    errors: list[SandwichError]
