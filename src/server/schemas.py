from pydantic import BaseModel
from typing import Dict, List, Literal, Optional

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
    local_search_rate: float = 0.3
    local_search_attempts: int = 15
    seed: Optional[int] = None
    verbose: bool = True

class SolveRequest(InputData):
    method: Literal["milp", "ga"] = "milp"
    ga_params: Optional[GAParams] = None
