from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

import numpy as np


@dataclass(frozen=True)
class ProblemData:
    days: List[str]
    times: List[str]
    rooms: List[str]
    groups: List[str]
    subjects: List[str]
    teachers: List[str]
    default_count: int
    subject_count: Dict[str, Dict[str, int]]
    subject_teachers: Dict[str, List[str]]


class ScheduleOptimizer:
    """
    Генетический алгоритм для составления расписания.

    Вход: dict той же структуры, что использует MILP-часть (default_count, groups, ...).
    Выход: таблица (list[list[str]]) в формате, совместимом с MyPulp.get_json_2().
    """

    def __init__(self, raw: Dict) -> None:
        self.problem = self._parse_input(raw)
        self.class_groups_, self.class_subjects_ = self._build_class_list()
        self.allowed_ = self._build_allowed_matrix()
        self.n_classes_ = int(self.class_groups_.shape[0])
        self._allowed_teachers_per_subject = self._build_allowed_teachers_per_subject()

        self.best_chromosome_: Optional[np.ndarray] = None
        self.best_penalty_: Optional[int] = None
        self.history_: List[int] = []

    def _parse_input(self, raw: Dict) -> ProblemData:
        return ProblemData(
            days=list(raw["days"]),
            times=list(raw["times"]),
            rooms=list(raw["rooms"]),
            groups=list(raw["groups"]),
            subjects=list(raw["subjects"]),
            teachers=list(raw["teachers"]),
            default_count=int(raw.get("default_count", 0)),
            subject_count=dict(raw["subject_count"]),
            subject_teachers={k: list(v) for k, v in dict(raw["subject_teachers"]).items()},
        )

    def _build_class_list(self) -> Tuple[np.ndarray, np.ndarray]:
        p = self.problem
        group_index = {g: i for i, g in enumerate(p.groups)}
        subject_index = {s: i for i, s in enumerate(p.subjects)}

        class_groups: List[int] = []
        class_subjects: List[int] = []

        for g_name in p.groups:
            g = group_index[g_name]
            subj_counts = p.subject_count.get(g_name, {})
            for s_name in p.subjects:
                s = subject_index[s_name]
                cnt = subj_counts.get(s_name, p.default_count)
                for _ in range(int(cnt)):
                    class_groups.append(g)
                    class_subjects.append(s)

        return np.array(class_groups, dtype=np.int32), np.array(class_subjects, dtype=np.int32)

    def _build_allowed_matrix(self) -> np.ndarray:
        S = len(self.problem.subjects)
        T = len(self.problem.teachers)
        allowed = np.zeros((S, T), dtype=np.int8)

        teacher_index = {t: i for i, t in enumerate(self.problem.teachers)}
        subject_index = {s: i for i, s in enumerate(self.problem.subjects)}

        for subj, teachers in self.problem.subject_teachers.items():
            s = subject_index[subj]
            for t_name in teachers:
                allowed[s, teacher_index[t_name]] = 1

        return allowed

    def _build_allowed_teachers_per_subject(self) -> List[np.ndarray]:
        result: List[np.ndarray] = []
        for s in range(self.allowed_.shape[0]):
            idx = np.nonzero(self.allowed_[s])[0]
            if idx.size == 0:
                raise ValueError(
                    f"Предмет {self.problem.subjects[s]} не имеет квалифицированных преподавателей"
                )
            result.append(idx)
        return result

    @staticmethod
    def _count_conflicts(slots: np.ndarray) -> int:
        if slots.shape[0] <= 1:
            return 0
        encoded = slots[:, 0] * 16_384 + slots[:, 1]
        _, counts = np.unique(encoded, return_counts=True)
        return int(np.sum(counts[counts > 1] - 1))

    @staticmethod
    def _count_gaps_for_group_on_day(g_times: np.ndarray, g_days: np.ndarray, day: int) -> int:
        mask = g_days == day
        times_on_day = g_times[mask]
        if times_on_day.size <= 1:
            return 0
        sorted_times = np.sort(np.unique(times_on_day))
        gaps = np.diff(sorted_times) - 1
        return int(np.sum(gaps[gaps > 0]))

    def _fitness_penalty(self, chromosome: np.ndarray) -> int:
        days = chromosome[:, 0]
        times = chromosome[:, 1]
        rooms = chromosome[:, 2]
        teachers = chromosome[:, 3]

        D = len(self.problem.days)
        R = len(self.problem.rooms)
        Tch = len(self.problem.teachers)
        G = len(self.problem.groups)

        penalty = 0

        for i in range(chromosome.shape[0]):
            s = int(self.class_subjects_[i])
            t = int(teachers[i])
            if self.allowed_[s, t] == 0:
                penalty += 10_000

        for g in range(G):
            idx = np.nonzero(self.class_groups_ == g)[0]
            g_days = days[idx]
            g_times = times[idx]
            slots = np.stack([g_days, g_times], axis=1)
            penalty += 10_000 * self._count_conflicts(slots)

            for d in range(D):
                penalty += 1 * self._count_gaps_for_group_on_day(g_times, g_days, d)

            unique_days = np.unique(g_days)
            if unique_days.size > 1:
                penalty += 1 * np.max(((unique_days.size - 3), 0))

        for r in range(R):
            idx = np.nonzero(rooms == r)[0]
            if idx.size <= 1:
                continue
            r_days = days[idx]
            r_times = times[idx]
            slots = np.stack([r_days, r_times], axis=1)
            penalty += 10_000 * self._count_conflicts(slots)

        for t in range(Tch):
            idx = np.nonzero(teachers == t)[0]
            if idx.size <= 1:
                continue
            t_days = days[idx]
            t_times = times[idx]
            slots = np.stack([t_days, t_times], axis=1)
            penalty += 10_000 * self._count_conflicts(slots)

            for d in range(D):
                penalty += 1 * self._count_gaps_for_group_on_day(t_times, t_days, d)

        return int(penalty)

    def _evaluate_population(self, population: np.ndarray) -> np.ndarray:
        n = population.shape[0]
        penalties = np.empty(n, dtype=np.int64)
        for i in range(n):
            penalties[i] = self._fitness_penalty(population[i])
        return penalties

    @staticmethod
    def _tournament_selection(rng: np.random.Generator, penalties: np.ndarray, k: int) -> int:
        n = penalties.shape[0]
        idx = rng.integers(0, n, size=k)
        return int(idx[np.argmin(penalties[idx])])

    @staticmethod
    def _crossover(
        rng: np.random.Generator, parent1: np.ndarray, parent2: np.ndarray
    ) -> Tuple[np.ndarray, np.ndarray]:
        n = parent1.shape[0]
        if n < 2:
            return parent1.copy(), parent2.copy()
        point = int(rng.integers(1, n))
        child1 = np.vstack([parent1[:point], parent2[point:]])
        child2 = np.vstack([parent2[:point], parent1[point:]])
        return child1, child2

    def _mutate(self, rng: np.random.Generator, chromosome: np.ndarray, mutation_rate: float) -> None:
        D = len(self.problem.days)
        Tm = len(self.problem.times)
        R = len(self.problem.rooms)

        for i in range(chromosome.shape[0]):
            if rng.random() < mutation_rate:
                s = int(self.class_subjects_[i])
                chromosome[i, 0] = rng.integers(0, D)
                chromosome[i, 1] = rng.integers(0, Tm)
                chromosome[i, 2] = rng.integers(0, R)
                chromosome[i, 3] = rng.choice(self._allowed_teachers_per_subject[s])

    def _local_search(
        self,
        rng: np.random.Generator,
        chromosome: np.ndarray,
        max_attempts: int = 20,
    ) -> np.ndarray:
        """
        Локальный поиск (hill climbing) после мутации.
        Делает до max_attempts проб локальных изменений и принимает только улучшающие.
        """
        if max_attempts <= 0:
            return chromosome

        D = len(self.problem.days)
        Tm = len(self.problem.times)
        R = len(self.problem.rooms)

        current = chromosome.copy()
        current_penalty = self._fitness_penalty(current)

        for _ in range(max_attempts):
            neighbor = current.copy()

            i = int(rng.integers(0, neighbor.shape[0]))
            move_type = int(rng.integers(0, 4))

            if move_type == 0:
                neighbor[i, 0] = rng.integers(0, D)
            elif move_type == 1:
                neighbor[i, 1] = rng.integers(0, Tm)
            elif move_type == 2:
                neighbor[i, 2] = rng.integers(0, R)
            else:
                s = int(self.class_subjects_[i])
                neighbor[i, 3] = rng.choice(self._allowed_teachers_per_subject[s])

            neighbor_penalty = self._fitness_penalty(neighbor)

            if neighbor_penalty < current_penalty:
                current, current_penalty = neighbor, neighbor_penalty

        return current

    def _random_population(self, rng: np.random.Generator, pop_size: int) -> np.ndarray:
        D = len(self.problem.days)
        Tm = len(self.problem.times)
        R = len(self.problem.rooms)
        n = self.n_classes_

        population = np.empty((pop_size, n, 4), dtype=np.int16)

        for i in range(pop_size):
            days = rng.integers(0, D, size=n, dtype=np.int16)
            times = rng.integers(0, Tm, size=n, dtype=np.int16)
            rooms = rng.integers(0, R, size=n, dtype=np.int16)

            teachers = np.empty(n, dtype=np.int16)
            for ci in range(n):
                s = int(self.class_subjects_[ci])
                teachers[ci] = rng.choice(self._allowed_teachers_per_subject[s])

            population[i] = np.stack([days, times, rooms, teachers], axis=1)

        return population

    def fit(
        self,
        *,
        pop_size: int = 200,
        generations: int = 2000,
        crossover_rate: float = 0.7,
        mutation_rate: float = 0.02,
        elite_size: int = 20,
        tournament_size: int = 7,
        local_search_rate: float = 0.3,
        local_search_attempts: int = 15,
        random_seed: Optional[int] = None,
        verbose: bool = True,
    ) -> "ScheduleOptimizer":
        if pop_size < 2:
            raise ValueError("pop_size must be >= 2")
        if not (0.0 <= crossover_rate <= 1.0):
            raise ValueError("crossover_rate must be in [0, 1]")
        if not (0.0 <= mutation_rate <= 1.0):
            raise ValueError("mutation_rate must be in [0, 1]")
        elite_size = int(max(0, min(elite_size, pop_size)))
        tournament_size = int(max(2, tournament_size))
        local_search_attempts = int(max(0, local_search_attempts))

        rng = np.random.default_rng(random_seed)
        population = self._random_population(rng, pop_size)

        best_chromosome = None
        best_penalty = None
        history: List[int] = []

        for gen in range(int(generations)):
            penalties = self._evaluate_population(population)

            gen_best_idx = int(np.argmin(penalties))
            gen_best_penalty = int(penalties[gen_best_idx])
            history.append(gen_best_penalty)

            print(f"{gen} - {gen_best_penalty}")

            # if verbose:
            #     print(f"{gen} - {gen_best_penalty}")

            if best_penalty is None or gen_best_penalty < best_penalty:
                best_penalty = gen_best_penalty
                best_chromosome = population[gen_best_idx].copy()

            if best_penalty == 0:
                break

            order = np.argsort(penalties)
            new_population = np.empty_like(population)
            if elite_size > 0:
                new_population[:elite_size] = population[order[:elite_size]]

            cur = elite_size
            while cur < pop_size:
                p1 = self._tournament_selection(rng, penalties, tournament_size)
                p2 = self._tournament_selection(rng, penalties, tournament_size)
                parent1 = population[p1]
                parent2 = population[p2]

                if rng.random() < crossover_rate:
                    child1, child2 = self._crossover(rng, parent1, parent2)
                else:
                    child1, child2 = parent1.copy(), parent2.copy()

                self._mutate(rng, child1, mutation_rate)
                self._mutate(rng, child2, mutation_rate)

                if local_search_rate > 0.0 and local_search_attempts > 0:
                    if rng.random() < local_search_rate:
                        child1 = self._local_search(rng, child1, max_attempts=local_search_attempts)
                    if rng.random() < local_search_rate:
                        child2 = self._local_search(rng, child2, max_attempts=local_search_attempts)

                new_population[cur] = child1
                cur += 1
                if cur < pop_size:
                    new_population[cur] = child2
                    cur += 1

            population = new_population

        self.best_chromosome_ = best_chromosome
        self.best_penalty_ = best_penalty
        self.history_ = history
        return self

    def to_table(self) -> List[List[str]]:
        if self.best_chromosome_ is None:
            raise RuntimeError("Сначала вызовите fit()")

        header = ["group", "day", "time", "subject", "room", "teacher"]
        rows: List[List[str]] = [header]

        p = self.problem
        for i in range(self.best_chromosome_.shape[0]):
            g_idx = int(self.class_groups_[i])
            s_idx = int(self.class_subjects_[i])
            day_idx, time_idx, room_idx, teacher_idx = map(int, self.best_chromosome_[i])
            rows.append(
                [
                    p.groups[g_idx],
                    p.days[day_idx],
                    p.times[time_idx],
                    p.subjects[s_idx],
                    p.rooms[room_idx],
                    p.teachers[teacher_idx],
                ]
            )

        day_index = {d: i for i, d in enumerate(p.days)}
        time_index = {t: i for i, t in enumerate(p.times)}
        rows[1:] = sorted(rows[1:], key=lambda r: (r[0], day_index[r[1]], time_index[r[2]]))
        return rows

