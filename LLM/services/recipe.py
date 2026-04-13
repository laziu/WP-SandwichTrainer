import json
from pathlib import Path

_RECIPES_PATH = Path(__file__).parent.parent / "data" / "recipes.json"
_recipes: dict | None = None


def load_recipes() -> dict:
    global _recipes
    if _recipes is None:
        with open(_RECIPES_PATH, encoding="utf-8") as f:
            _recipes = json.load(f)
    return _recipes


def get_recipe(recipe_id: str) -> dict | None:
    recipes = load_recipes()
    return recipes.get(recipe_id)
