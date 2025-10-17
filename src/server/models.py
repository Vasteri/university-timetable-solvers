from sqlalchemy import Column, Integer, String, ForeignKey, Table, MetaData
from sqlalchemy.orm import relationship, declarative_base

Base = declarative_base()

# many-to-many
subject_teacher = Table(
    "subject_teacher",
    Base.metadata,
    Column("subject_id", ForeignKey("subjects.id"), primary_key=True),
    Column("teacher_id", ForeignKey("teachers.id"), primary_key=True),
)

class Group(Base):
    __tablename__ = "groups"
    id = Column(Integer, primary_key=True)
    name = Column(String, unique=True, nullable=False)

class Subject(Base):
    __tablename__ = "subjects"
    id = Column(Integer, primary_key=True)
    name = Column(String, unique=True, nullable=False)
    teachers = relationship("Teacher", secondary=subject_teacher, back_populates="subjects")

class Teacher(Base):
    __tablename__ = "teachers"
    id = Column(Integer, primary_key=True)
    name = Column(String, unique=True, nullable=False)
    subjects = relationship("Subject", secondary=subject_teacher, back_populates="teachers")

class Room(Base):
    __tablename__ = "rooms"
    id = Column(Integer, primary_key=True)
    name = Column(String, unique=True, nullable=False)

class Day(Base):
    __tablename__ = "days"
    id = Column(Integer, primary_key=True)
    name = Column(String, unique=True, nullable=False)

class TimeSlot(Base):
    __tablename__ = "times"
    id = Column(Integer, primary_key=True)
    time = Column(String, unique=True, nullable=False)


all_models = [Group, Subject, Teacher, Room, Day, TimeSlot]