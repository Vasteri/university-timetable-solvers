from milp_pulp import MyPulp
from sql import DataBase

def analyz_data(r):
    import polars as pl

    data = pl.read_csv('res.csv')
    print(data)

    res = data.filter(pl.col('times') == '11:10')
    print(res)
    print(r.days)

def main():
    db = DataBase()
    r = MyPulp(db)

    r.solve()
    r.save_csv()
    r.print_res()

    analyz_data(r)
    

if __name__ == "__main__":
    main()