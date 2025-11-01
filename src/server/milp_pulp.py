from pulp import LpProblem, LpMinimize, LpVariable, lpSum, LpStatus
from collections import defaultdict
from pandas import DataFrame
import pulp


class MyPulp:
    def __init__(self, db):
        self.db = db
        self.set_db_values()
        self.names = ["groups", "subjects", "days", "times", "rooms", "teachers"]
        self.assigned = list()
        self.problem = LpProblem("University_Timetable", LpMinimize)
        self._init_variables()
        self._init_objective_function()
        self._init_constraints()

    def get_json(self):
        schedule = [
            {
                "group": g,
                "subject": s,
                "day": d,
                "time": t,
                "room": r,
                "teacher": tea
            }
            for (g, s, d, t, r, tea) in self.assigned
        ]
        return schedule

    def _init_objective_function(self):
        # Минимизировать количество пар в первой паре (11:10)
        self.problem += lpSum(
            self.x[(g, s, d, "11:10", r, tea)] 
            for g in self.groups for s in self.subjects for d in self.days 
            for r in self.rooms for tea in self.subject_teachers[s]
        )

    def set_default_values(self):
        self.subject_count = {
            ("A-18-30", "math"): 2,
            ("A-16-30", "math"): 1,
        }

        self.default_count = 2

        self.groups = ["A-02-30", "A-05-30", "A-08-30", "A-16-30", "A-18-30"]
        self.subjects = ["math", "physics", "english", "IT", "economic"]
        self.teachers = ["T1", "T2", "T3", "T4", "T5", "T6", "T7"]  
        self.rooms = ["m700", "m707", "m200"]
        self.days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"] 
        self.times = ["9:20", "11:10", "13:45", "15:35"] 

        self.subject_teachers = {
            "math": ["T1"],
            "physics": ["T2"],
            "english": ["T3"],
            "economic": ["T4"],
            "IT": ["T5", "T6", "T7"]
        }

    def set_db_values(self):
        self.subject_count = {
            ("A-18-30", "math"): 2,
            ("A-16-30", "math"): 1,
        }
        self.default_count = 2
        self.groups = self.db.get_groups()
        self.subjects = self.db.get_subjects()
        self.teachers = self.db.get_teachers()
        self.rooms = self.db.get_rooms()
        self.days = self.db.get_days()
        self.times = self.db.get_times()
        self.subject_teachers = self.db.get_subject_teachers()

    def _init_constraints(self):
        # 1) Для всех (g, s) задать нужное количество занятий в неделю
        for g in self.groups:
            for s in self.subjects:
                count = self.subject_count.get((g, s), self.default_count)
                self.problem += lpSum(
                    self.x[(g, s, d, tt, r, tea)]
                    for d in self.days for tt in self.times for r in self.rooms for tea in self.subject_teachers[s]
                ) == count, f"Num_slots_for_{g}_{s}"


        # 2) Для каждого (day, time_slot, room) не более 1 занятия
        for d in self.days:
            for tt in self.times:
                for r in self.rooms:
                    self.problem += lpSum(
                        self.x[(g, s, d, tt, r, tea)]
                        for g in self.groups for s in self.subjects for tea in self.subject_teachers[s]
                    ) <= 1, f"Room_one_{d}_{tt}_{r}"

        # 3) Для каждого преподавателя в (day, time_slot) не более 1 занятия
        for tea in self.teachers:
            for d in self.days:
                for tt in self.times:
                    self.problem += lpSum(
                        self.x[(g, s, d, tt, r, tea)]
                        for g in self.groups for s in self.subjects if tea in self.subject_teachers[s] for r in self.rooms
                    ) <= 1, f"Teacher_one_{tea}_{d}_{tt}"

        # 4) Для каждой группы в (day, time_slot) не более 1 занятия
        for g in self.groups:
            for d in self.days:
                for tt in self.times:
                    self.problem += lpSum(
                        self.x[(g, s, d, tt, r, tea)]
                        for s in self.subjects for r in self.rooms for tea in self.subject_teachers[s]
                    ) <= 1, f"Group_one_{g}_{d}_{tt}"

    def _init_variables(self):
        self.x = {}
        for g in self.groups:
            for d in self.days:
                for tt in self.times:
                    for r in self.rooms:
                        for s in self.subjects:
                            allowed_teachers = self.subject_teachers[s]
                            for tea in allowed_teachers:
                                varname = f"x_{g}_{d}_{tt}_{r}_{s}_{tea}"
                                self.x[(g, s, d, tt, r, tea)] = LpVariable(varname, cat="Binary")

    def solve(self):
        print("Solving...")
        status = self.problem.solve(pulp.PULP_CBC_CMD(msg=True))
        print("Status:", LpStatus[self.problem.status])

        self.assigned = []
        for key, var in self.x.items():
            if var.varValue is not None and var.varValue > 0.5:
                g, s, d, tt, r, tea = key
                self.assigned.append((g, s, d, tt, r, tea))

    def save_csv(self):
        res = DataFrame(self.assigned)
        res.to_csv('res.csv', index=False, header=self.names)

    def print_res(self):
        print()
        for (g, s, d, tt, r, tea) in self.assigned:
            print(f"{g}\t{s:8}\t{d:8}\t{tt}\t{r}\t{tea}")
        print()

        schedule_by_slots = defaultdict(list)
        for (g, s, d, tt, r, tea) in self.assigned:
            schedule_by_slots[(g, d, tt)].append((s, r, tea))

        for g in self.groups:
            for d in self.days:
                for tt in self.times:
                    slot = (g, d, tt)
                    items = schedule_by_slots.get(slot, [])
                    print(f"{g} {d} {tt}:", end='')
                    if not items:
                        print("   (свободно)")
                    else:
                        for (s, r, tea) in items:
                            print(f"   {s} в {r} у {tea}")
                print("-" * 40)
            print("next group")


def example():
    import sql
    # Количество занятий по предметам для каждой группы в неделю
    subject_count = {
        ("A-18-30", "math"): 2,
        ("A-16-30", "math"): 1,
    }

    default_count = 2


    names = ["groups", "subjects", "days", "times", "rooms", "teachers"]

    db = sql.DataBase()

    groups = db.get_groups()
    subjects = db.get_subjects()
    teachers = db.get_teachers()
    rooms = db.get_rooms()
    days = db.get_days()
    times = db.get_times()
    subject_teachers = db.get_subject_teachers()
    """
    # Исходные множества
    groups = ["A-02-30", "A-05-30", "A-08-30", "A-16-30", "A-18-30"]
    subjects = ["math", "physics", "english", "IT", "economic"]
    teachers = ["T1", "T2", "T3", "T4", "T5", "T6", "T7"]  
    rooms = ["m700", "m707", "m200"]
    days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"] 
    times = ["9:20", "11:10", "13:45", "15:35"] 

    # Назначение преподавателей на предметы
    subject_teachers = {
        "math": ["T1"],
        "physics": ["T2"],
        "english": ["T3"],
        "economic": ["T4"],
        "IT": ["T5", "T6", "T7"]
    }
    """

    total_required_pairs = sum(subject_count.get((g, s), default_count) for g in groups for s in subjects)
    print("Обязательных пар в неделю:", total_required_pairs)

    # Создаём модель
    prob = LpProblem("University_Timetable", LpMinimize)

    # Переменные:
    # x[g,s,d,t,r,tea] == 1, если группа g на предмете s назначена в (d,t) в аудитории r у преподавателя tea
    x = {}
    for g in groups:
        for d in days:
            for tt in times:
                for r in rooms:
                    for s in subjects:
                        allowed_teachers = subject_teachers[s]
                        for tea in allowed_teachers:
                            varname = f"x_{g}_{d}_{tt}_{r}_{s}_{tea}"
                            x[(g, s, d, tt, r, tea)] = LpVariable(varname, cat="Binary")

    # Целевая функция
    # prob += lpSum([])  # нулевая целевая функция

    # Минимизировать количество пар в первой паре (11:10)
    early_penalty = lpSum(
        x[(g, s, d, "11:10", r, tea)] 
        for g in groups for s in subjects for d in days 
        for r in rooms for tea in subject_teachers[s]
    )

    prob += early_penalty

    # Ограничения

    # 1) Для всех (g, s) задать нужное количество занятий в неделю
    for g in groups:
        for s in subjects:
            count = subject_count.get((g, s), default_count)
            prob += lpSum(
                x[(g, s, d, tt, r, tea)]
                for d in days for tt in times for r in rooms for tea in subject_teachers[s]
            ) == count, f"Num_slots_for_{g}_{s}"


    # 2) Для каждого (day, time_slot, room) не более 1 занятия
    for d in days:
        for tt in times:
            for r in rooms:
                prob += lpSum(
                    x[(g, s, d, tt, r, tea)]
                    for g in groups for s in subjects for tea in subject_teachers[s]
                ) <= 1, f"Room_one_{d}_{tt}_{r}"

    # 3) Для каждого преподавателя в (day, time_slot) не более 1 занятия
    for tea in teachers:
        for d in days:
            for tt in times:
                prob += lpSum(
                    x[(g, s, d, tt, r, tea)]
                    for g in groups for s in subjects if tea in subject_teachers[s] for r in rooms
                ) <= 1, f"Teacher_one_{tea}_{d}_{tt}"

    # 4) Для каждой группы в (day, time_slot) не более 1 занятия
    for g in groups:
        for d in days:
            for tt in times:
                prob += lpSum(
                    x[(g, s, d, tt, r, tea)]
                    for s in subjects for r in rooms for tea in subject_teachers[s]
                ) <= 1, f"Group_one_{g}_{d}_{tt}"


    print("Solving...")
    status = prob.solve()
    print("Status:", LpStatus[prob.status])

    # Сбор результата — читаем назначенные переменные
    assigned = []
    for key, var in x.items():
        if var.varValue is not None and var.varValue > 0.5:
            g, s, d, tt, r, tea = key
            assigned.append((g, s, d, tt, r, tea))

    # Проверка количества назначений
    print("Назначено пар:", len(assigned))
    if len(assigned) != total_required_pairs:
        print("Внимание: назначено пар не совпадает с требуемым числом!")


    print()
    for (g, s, d, tt, r, tea) in assigned:
        print(f"{g}\t{s:8}\t{d:8}\t{tt}\t{r}\t{tea}")
    print()

    schedule_by_slots = defaultdict(list)
    for (g, s, d, tt, r, tea) in assigned:
        schedule_by_slots[(g, d, tt)].append((s, r, tea))

    for g in groups:
        for d in days:
            for tt in times:
                slot = (g, d, tt)
                items = schedule_by_slots.get(slot, [])
                print(f"{g} {d} {tt}:", end='')
                if not items:
                    print("   (свободно)")
                else:
                    for (s, r, tea) in items:
                        print(f"   {s} в {r} у {tea}")
            print("-" * 40)
        print("next group")

    res = pd.DataFrame(assigned)
    res.to_csv('res.csv', index=False, header=names)
    #print(x)
