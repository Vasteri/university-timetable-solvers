
def test_sql(): ...
def test_pulp(): ...
def test_server(): ...

def test_sql():
    import sql

    db = sql.DataBase()

    db.drop_tables()
    db.create_tables()
    db.init_data()

    groups = db.get_groups()
    subjects = db.get_subjects()
    teachers = db.get_teachers()
    rooms = db.get_rooms()
    days = db.get_days()
    times = db.get_times()
    subject_teachers = db.get_subject_teachers()

    print(groups)
    print(subjects)
    print(teachers)
    print(rooms)
    print(days)
    print(times)
    print(subject_teachers)

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
            resp = requests.get(API_URL + 'solve_pulp/') #json={"x1": x1, "x2": x2}
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



#test_sql()
#test_pulp()
test_server()