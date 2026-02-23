from milp_pulp import MyPulp
from ga_solver import ScheduleOptimizer

from fastapi import FastAPI
from fastapi.responses import ORJSONResponse
from pydantic import BaseModel
from typing import Dict, List, Literal, Optional
import json

class SubjectCountItem(BaseModel):
    group: str
    subject: str
    count: int

class InputData(BaseModel):
    default_count: int
    groups: List[str]
    subjects: List[str]
    teachers: List[str]
    rooms: List[str]
    days: List[str]
    times: List[str]
    subject_teachers: Dict[str, List[str]]
    subject_count: Dict[str, Dict[str, int]]

class GAParams(BaseModel):
    pop_size: int = 500
    generations: int = 500
    crossover_rate: float = 0.7
    mutation_rate: float = 0.01
    elite_size: int = 50
    tournament_size: int = 12
    seed: Optional[int] = None
    verbose: bool = True

class SolveRequest(InputData):
    method: Literal["milp", "ga"] = "milp"
    ga_params: Optional[GAParams] = None

DEBUG = True

app = FastAPI(debug=DEBUG)

@app.get("/solve_pulp", response_class=ORJSONResponse)
def solve_pulp():
    r = MyPulp()
    r.solve()
    """    with open("temp/res_1.json", "w", encoding="utf-8") as f:
        json.dump(r.get_json(), f, indent=4, separators=(",", ": "), ensure_ascii=False)
    with open("temp/res_2.json", "w", encoding="utf-8") as f:
        json.dump(r.get_json_2(), f, indent=4, separators=(",", ": "), ensure_ascii=False)
    with open("temp/res_3.json", "w", encoding="utf-8") as f:
        json.dump(r.get_json_2(), f, separators=(",", ":"), ensure_ascii=False)"""
    return {"result": r.get_json_2()}

@app.post("/solve_pulp_2", response_class=ORJSONResponse)
def solve_pulp_2(input:InputData):
    r = MyPulp(json_data=input)
    r.solve()
    return {"result": r.get_json_2()}

@app.post("/solve", response_class=ORJSONResponse)
def solve(input: SolveRequest):
    if input.method == "milp":
        r = MyPulp(json_data=input)
        r.solve()
        return {"result": r.get_json_2(), "method": "milp"}

    params = input.ga_params or GAParams()
    raw = input.model_dump(exclude={"method", "ga_params"})
    opt = ScheduleOptimizer(raw).fit(
        pop_size=params.pop_size,
        generations=params.generations,
        crossover_rate=params.crossover_rate,
        mutation_rate=params.mutation_rate,
        elite_size=params.elite_size,
        tournament_size=params.tournament_size,
        random_seed=params.seed,
        verbose=params.verbose,
    )
    return {
        "result": opt.to_table(),
        "method": "ga",
        "meta": {"penalty": opt.best_penalty_, "history": opt.history_},
    }

@app.get("/")
def hello():
    return {"result": "Hello"}