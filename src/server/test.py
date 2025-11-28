
def test_pulp(): ...
def test_server(): ...

def test_pulp():
    from milp_pulp import MyPulp
    from sql import DataBase

    def analyz_data(r):
        import polars as pl

        data = pl.read_csv('res.csv')
        print(data)

        res = data.filter(pl.col('times') == '11:10')
        print(res)
        print(r.days)

    db = DataBase()
    r = MyPulp(db)

    r.solve()
    #r.save_csv()
    #r.print_res()

    #analyz_data(r)
    print(r.get_json())

def test_server():
    import tkinter as tk
    from tkinter import messagebox
    import requests
    import json

    API_URL = "http://127.0.0.1:8000/"  # наш API

    def hello():
        try:
            resp = requests.get(API_URL + '') #json={"x1": x1, "x2": x2}
            resp.raise_for_status()
            data = resp.json()
            result_label.config(text=f"Result: {data["result"]}")
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def solve_pulp():
        try:
            with open("temp/input_1.json", "r") as file:
                inp = json.load(file)
            resp = requests.post(API_URL + 'solve_pulp_2/', json=inp) #json={"x1": x1, "x2": x2}
            resp.raise_for_status()
            data = resp.json()
            result_label.config(text=f"Result: {data["result"]}")
            print(type(data))
        except Exception as e:
            messagebox.showerror("Error", str(e))

    root = tk.Tk()
    root.geometry('500x600')
    root.title("PyTorch API Client")

    tk.Label(root, text="x1:").grid(row=0, column=0, padx=5, pady=5)
    entry_x1 = tk.Entry(root)
    entry_x1.grid(row=0, column=1, columnspan=2, padx=5, pady=5)

    tk.Label(root, text="x2:").grid(row=1, column=0, padx=5, pady=5)
    entry_x2 = tk.Entry(root)
    entry_x2.grid(row=1, column=1, columnspan=2, padx=5, pady=5)

    predict_button = tk.Button(root, text="Hello", command=hello)
    predict_button.grid(row=2, column=1, columnspan=1, pady=10)

    predict_button = tk.Button(root, text="solve_pulp", command=solve_pulp)
    predict_button.grid(row=2, column=2, columnspan=1, pady=10)

    result_label = tk.Label(root, text="Result: ", wraplength=500)
    result_label.grid(row=3, column=0, columnspan=3, pady=5)

    root.mainloop()

def create_default_json():
    import json
    from collections import defaultdict

    subject_count = {
        ("A-18-30", "math"): 2,
        ("A-16-30", "math"): 1,
    }

    default_count = 2

    groups = ["A-02-30", "A-05-30", "A-08-30", "A-16-30", "A-18-30"]
    subjects = ["math", "physics", "english", "IT", "economic"]
    teachers = ["T1", "T2", "T3", "T4", "T5", "T6", "T7"]
    rooms = ["m700", "m707", "m200"]
    days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"]
    times = ["9:20", "11:10", "13:45", "15:35"]

    subject_teachers = {
        "math": ["T1"],
        "physics": ["T2"],
        "english": ["T3"],
        "economic": ["T4"],
        "IT": ["T5", "T6", "T7"]
    }

    subject_count_json = [
        #[g, s, c]
        {"group":g, "subject":s, "count":c}
        for (g, s), c in subject_count.items()
    ]

    subject_count_json = defaultdict(dict)

    for (g, s), c in subject_count.items():
        subject_count_json[g][s] = c

    # Если нужно получить обычный dict:
    subject_count_json = dict(subject_count_json)

    # структура
    data = {
        "default_count": default_count,
        "groups": groups,
        "subjects": subjects,
        "teachers": teachers,
        "rooms": rooms,
        "days": days,
        "times": times,
        "subject_teachers": subject_teachers,
        "subject_count": subject_count_json
    }

    # Сохраняем в UTF-8 (ensure_ascii=False)
    with open("temp/input_1.json", "w", encoding="utf-8") as f:
        json.dump(data, f, indent=4, separators=(",", ": "), ensure_ascii=False)

    with open("temp/input_2.json", "w", encoding="utf-8") as f:
        json.dump(data, f, separators=(",", ":"), ensure_ascii=False)

    print("Файл data.json успешно сохранён!")



#test_pulp()
test_server()
#create_default_json()