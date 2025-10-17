from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, Session
from models import *
from contextlib import contextmanager

#engine_memory = create_engine("sqlite:///:memory:", echo=True)
#engine_file = create_engine("sqlite:///my_database.db", echo=True)

class DataBase:
    def __init__(self, URL_ENGINE = "sqlite:///mydb.db"):
        self.URL_ENGINE = URL_ENGINE
        self.engine = create_engine(self.URL_ENGINE, echo=False)
        self.localsession = sessionmaker(bind=self.engine)

    @contextmanager
    def get_session(self) -> Session:
        session = self.localsession()
        try:
            yield session
            session.commit()
        except Exception as e:
            session.rollback()
            raise e
        finally:
            session.close()

    def create_tables(self):
        Base.metadata.create_all(bind=self.engine)
        print("Таблицы успешно созданы")

    def drop_tables(self):
        Base.metadata.drop_all(bind=self.engine)
        print("Таблицы удалены")

    def reset_database(self):
        self.drop_tables()
        self.create_tables()
        print("База пересоздана")

    def clear_data(self):
        with self.get_session() as session:
            for model in all_models:
                session.query(model).delete()
            print("Таблицы очищены")

    # --- Инициализация ---
    def init_data(self):
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
            "IT": ["T5", "T6", "T7"],
        }

        with self.get_session() as session:
            for g in groups:
                session.merge(Group(name=g))
            for s in subjects:
                session.merge(Subject(name=s))
            for t in teachers:
                session.merge(Teacher(name=t))
            for r in rooms:
                session.merge(Room(name=r))
            for d in days:
                session.merge(Day(name=d))
            for tm in times:
                session.merge(TimeSlot(time=tm))
            session.commit()

            # Связываем предметы с преподавателями
            for subj_name, teacher_names in subject_teachers.items():
                subject = session.query(Subject).filter_by(name=subj_name).first()
                for tname in teacher_names:
                    teacher = session.query(Teacher).filter_by(name=tname).first()
                    if teacher not in subject.teachers:
                        subject.teachers.append(teacher)
            session.commit()
            print("Данные и связи добавлены")
    
    def get_groups(self):
        with self.get_session() as session:
            return [g.name for g in session.query(Group).all()]

    def get_subjects(self):
        with self.get_session() as session:
            return [s.name for s in session.query(Subject).all()]

    def get_teachers(self):
        with self.get_session() as session:
            return [t.name for t in session.query(Teacher).all()]

    def get_rooms(self):
        with self.get_session() as session:
            return [r.name for r in session.query(Room).all()]

    def get_days(self):
        with self.get_session() as session:
            return [d.name for d in session.query(Day).all()]

    def get_times(self):
        with self.get_session() as session:
            return [t.time for t in session.query(TimeSlot).all()]

    def get_subject_teachers(self):
        """Возвращает словарь вида {'math': ['T1', 'T2'], ...}"""
        result = {}
        with self.get_session() as session:
            subjects = session.query(Subject).all()
            for subj in subjects:
                result[subj.name] = [t.name for t in subj.teachers]
        return result
