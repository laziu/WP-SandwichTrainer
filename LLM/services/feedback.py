import json
from pathlib import Path

from openai import AsyncOpenAI

import config
from schemas.request import SandwichResult
from schemas.response import FeedbackResponse
from services.recipe import get_recipe

client = AsyncOpenAI(api_key=config.OPENAI_API_KEY)

_SYSTEM_PROMPT_PATH = Path(__file__).parent.parent / "prompts" / "feedback_system.txt"
_system_prompt: str | None = None


def _load_system_prompt() -> str:
    global _system_prompt
    if _system_prompt is None:
        with open(_SYSTEM_PROMPT_PATH, encoding="utf-8") as f:
            _system_prompt = f.read().strip()
    return _system_prompt


def build_user_prompt(recipe: dict, result: SandwichResult) -> str:
    errors_text = "\n".join(
        f"- {e.type.value}: {e.detail}" for e in result.errors
    )
    if not errors_text:
        errors_text = "없음"

    return f"""[레시피 정보]
레시피: {recipe["name"]}
정답 재료: {", ".join(recipe["ingredients"])}
정답 순서: {" → ".join(recipe["order"])}

[사용자 결과]
사용한 재료: {", ".join(result.ingredients_used)}
재료 순서: {" → ".join(result.ingredients_order)}
점수: {result.score}점
소요 시간: {result.time_seconds}초

[오류 목록]
{errors_text}

위 결과를 분석해서 피드백을 생성해줘.
JSON 형식으로 응답: {{"feedback_text": "...", "tips": ["...", "..."]}}"""


async def generate_feedback(result: SandwichResult) -> FeedbackResponse:
    recipe = get_recipe(result.recipe_id)
    if recipe is None:
        return FeedbackResponse(
            session_id=result.session_id,
            feedback_text="레시피를 찾을 수 없습니다.",
            tips=[],
        )

    system_prompt = _load_system_prompt()
    user_prompt = build_user_prompt(recipe, result)

    response = await client.chat.completions.create(
        model=config.LLM_MODEL,
        temperature=config.LLM_TEMPERATURE,
        response_format={"type": "json_object"},
        messages=[
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_prompt},
        ],
    )

    try:
        content = response.choices[0].message.content
        data = json.loads(content)
        return FeedbackResponse(
            session_id=result.session_id,
            feedback_text=data["feedback_text"],
            tips=data["tips"],
        )
    except (json.JSONDecodeError, KeyError, TypeError, IndexError):
        return FeedbackResponse(
            session_id=result.session_id,
            feedback_text="피드백 생성 중 오류가 발생했습니다. 다시 시도해주세요.",
            tips=[],
        )
