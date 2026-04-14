from pydantic import BaseModel


class FeedbackResponse(BaseModel):
    type: str = "feedback"
    session_id: str
    feedback_text: str
    tips: list[str]


class ErrorResponse(BaseModel):
    type: str = "error"
    session_id: str | None = None
    code: str
    message: str
