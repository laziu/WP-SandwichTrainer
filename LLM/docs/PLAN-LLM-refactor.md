# PLAN: LLM 모듈 호환성 이슈 일괄 정비 (#1~#6)

## 목표

| # | 이슈 | 해결 방향 |
|---|---|---|
| 1 | 앱 진입점 부재 | `LLM/main.py` 신규, 독립 FastAPI 앱 |
| 2 | 포트 전략 미결정 | 팀 서버(8888)와 분리, LLM = `8000` |
| 3 | import 경로 문제 | `LLM.xxx` 패키지 절대경로로 통일 |
| 4 | 에러 응답에 session_id 누락 | `ErrorResponse` 스키마 도입 + ws 핸들러에서 raw data에서 session_id 보존 |
| 5 | 메시지 `type` 필드 검증 없음 | `type`에 따라 디스패치, 미지원 타입은 명시적 에러 |
| 6 | UE ↔ recipe 식별자 계약 미확인 | `LLM/docs/ue_contract.md` 신규, UE팀 공유용 계약 문서 |

## 전제

- UE는 두 WebSocket 연결 유지: `ws://host:8888/ws/hand/stream` (손추적), `ws://host:8000/ws` (피드백)
- LLM 서버 재시작이 손추적에 영향 없어야 함 → 프로세스 분리
- 본 작업은 **스펙 레벨 정비**까지. OpenAI 실호출 비용 발생하는 end-to-end 테스트는 선택적 수동 검증.

---

## 변경 상세

### [이슈 #1, #2] 신규: `LLM/main.py`

- FastAPI 앱, CORS, `ws.router` include, 루트 상태 엔드포인트, `__main__` 기동 블록
- 팀 `server/main.py`와 동일한 CORS 정책 (`allow_origins=["*"]`)
- 포트/호스트는 `config.SERVER_HOST/SERVER_PORT` 사용 (기본 `0.0.0.0:8000`)

```python
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from LLM import config
from LLM.routers.ws import router as ws_router

app = FastAPI(title="Sandwich Trainer LLM Feedback", version="0.1.0")
app.add_middleware(CORSMiddleware, allow_origins=["*"],
                   allow_credentials=True, allow_methods=["*"], allow_headers=["*"])
app.include_router(ws_router)

@app.get("/")
async def root():
    return {"service": "llm-feedback", "status": "ok", "ws": "/ws"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("LLM.main:app", host=config.SERVER_HOST, port=config.SERVER_PORT)
```

### [이슈 #3] 신규: `LLM/__init__.py`

- 빈 파일 (패키지 인식용)

### [이슈 #3] 수정: 내부 임포트 절대경로 통일

#### `LLM/routers/ws.py`
- `from schemas.request import SandwichResult` → `from LLM.schemas.request import SandwichResult`
- `from services.feedback import generate_feedback` → `from LLM.services.feedback import generate_feedback`
- (이슈 #4, #5로 본 파일 추가 수정. 아래 참조)

#### `LLM/services/feedback.py`
- `import config` → `from LLM import config`
- `from schemas.request import SandwichResult` → `from LLM.schemas.request import SandwichResult`
- `from schemas.response import FeedbackResponse` → `from LLM.schemas.response import FeedbackResponse`
- `from services.recipe import get_recipe` → `from LLM.services.recipe import get_recipe`

#### `LLM/services/recipe.py`, `LLM/config.py`
- 변경 없음

### [이슈 #4] 수정: `LLM/schemas/response.py`

`ErrorResponse` 추가:

```python
from pydantic import BaseModel

class FeedbackResponse(BaseModel):
    type: str = "feedback"
    session_id: str
    feedback_text: str
    tips: list[str]

class ErrorResponse(BaseModel):
    type: str = "error"
    session_id: str | None = None   # 파싱 실패 시 None 허용
    code: str                       # "invalid_payload" | "unsupported_type" | "internal_error"
    message: str
```

- `session_id: str | None` — 최초 JSON 파싱은 성공했지만 필드가 누락된 경우 등을 위해 Optional
- `code` 필드로 UE 측 분기 용이

### [이슈 #5] 수정: `LLM/schemas/request.py`

`type`을 `Literal`로 고정:

```python
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
```

- 다른 `type` 값이 들어오면 Pydantic이 거부 → ws 핸들러에서 `unsupported_type`으로 응답

### [이슈 #4, #5] 수정: `LLM/routers/ws.py` 전체 구조

```python
import logging
from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from pydantic import ValidationError

from LLM.schemas.request import SandwichResult
from LLM.schemas.response import ErrorResponse
from LLM.services.feedback import generate_feedback

logger = logging.getLogger(__name__)
router = APIRouter()


@router.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    logger.info("WebSocket 연결됨")
    try:
        while True:
            data = await websocket.receive_json()
            session_id = data.get("session_id") if isinstance(data, dict) else None
            msg_type = data.get("type") if isinstance(data, dict) else None

            # 이슈 #5: type 분기
            if msg_type != "sandwich_result":
                err = ErrorResponse(
                    session_id=session_id,
                    code="unsupported_type",
                    message=f"지원하지 않는 메시지 타입: {msg_type!r}",
                )
                await websocket.send_json(err.model_dump())
                continue

            # 이슈 #4: 파싱/생성 예외 시 session_id 보존
            try:
                result = SandwichResult(**data)
            except ValidationError as e:
                err = ErrorResponse(
                    session_id=session_id,
                    code="invalid_payload",
                    message=str(e),
                )
                await websocket.send_json(err.model_dump())
                continue

            try:
                feedback = await generate_feedback(result)
                await websocket.send_json(feedback.model_dump())
            except Exception as e:
                logger.exception("피드백 생성 실패")
                err = ErrorResponse(
                    session_id=result.session_id,
                    code="internal_error",
                    message=str(e),
                )
                await websocket.send_json(err.model_dump())
    except WebSocketDisconnect:
        logger.info("WebSocket 연결 해제")
```

### [이슈 #6] 신규: `LLM/docs/ue_contract.md`

UE팀과 공유할 계약 문서. 담을 내용:

1. **WebSocket 엔드포인트** — `ws://<host>:8000/ws`
2. **요청 메시지 스키마** (`sandwich_result`) — 필드별 타입/제약
3. **응답 메시지 스키마** (`feedback`, `error`) — `error.code` 값 목록
4. **레시피 ID 목록** — `data/recipes.json`에서 동적으로 뽑을지, 표로 박제할지 → **표로 박제** (UE 코드에 상수로 넣기 쉬움)
   - `italian_bmt`, `tuna`, `ham`, `egg_mayo`, `blt`
5. **재료 식별자 규칙** — 현재 `recipes.json`에 `bread_choice`, `vegetable_choice`, `sauce_choice` 같은 **"_choice" 그룹 placeholder**가 있음. UE에서 구체 값(예: `white_bread`, `lettuce`, `ranch`)으로 치환해 전송할지, placeholder 그대로 전송할지 **정책 결정 필요** → 문서에 **TBD 섹션**으로 명시하고 UE팀 회신 요청.
6. **재료 순서 규칙** — `order` 배열은 맨 아래 빵(`bread_bottom`)부터 위쪽 빵(`bread_top`)까지 적재 순서. `ingredients_order`도 동일 방향.
7. **에러 타입 4종** — `wrong_order` / `missing_ingredient` / `wrong_ingredient` / `time_exceeded`
8. **샘플 요청/응답 JSON**

### 문서 업데이트: `LLM/llm_feedback.md`

- 프로젝트 구조 섹션에 `LLM/__init__.py`, `LLM/main.py`, `LLM/docs/ue_contract.md` 추가
- "(참고) main.py 미작성 TODO" 문구 제거
- 포트(`8000`) 및 기동 방법(`python -m LLM.main`) 명시

---

## 실행 / 검증 방법

### 기동
프로젝트 루트(`WP-SandwichTrainer/`)에서:

```bash
python -m LLM.main
# 또는
uvicorn LLM.main:app --host 0.0.0.0 --port 8000 --reload
```

### 검증 체크리스트

1. **임포트 정상** — `python -c "from LLM.main import app"` 에러 없음
2. **서버 기동** — `python -m LLM.main` → `Uvicorn running on ... 8000` 로그
3. **루트 응답** — `curl http://localhost:8000/` → `{"service":"llm-feedback",...}`
4. **에러 응답: type 누락/오타** — `{"type": "foo", "session_id": "s1"}` 전송 → `{"type":"error","session_id":"s1","code":"unsupported_type",...}`
5. **에러 응답: 필드 누락** — `{"type":"sandwich_result","session_id":"s2"}` 전송 → `{"type":"error","session_id":"s2","code":"invalid_payload",...}`
6. **에러 응답: 알수없는 recipe_id** — 정상 payload에 `recipe_id: "not_exist"` → (feedback 응답, "레시피를 찾을 수 없습니다.")
7. **정상 피드백 (선택, OpenAI 과금)** — 정상 payload(`italian_bmt`) 전송 → `FeedbackResponse` 수신

---

## 리스크 / 주의

- **OpenAI 실호출 비용** — 검증 7번은 과금. 필요 시 `LLM_MODEL` 환경변수를 더미 값으로 바꾸고 수동 mock 고려 (본 작업에는 포함하지 않음).
- **`LLM/llm_feedback.md` 수정 범위** — 구조/진입점/포트만 업데이트. 상위 설계 의도는 건드리지 않음.
- **`_choice` placeholder 정책** (이슈 #6) — 계약 문서에 **TBD**로 남김. UE팀 합의 전까지 서버는 어떤 재료 문자열이든 받아들임 (현재 스키마 `list[str]` 유지).

---

## 체크리스트 (작업 순서)

- [ ] `LLM/__init__.py` 신규 (빈 파일)
- [ ] `LLM/schemas/request.py` — `type` Literal 고정
- [ ] `LLM/schemas/response.py` — `ErrorResponse` 추가
- [ ] `LLM/services/feedback.py` — 임포트 4건 패키지 경로화
- [ ] `LLM/services/recipe.py` — 변경 없음 (확인만)
- [ ] `LLM/routers/ws.py` — 임포트 + type 분기 + session_id 보존 에러 응답
- [ ] `LLM/main.py` 신규 — FastAPI 앱 + CORS + include_router + `__main__`
- [ ] `LLM/docs/ue_contract.md` 신규 — UE팀 공유 계약서
- [ ] `LLM/llm_feedback.md` — 구조/기동 섹션 갱신
- [ ] 기동 검증 (루트에서 `python -m LLM.main`)
- [ ] WebSocket 에러 케이스 수동 검증 (4,5,6번)
- [ ] (선택) OpenAI 실호출 end-to-end 검증

---

## 범위 제외

- 자동화 테스트 (`pytest`) 추가 — 별도 차수
- UE 클라이언트 코드 변경 — UE 담당자 영역
- `_choice` placeholder 정책 결정 — UE팀 회신 후 반영
