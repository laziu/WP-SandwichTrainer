from pydantic import BaseModel


class FeedbackResponse(BaseModel):
    type: str = "feedback"
    session_id: str
    feedback_text: str
    tips: list[str]
