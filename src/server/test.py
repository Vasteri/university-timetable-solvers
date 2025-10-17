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