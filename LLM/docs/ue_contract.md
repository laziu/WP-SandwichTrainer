# UE ↔ LLM 피드백 서버 계약서

이 문서는 Unreal Engine 클라이언트와 LLM 피드백 서버 간의 WebSocket 통신 계약을 정의합니다.

## 연결 정보

- **엔드포인트**: `ws://<host>:8000/ws`
- **기본 호스트**: 개발 환경 `localhost`, 운영 환경 별도 공유
- **포트**: `8000` (팀 손추적 서버 `8888`과 분리)
- **프로토콜**: JSON over WebSocket

---

## 요청 메시지 (UE → Server)

### `sandwich_result`

샌드위치 완성 결과를 서버로 전송.

| 필드 | 타입 | 필수 | 설명 |
|---|---|---|---|
| `type` | `"sandwich_result"` (고정) | ✓ | 메시지 타입 식별자. 다른 값이면 `unsupported_type` 에러 |
| `session_id` | string | ✓ | UE가 생성한 고유 세션 ID. 응답과 매칭하는 용도 |
| `recipe_id` | string | ✓ | 레시피 식별자. 아래 [레시피 ID 목록](#레시피-id-목록) 참조 |
| `score` | int | ✓ | UE에서 계산한 점수 (0~100) |
| `time_seconds` | int | ✓ | 소요 시간 (초) |
| `ingredients_used` | string[] | ✓ | 사용한 재료 목록 |
| `ingredients_order` | string[] | ✓ | 재료 배치 순서 (맨 아래부터 위쪽 순서) |
| `errors` | SandwichError[] | ✓ | 오류 목록. 빈 배열 허용 |

### `SandwichError`

| 필드 | 타입 | 설명 |
|---|---|---|
| `type` | enum | `wrong_order` \| `missing_ingredient` \| `wrong_ingredient` \| `time_exceeded` |
| `detail` | string | 오류 상세 (자유 텍스트) |

### 예시

```json
{
  "type": "sandwich_result",
  "session_id": "abc-123",
  "recipe_id": "italian_bmt",
  "score": 75,
  "time_seconds": 45,
  "ingredients_used": ["white_bread", "pepperoni", "salami", "ham", "lettuce", "ranch"],
  "ingredients_order": ["bread_bottom", "pepperoni", "salami", "ham", "lettuce", "ranch", "bread_top"],
  "errors": [
    {"type": "missing_ingredient", "detail": "올리브"},
    {"type": "wrong_order", "detail": "햄이 살라미보다 먼저"}
  ]
}
```

---

## 응답 메시지 (Server → UE)

### `feedback` (정상 응답)

| 필드 | 타입 | 설명 |
|---|---|---|
| `type` | `"feedback"` (고정) | 메시지 타입 |
| `session_id` | string | 요청과 동일한 세션 ID |
| `feedback_text` | string | LLM이 생성한 종합 피드백 |
| `tips` | string[] | 핵심 개선 포인트 1~3개 |

### `error` (에러 응답)

| 필드 | 타입 | 설명 |
|---|---|---|
| `type` | `"error"` (고정) | 메시지 타입 |
| `session_id` | string \| null | 요청에서 식별 가능하면 동일 값, 불가능하면 `null` |
| `code` | string | 아래 [에러 코드 목록](#에러-코드-목록) 참조 |
| `message` | string | 사람이 읽을 수 있는 에러 메시지 |

### 에러 코드 목록

| code | 상황 |
|---|---|
| `unsupported_type` | 요청의 `type` 필드가 `sandwich_result`가 아님 |
| `invalid_payload` | 필드 누락/타입 오류 등 스키마 검증 실패 |
| `internal_error` | 피드백 생성 중 서버 내부 에러 (OpenAI 실패 등) |

> 참고: `recipe_id`가 DB에 없는 경우는 **에러가 아니라** `feedback` 응답으로 `feedback_text: "레시피를 찾을 수 없습니다."`가 반환됩니다.

### 예시

```json
{
  "type": "feedback",
  "session_id": "abc-123",
  "feedback_text": "이야~ 거의 다 왔는데! 올리브 빼먹은 건 아쉽다. 다음엔 순서 한번 더 확인해봐!",
  "tips": ["올리브 잊지 말기", "살라미 → 햄 순서 지키기"]
}
```

```json
{
  "type": "error",
  "session_id": "abc-123",
  "code": "invalid_payload",
  "message": "1 validation error for SandwichResult\nscore\n  field required"
}
```

---

## 레시피 ID 목록

현재 서버에 등록된 레시피 (`LLM/data/recipes.json`):

| recipe_id | 이름 |
|---|---|
| `italian_bmt` | 이탈리안 비엠티 |
| `tuna` | 참치 |
| `ham` | 햄 |
| `egg_mayo` | 에그마요 |
| `blt` | 비엘티 |

신규 레시피 추가는 `recipes.json` 수정만으로 가능. UE 상수 동기화 필요.

---

## 재료 순서 규칙

- `ingredients_order`는 **아래에서 위로 쌓인 순서**
- 시작: `bread_bottom` (맨 아래 빵)
- 끝: `bread_top` (맨 위 빵)

예:
```
["bread_bottom", "pepperoni", "salami", "ham", "vegetable_choice", "sauce_choice", "bread_top"]
```

---

## ⚠️ TBD: 재료 식별자 정책 (UE팀 회신 필요)

현재 `recipes.json`의 `ingredients` / `order` 배열에는 **두 종류의 식별자**가 혼재합니다.

1. **구체 재료명**: `pepperoni`, `ham`, `egg`, `bacon`, `tuna`, `mayonnaise`
2. **그룹 placeholder** (`_choice` 접미사): `bread_choice`, `vegetable_choice`, `sauce_choice`

**결정 필요 사항**: UE에서 전송하는 `ingredients_used` / `ingredients_order` 배열에 placeholder와 구체값 중 무엇을 담을 것인가?

- **옵션 A**: UE가 구체값으로 치환해 전송 (예: `bread_choice` → `white_bread`)
  - 레시피의 placeholder와 직접 비교 불가 → 서버에서 그룹 매핑 테이블 필요
- **옵션 B**: placeholder 그대로 전송 (예: UE가 어떤 빵을 선택해도 `bread_choice`로 전송)
  - 단순 비교 가능하나, LLM 피드백에서 "어떤 빵을 골랐나"를 언급할 수 없음
- **옵션 C**: 이중 필드 (구체값 + 그룹)

**현재 서버 동작**: `list[str]`을 그대로 받아 LLM 프롬프트에 삽입하므로 어느 옵션이든 기술적 차단은 없음. 단 피드백 품질/레시피 매칭 정확도가 달라짐.

→ UE 담당자와 합의 후 본 문서 업데이트 예정.

---

## 버전 / 변경 이력

- `0.1.0` (2026-04-14) — 초안. 이슈 #1~#6 정비 일괄 반영.
