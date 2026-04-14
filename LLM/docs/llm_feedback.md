# LLM 피드백 서버 설계 문서

## 개요

Sandwich Trainer XR의 LLM 서버 컴포넌트.
Unreal Engine에서 샌드위치 제작 결과를 WebSocket으로 수신하고, OpenAI API를 통해 유쾌한 교육 피드백을 생성하여 반환한다.

## 시스템 아키텍처

```
┌─────────────────┐    WebSocket     ┌─────────────────────────┐
│  Unreal Engine   │◄──────────────►│   FastAPI Server         │
│                  │                 │                          │
│  - 조립 결과 전송  │    ──────►     │  /ws  WebSocket endpoint │
│  - 피드백 수신     │    ◄──────     │                          │
└─────────────────┘                 │  ┌───────────────────┐   │
                                    │  │ Recipe DB (JSON)   │   │
                                    │  └───────────────────┘   │
                                    │           │              │
                                    │           ▼              │
                                    │  ┌───────────────────┐   │
                                    │  │ Prompt Builder     │   │
                                    │  └───────────────────┘   │
                                    │           │              │
                                    │           ▼              │
                                    │  ┌───────────────────┐   │
                                    │  │ OpenAI API        │   │
                                    │  └───────────────────┘   │
                                    └─────────────────────────┘
```

### 핵심 흐름

1. UE가 WebSocket으로 샌드위치 완성 결과 JSON 전송
2. FastAPI가 해당 레시피를 JSON DB에서 조회
3. 결과 + 레시피를 조합해서 프롬프트 빌드
4. OpenAI API 호출 → 피드백 텍스트 생성
5. WebSocket으로 UE에 피드백 반환

## WebSocket 메시지 스키마

### UE → LLM Server (요청)

```json
{
  "type": "sandwich_result",
  "session_id": "string (고유 세션 ID)",
  "recipe_id": "string (레시피 식별자, 예: turkey_classic)",
  "score": "number (0-100, UE에서 계산한 점수)",
  "time_seconds": "number (소요 시간, 초)",
  "ingredients_used": ["string (사용한 재료 목록)"],
  "ingredients_order": ["string (재료 배치 순서)"],
  "errors": [
    {
      "type": "string (wrong_order | missing_ingredient | wrong_ingredient | time_exceeded)",
      "detail": "string (오류 상세)"
    }
  ]
}
```

**예시:**
```json
{
  "type": "sandwich_result",
  "session_id": "abc-123",
  "recipe_id": "turkey_classic",
  "score": 75,
  "time_seconds": 45,
  "ingredients_used": ["white_bread", "turkey", "lettuce", "tomato", "ranch"],
  "ingredients_order": ["white_bread", "turkey", "lettuce", "tomato", "ranch", "white_bread"],
  "errors": [
    {"type": "wrong_order", "detail": "tomato before lettuce"},
    {"type": "missing_ingredient", "detail": "cheese"}
  ]
}
```

### LLM Server → UE (응답)

```json
{
  "type": "feedback",
  "session_id": "string (요청과 동일한 세션 ID)",
  "feedback_text": "string (LLM이 생성한 종합 피드백)",
  "tips": ["string (핵심 개선 포인트, 1-3개)"]
}
```

**예시:**
```json
{
  "type": "feedback",
  "session_id": "abc-123",
  "feedback_text": "이야~ 거의 다 왔는데! 치즈를 깜빡했네~ 터키 샌드위치엔 치즈가 빠지면 서운하지! 그리고 토마토는 양상추 위에 올려야 안 미끄러져. 다음엔 한번 순서 신경 써봐!",
  "tips": [
    "치즈를 잊지 말자!",
    "양상추 → 토마토 순서 기억!"
  ]
}
```

### 에러 타입 정의

| type | 의미 | detail 예시 |
|------|------|-------------|
| `wrong_order` | 재료 순서 오류 | "tomato before lettuce" |
| `missing_ingredient` | 재료 누락 | "cheese" |
| `wrong_ingredient` | 잘못된 재료 사용 | "used cheddar instead of swiss" |
| `time_exceeded` | 시간 초과 | "exceeded 60s limit" |

## 프롬프트 설계

### 시스템 프롬프트

```
너는 샌드위치 가게의 베테랑 알바 선배야. 플래시 게임 느낌으로 유쾌하고 가볍게 피드백해줘.
- 반말 사용
- 짧고 재밌게
- 잘한 점도 꼭 언급
- 개선점은 구체적으로
- tips 배열에는 핵심 포인트만 짧게
```

### 유저 프롬프트 (동적 조립)

```
[레시피 정보]
레시피: {recipe_name}
정답 재료: {correct_ingredients}
정답 순서: {correct_order}

[사용자 결과]
사용한 재료: {ingredients_used}
재료 순서: {ingredients_order}
점수: {score}점
소요 시간: {time_seconds}초

[오류 목록]
{errors}

위 결과를 분석해서 피드백을 생성해줘.
JSON 형식으로 응답: {"feedback_text": "...", "tips": ["...", "..."]}
```

### OpenAI 호출 설정

- 모델: `gpt-4o-mini`
- `response_format: {"type": "json_object"}`
- `temperature: 0.8`

## 프로젝트 구조

LLM 모듈은 프로젝트 루트(`WP-SandwichTrainer/`)의 `LLM/` 하위에 위치하며, 팀원들의 `server/`, `client/`, `Game/`와 병렬로 존재한다.

```
WP-SandwichTrainer/
├── server/                      # (팀원) MediaPipe 손추적 FastAPI 서버 (port 8888)
├── client/                      # (팀원) REST/WebSocket 클라이언트
├── Game/                        # (팀원) Unreal 프로젝트
├── pyproject.toml               # 루트 의존성 (openai, python-dotenv 포함)
└── LLM/
    ├── __init__.py              # 패키지 마커
    ├── .env                     # API 키 등 환경변수 (gitignore 대상)
    ├── config.py                # 환경변수 로드 (OPENAI_API_KEY, LLM_MODEL, LLM_TEMPERATURE, SERVER_HOST/PORT)
    ├── main.py                  # FastAPI 앱 진입점 (port 8000)
    ├── routers/
    │   ├── __init__.py
    │   └── ws.py                # WebSocket 라우터 (/ws 엔드포인트, type/에러 분기 포함)
    ├── services/
    │   ├── __init__.py
    │   ├── feedback.py          # 피드백 생성 로직 (프롬프트 조립 + OpenAI 호출)
    │   └── recipe.py            # 레시피 DB 조회
    ├── schemas/
    │   ├── __init__.py
    │   ├── request.py           # UE → Server 메시지 모델 (SandwichResult, SandwichError, ErrorType)
    │   └── response.py          # Server → UE 메시지 모델 (FeedbackResponse, ErrorResponse)
    ├── data/
    │   └── recipes.json         # 레시피 데이터
    ├── prompts/
    │   └── feedback_system.txt  # 시스템 프롬프트 템플릿
    ├── docs/
    │   └── ue_contract.md       # UE팀 공유 계약서 (엔드포인트/스키마/레시피 ID)
    └── llm_feedback.md          # 본 설계 문서
```

> **기동:** 프로젝트 루트에서 `python -m LLM.main` 또는 `uvicorn LLM.main:app --port 8000 --reload`. 팀 손추적 서버(`server/`, port 8888)와 독립 프로세스로 운영한다.

### 모듈 역할

| 모듈 | 역할 |
|------|------|
| `LLM/config.py` | `.env` 로드 → `OPENAI_API_KEY`, `LLM_MODEL`, `LLM_TEMPERATURE`, `SERVER_HOST`, `SERVER_PORT` 노출 |
| `LLM/routers/ws.py` | `/ws` WebSocket 연결 관리, JSON 수신 → `generate_feedback` 호출 → JSON 응답 |
| `LLM/services/feedback.py` | 시스템 프롬프트 로드 + 유저 프롬프트 조립 + OpenAI 호출 + 응답 파싱 |
| `LLM/services/recipe.py` | `data/recipes.json` 로드/조회 (`get_recipe(recipe_id)`) |
| `LLM/schemas/request.py` | `SandwichResult`, `SandwichError`, `ErrorType` (입력 검증) |
| `LLM/schemas/response.py` | `FeedbackResponse` (출력 검증) |
| `LLM/prompts/feedback_system.txt` | 시스템 프롬프트 원본 텍스트 |
| `LLM/data/recipes.json` | 레시피 메타데이터 (이름, 정답 재료, 정답 순서) |

## 기술 스택

| 기술 | 버전/비고 |
|------|----------|
| Python | 3.12+ |
| FastAPI | WebSocket 지원 |
| OpenAI Python SDK | `openai` 패키지 |
| Pydantic | v2 (FastAPI 내장) |
| uvicorn | ASGI 서버 |
| python-dotenv | .env 로드 |


## MVP 범위

**포함:**
- 완성 후 종합 피드백 1회
- WebSocket 단일 연결
- JSON 기반 레시피 저장

**제외 (확장 시):**
- 실시간 이벤트 스트리밍 피드백
- 세션 히스토리 저장
- 반복 오류 패턴 분석

## 확장 계획

1. **실시간 피드백:** WebSocket 이벤트 핸들러 추가, 재료 집을 때마다 즉시 피드백
2. **다중 레시피:** `recipes.json`에 레시피 추가만으로 확장
3. **세션 히스토리:** SQLite/PostgreSQL로 세션 결과 저장, 반복 오류 패턴 분석
