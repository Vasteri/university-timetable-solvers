from pulp import LpProblem, LpMinimize, LpVariable, lpSum, LpStatus, PULP_CBC_CMD
from collections import defaultdict
#from pandas import DataFrame


class MyPulp:
    def __init__(self, json_data=None):
        if not(json_data is None):
            self.set_json_values(json_data)
        else:
            print("[WARMING]: DEFAULT VALUES IN PULP")
            self.set_default_values()
        self.names = ["groups", "subjects", "days", "times", "rooms", "teachers"]
        self.assigned = list()
        self.problem = LpProblem("University_Timetable", LpMinimize)
        self._init_variables()
        self._init_objective_function()
        self._init_constraints()

    def set_json_values(self, json_data):
        self.subject_count = json_data.subject_count
        self.default_count = json_data.default_count

        self.groups = json_data.groups
        self.subjects = json_data.subjects
        self.teachers = json_data.teachers
        self.rooms = json_data.rooms
        self.days = json_data.days
        self.times = json_data.times

        self.subject_teachers = json_data.subject_teachers
        self.teacher_subjects = defaultdict(list)

        for subject, teachers in json_data.subject_teachers.items():
            for teacher in teachers:
                self.teacher_subjects[teacher].append(subject)

        self.teacher_subjects = dict(self.teacher_subjects)

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
    
    def get_json_2(self):
        schedule = [["group", "day", "time", "subject", "room", "teacher"]]
        schedule += [
            [ g, d, t, s, r, tea ]
            for (g, s, d, t, r, tea) in self.assigned
        ]
        return schedule

    def _init_objective_function(self):
        WINDOW_PENALTY = 10
        self.problem += (
            WINDOW_PENALTY * lpSum(self.idle.values())
          + WINDOW_PENALTY * lpSum(self.idle_t.values())
        )


    def set_default_values(self):
        self.subject_count = {
            "A-18-30": {"math": 2},
            "A-16-30": {"math": 1},
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

    def _init_constraints(self):
        # 1) Для всех (g, s) задать нужное количество занятий в неделю
        for g in self.groups:
            for s in self.subjects:
                count = self.subject_count.get(g, dict()).get(s, self.default_count)
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
        
        ###### 5) ОКНА ДЛЯ ГРУПП
        # есть ли занятие в (группа, день, время)
        for g in self.groups:
            for d in self.days:
                for tt in self.times:
                    self.problem += (
                        lpSum(
                            self.x[(g,s,d,tt,r,tea)]
                            for s in self.subjects
                            for r in self.rooms
                            for tea in self.subject_teachers[s]
                        )
                        == self.y[(g,d,tt)]
                    )
 
        T = len(self.times)

        for g in self.groups:
            for d in self.days:
                for i in range(T):
                    # has_left = 1 если хотя бы одно занятие слева
                    if i > 0:
                        self.problem += self.has_left[(g,d,i)] <= lpSum(self.y[(g,d,self.times[j])] for j in range(0, i))
                        for j in range(0, i):
                            self.problem += self.has_left[(g,d,i)] >= self.y[(g,d,self.times[j])]
                    else:
                        self.problem += self.has_left[(g,d,i)] == 0

                    # has_right = 1 если хотя бы одно занятие справа
                    if i < T-1:
                        self.problem += self.has_right[(g,d,i)] <= lpSum(self.y[(g,d,self.times[j])] for j in range(i+1, T))
                        for j in range(i+1, T):
                            self.problem += self.has_right[(g,d,i)] >= self.y[(g,d,self.times[j])]
                    else:
                        self.problem += self.has_right[(g,d,i)] == 0

        # если есть занятия слева и справа, но в этот момент нет занятия, то это окно
        for g in self.groups:
            for d in self.days:
                for i in range(1, T-1):
                    tt = self.times[i]

                    self.problem += (
                        self.idle[(g,d,tt)]
                        >= self.has_left[(g,d,i)]
                        + self.has_right[(g,d,i)]
                        - 1
                        - self.y[(g,d,tt)]
                    )
        
        ###### 6) ОКНА ДЛЯ ПРЕПОДАВАТЕЛЕЙ
        # есть ли занятие в (преподавателей, день, время)
        for t in self.teachers:
            for d in self.days:
                for tt in self.times:
                    self.problem += (
                        lpSum(
                            self.x[(g,s,d,tt,r,t)]
                            for g in self.groups
                            for r in self.rooms
                            for s in self.teacher_subjects[t]
                        )
                        == self.y_t[(t,d,tt)]
                    )    
        
        T = len(self.times)

        for t in self.teachers:
            for d in self.days:
                for i in range(T):
                    # has_left = 1 если хотя бы одно занятие слева
                    if i > 0:
                        self.problem += self.has_left_t[(t,d,i)] <= lpSum(self.y_t[(t,d,self.times[j])] for j in range(0, i))
                        for j in range(0, i):
                            self.problem += self.has_left_t[(t,d,i)] >= self.y_t[(t,d,self.times[j])]
                    else:
                        self.problem += self.has_left_t[(t,d,i)] == 0
                    # has_right = 1 если хотя бы одно занятие справа
                    if i < T-1:
                        self.problem += self.has_right_t[(t,d,i)] <= lpSum(self.y_t[(t,d,self.times[j])] for j in range(i+1, T))
                        for j in range(i+1, T):
                            self.problem += self.has_right_t[(t,d,i)] >= self.y_t[(t,d,self.times[j])]
                    else:
                        self.problem += self.has_right_t[(t,d,i)] == 0

        # если есть занятия слева и справа, но в этот момент нет занятия, то это окно
        for t in self.teachers:
            for d in self.days:
                for i in range(1, T-1):
                    tt = self.times[i]

                    self.problem += (
                        self.idle_t[(t,d,tt)]
                        >= self.has_left_t[(t,d,i)]
                        + self.has_right_t[(t,d,i)]
                        - 1
                        - self.y_t[(t,d,tt)]
                    )


    def _init_variables(self):
        # пары (группа, предмет, день, время, аудитория, преподаватель) - есть ли занятие
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
        
        
        # пары (группа, день, время) - есть ли занятие
        self.y = {}
        for g in self.groups:
            for d in self.days:
                for tt in self.times:
                    self.y[(g,d,tt)] = LpVariable(f"y_{g}_{d}_{tt}", cat="Binary")
        
        # окна между парами для групп (является ли окном)
        self.idle = {}
        for g in self.groups:
            for d in self.days:
                for tt in self.times:
                    self.idle[(g,d,tt)] = LpVariable(f"idle_{g}_{d}_{tt}", cat="Binary")
        
        # наличие занятий до и после для текущего дня в данное время для групп
        self.has_left = {}
        self.has_right = {}

        for g in self.groups:
            for d in self.days:
                for i in range(len(self.times)):
                    self.has_left[(g,d,i)]  = LpVariable(f"has_left_{g}_{d}_{i}",  cat="Binary")
                    self.has_right[(g,d,i)] = LpVariable(f"has_right_{g}_{d}_{i}", cat="Binary")

        # пары (преподавателей, день, время) - есть ли занятие
        self.y_t = {}
        for t in self.teachers:
            for d in self.days:
                for tt in self.times:
                    self.y_t[(t,d,tt)] = LpVariable(f"y_t_{t}_{d}_{tt}", cat="Binary")
        
        # окна между парами для преподавателей(является ли окном)
        self.idle_t = {}
        for t in self.teachers:
            for d in self.days:
                for tt in self.times:
                    self.idle_t[(t,d,tt)] = LpVariable(f"idle_t_{t}_{d}_{tt}", cat="Binary")
        
        # наличие занятий до и после для текущего дня в данное время для преподавателей
        self.has_left_t = {}
        self.has_right_t = {}

        for t in self.teachers:
            for d in self.days:
                for i in range(len(self.times)):
                    self.has_left_t[(t,d,i)]  = LpVariable(f"has_left_t_{t}_{d}_{i}",  cat="Binary")
                    self.has_right_t[(t,d,i)] = LpVariable(f"has_right_t_{t}_{d}_{i}", cat="Binary")

    def solve(self):
        print("Solving...")
        status = self.problem.solve(PULP_CBC_CMD(msg=True))
        print("Status:", LpStatus[self.problem.status])

        self.assigned = []
        for key, var in self.x.items():
            if var.varValue is not None and var.varValue > 0.5:
                g, s, d, tt, r, tea = key
                self.assigned.append((g, s, d, tt, r, tea))

    """
    def save_csv(self):
        res = DataFrame(self.assigned)
        res.to_csv('res.csv', index=False, header=self.names)
    """

    def print_res(self):
        from collections import defaultdict
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
