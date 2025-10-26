from milp_pulp import MyPulp
from sql import DataBase

from fastapi import FastAPI
from os.path import exists
#from pydantic import BaseModel  # Понадобится позже для формата данных по API

PATH_DB = 'mydb.db'
SQL_URL_ENGINE = f"sqlite:///{PATH_DB}"
DEBUG = True
FILE_DB_IS_EXISTS = exists(PATH_DB)

db = DataBase(SQL_URL_ENGINE)
if DEBUG and not FILE_DB_IS_EXISTS: db.create_tables(), db.init_data()
r = MyPulp(db)
app = FastAPI(debug=DEBUG)

@app.get("/solve_pulp")
def predict():
    r.solve()
    return {"result": r.get_json()}

@app.get("/")
def hello():
    return {"result": "Hello"}