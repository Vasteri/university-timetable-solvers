from schemas import InputData, SolveRequest, GAParams
from milp_pulp import MyPulp
from ga_solver import ScheduleOptimizer

from fastapi import FastAPI
from fastapi.responses import ORJSONResponse

DEBUG = True

app = FastAPI(debug=DEBUG)

@app.get("/")
def hello():
    return {"result": "Hello"}

@app.post("/solve_pulp", response_class=ORJSONResponse)
def solve_pulp(input: InputData):
    r = MyPulp(json_data=input)
    r.solve()
    return {"result": r.get_json_list(),
            "method": "milp",
            "status": r.status}

@app.post("/solve_genetic", response_class=ORJSONResponse)
def solve(input: SolveRequest):
    params = input.ga_params or GAParams()
    raw = input.model_dump(exclude={"method", "ga_params"})
    opt = ScheduleOptimizer(raw).fit(
        pop_size=params.pop_size,
        generations=params.generations,
        crossover_rate=params.crossover_rate,
        mutation_rate=params.mutation_rate,
        elite_size=params.elite_size,
        tournament_size=params.tournament_size,
        local_search_rate=params.local_search_rate,
        local_search_attempts=params.local_search_attempts,
        random_seed=params.seed,
        verbose=params.verbose,
    )
    return {
        "result": opt.to_table(),
        "method": "ga",
        "meta": {"penalty": opt.best_penalty_, "history": opt.history_},
    }