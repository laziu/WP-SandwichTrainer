from typing import List, Optional, Literal
from pydantic import BaseModel, Field


class Point2D(BaseModel):
    x: float = Field(..., ge=0.0, le=1.0)
    y: float = Field(..., ge=0.0, le=1.0)


class HandPayload(BaseModel):
    hand_detected: bool
    gesture: Literal["open", "pinch", "grab", "none"] = "none"
    index_tip: Optional[Point2D] = None
    thumb_tip: Optional[Point2D] = None
    landmarks: Optional[List[Point2D]] = None
    timestamp: float


class ServerResponse(BaseModel):
    ok: bool
    message: str
    action: Literal["idle", "hover", "grab_start", "grab_hold", "drop"] = "idle"