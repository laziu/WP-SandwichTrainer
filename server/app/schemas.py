# ============================================================================
# Pydantic 데이터 스키마 파일
# 역할: 클라이언트에서 받은 JSON을 검증하고, 서버 응답 형식 정의
# ============================================================================

from typing import List, Optional, Literal, Dict, Any
from pydantic import BaseModel, Field


# ============================================================================
# 2D 좌표 정보
# ============================================================================
class Point2D(BaseModel):
    """
    2D 평면상의 한 점 (x, y 좌표)
    
    Attributes:
        x: 가로 좌표 (0.0 ~ 1.0, 정규화됨)
        y: 세로 좌표 (0.0 ~ 1.0, 정규화됨)
    """
    x: float = Field(..., ge=0.0, le=1.0, description="가로 좌표 (0~1)")
    y: float = Field(..., ge=0.0, le=1.0, description="세로 좌표 (0~1)")


# ============================================================================
# 클라이언트에서 서버로 보내는 손 정보
# ============================================================================
class HandPayload(BaseModel):
    """
    MediaPipe에서 인식한 손 정보를 클라이언트가 서버로 전송하는 JSON 형식
    
    Attributes:
        hand_detected: 손이 감지되었는지 여부
        gesture: 손 제스처 ("open"|"pinch"|"grab"|"none")
        index_tip: 검지 끝의 좌표
        thumb_tip: 엄지 끝의 좌표
        landmarks: 손의 모든 랜드마크 좌표 (21개)
        timestamp: 데이터 생성 시간 (Unix 타임스탐프)
    """
    hand_detected: bool = Field(..., description="손 감지 여부")
    
    gesture: Literal["open", "pinch", "grab", "none"] = Field(
        default="none", 
        description="손 제스처 종류"
    )
    
    index_tip: Optional[Point2D] = Field(
        default=None, 
        description="검지 끝 좌표"
    )
    
    thumb_tip: Optional[Point2D] = Field(
        default=None, 
        description="엄지 끝 좌표"
    )
    
    landmarks: Optional[List[Point2D]] = Field(
        default=None, 
        description="손의 모든 랜드마크 좌표 (21개 점)"
    )
    
    timestamp: float = Field(..., description="데이터 생성 시간")


# ============================================================================
# 액션별 상세 정보 (Unreal에서 사용)
# ============================================================================
class ActionData(BaseModel):
    """
    각 액션에 따른 추가 정보
    
    Attributes:
        hand_position: 손의 중심 위치
        hand_confidence: 손 감지 신뢰도 (0.0 ~ 1.0)
        gesture: 현재 제스처
        action_duration: 액션이 유지된 시간 (초)
    """
    hand_position: Optional[Point2D] = Field(
        default=None,
        description="손의 중심 위치 (정규화됨)"
    )
    
    hand_confidence: float = Field(
        default=0.0,
        ge=0.0,
        le=1.0,
        description="손 감지 신뢰도"
    )
    
    gesture: Literal["open", "pinch", "grab", "none"] = Field(
        default="none",
        description="현재 제스처"
    )
    
    is_left_hand: bool = Field(
        default=False,
        description="왼손 여부"
    )


# ============================================================================
# 서버에서 Unreal 클라이언트로 보내는 응답 (개선된 버전)
# ============================================================================
class UnrealClientResponse(BaseModel):
    """
    Unreal Engine이 쉽게 해석할 수 있는 응답 형식
    
    Attributes:
        ok: 요청 처리 성공 여부
        hand_detected: 손 감지 여부
        action: 수행할 액션
            - idle: 대기 상태 (손 미감지)
            - hover: 마우스 호버 상태 (손 열린 상태)
            - grab_start: 집기 시작 (손가락 모음 시작)
            - grab_hold: 집기 유지 (손가락 모음 상태 유지)
            - drop: 놓기 (손가락 폈을 때)
        action_data: 액션별 상세 정보
        gesture: 현재 제스처
        index_position: 검지 끝 위치
        thumb_position: 엄지 끝 위치
        timestamp: 데이터 타임스탐프
    """
    ok: bool = Field(..., description="처리 성공 여부")
    
    hand_detected: bool = Field(..., description="손 감지 여부")
    
    action: Literal["idle", "hover", "grab_start", "grab_hold", "drop"] = Field(
        default="idle",
        description="클라이언트가 실행할 액션"
    )
    
    action_data: ActionData = Field(
        default_factory=ActionData,
        description="액션별 상세 정보"
    )
    
    gesture: Literal["open", "pinch", "grab", "none"] = Field(
        default="none",
        description="손 제스처"
    )
    
    index_position: Optional[Point2D] = Field(
        default=None,
        description="검지 끝 위치"
    )
    
    thumb_position: Optional[Point2D] = Field(
        default=None,
        description="엄지 끝 위치"
    )
    
    timestamp: float = Field(..., description="데이터 생성 시간 (Unix)")
    
    message: Optional[str] = Field(
        default=None,
        description="추가 메시지 (에러 등)"
    )


# ============================================================================
# 서버에서 클라이언트로 보내는 응답 (기존 버전 - 호환성)
# ============================================================================
class ServerResponse(BaseModel):
    """
    서버가 손 정보를 받고 반환하는 응답 형식
    
    Attributes:
        ok: 요청 처리 성공 여부
        message: 상태 메시지
        action: 수행할 액션
    """
    ok: bool = Field(..., description="처리 성공 여부")
    message: str = Field(..., description="상태 메시지")
    action: Literal["idle", "hover", "grip", "grab_start", "grab_hold", "drop"] = Field(
        default="idle", 
        description="수행할 액션"
    )
