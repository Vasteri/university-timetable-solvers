from milp_pulp import MyPulp

from fastapi import FastAPI
from fastapi.responses import ORJSONResponse
from pydantic import BaseModel
from typing import List, Dict
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
    subject_count: List[SubjectCountItem]

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
    return {"result": r.get_json()}

@app.post("/solve_pulp_2", response_class=ORJSONResponse)
def solve_pulp_2(input:InputData):
    r = MyPulp(json_data=input)
    r.solve()
    return {"result": r.get_json_2()}

@app.get("/")
def hello():
    return {"result": "Hello"}