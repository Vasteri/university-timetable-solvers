from __future__ import annotations

from typing import Dict, List, Optional, Set, Tuple, Union

import numpy as np

from schemas import InputData


class ScheduleOptimizer:
    """
    Генетический алгоритм для составления расписания.

    Вход: dict той же структуры, что использует MILP-часть (default_count, groups, ...).
    Выход: таблица (list[list[str]]) в формате, совместимом с MyPulp.get_json_2().
    """

    def __init__(self, raw: Union[InputData, Dict]) -> None:
        self.problem = self._parse_input(raw)
        self.class_groups_, self.class_subjects_ = self._build_class_list()
        self.allowed_ = self._build_allowed_matrix()
        self.n_classes_ = int(self.class_groups_.shape[0])
        self._allowed_teachers_per_subject = self._build_allowed_teachers_per_subject()
        self._classes_by_group = self._build_classes_by_group()
        self._n_day_time_slots = len(self.problem.days) * len(self.problem.times)
        self._all_slot_ids = np.arange(self._n_day_time_slots, dtype=np.int32)
        self._assert_group_slot_capacity()

        self.best_chromosome_: Optional[np.ndarray] = None
        self.best_penalty_: Optional[int] = None
        self.history_: List[int] = []

    def _parse_input(self, raw: Union[InputData, Dict]) -> InputData:
        if isinstance(raw, InputData):
            return raw
        return InputData(**raw)

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

    def _build_classes_by_group(self) -> List[List[int]]:
        G = len(self.problem.groups)
        buckets: List[List[int]] = [[] for _ in range(G)]
        for i in range(self.n_classes_):
            buckets[int(self.class_groups_[i])].append(i)
        return buckets

    def _assert_group_slot_capacity(self) -> None:
        """Уникальные (день, время) в группе: нужно не больше слотов D×Tm."""
        for g, idxs in enumerate(self._classes_by_group):
            if len(idxs) > self._n_day_time_slots:
                raise ValueError(
                    f"Группа «{self.problem.groups[g]}»: занятий {len(idxs)}, "
                    f"а уникальных слотов день×время только {self._n_day_time_slots}"
                )

    def _build_feasible_chromosome(
        self, rng: np.random.Generator, max_restarts: int = 50_000
    ) -> np.ndarray:
        """
        Случайная хромосома, удовлетворяющая жёстким ограничениям:
        учитель ∈ допустимых для предмета; в группе нет двух пар в один слот;
        аудитория и преподаватель не заняты в этот слот (глобально по особи).
        """
        Tm = len(self.problem.times)
        R = len(self.problem.rooms)
        Tch = len(self.problem.teachers)
        n = self.n_classes_
        slot_mult = Tm
        cbg = self._classes_by_group

        for _ in range(max_restarts):
            chrom = np.zeros((n, 4), dtype=np.int16)
            room_at_slot: Dict[int, Set[int]] = {}
            teacher_busy: List[Set[int]] = [set() for _ in range(Tch)]

            groups_order = np.arange(len(cbg), dtype=np.int32)
            rng.shuffle(groups_order)

            failed = False
            for g in groups_order:
                indices = cbg[int(g)]
                if not indices:
                    continue
                indices = indices[:]
                rng.shuffle(indices)

                available = self._all_slot_ids.copy()
                rng.shuffle(available)
                available_list = available.tolist()

                for idx in indices:
                    s = int(self.class_subjects_[idx])
                    allowed_arr = self._allowed_teachers_per_subject[s]

                    placed = False
                    rng.shuffle(available_list)

                    for sid in available_list:
                        sid = int(sid)
                        d = sid // slot_mult
                        t = sid % slot_mult

                        rooms_taken = room_at_slot.get(sid)
                        if rooms_taken is None:
                            r = int(rng.integers(0, R))
                        else:
                            if len(rooms_taken) >= R:
                                continue
                            cand_r = [rr for rr in range(R) if rr not in rooms_taken]
                            r = int(rng.choice(cand_r))

                        teach_candidates: List[int] = []
                        for te in allowed_arr:
                            te_i = int(te)
                            if sid not in teacher_busy[te_i]:
                                teach_candidates.append(te_i)
                        if not teach_candidates:
                            continue

                        teach = int(rng.choice(teach_candidates))

                        chrom[idx, 0] = d
                        chrom[idx, 1] = t
                        chrom[idx, 2] = r
                        chrom[idx, 3] = teach

                        if rooms_taken is None:
                            room_at_slot[sid] = {r}
                        else:
                            rooms_taken.add(r)
                        teacher_busy[teach].add(sid)
                        available_list.remove(sid)
                        placed = True
                        break

                    if not placed:
                        failed = True
                        break
                if failed:
                    break

            if not failed:
                return chrom

        raise RuntimeError(
            "Не удалось за отведённое число попыток собрать особь без нарушений жёстких ограничений. "
            "Проверьте данные: слоты день×время, число аудиторий и загрузку преподавателей."
        )

    @staticmethod
    def _slot_id(day: int, time_idx: int, n_times: int) -> int:
        return int(day) * int(n_times) + int(time_idx)

    def _pick_feasible_tuple_at_slot(
        self,
        rng: np.random.Generator,
        chromosome: np.ndarray,
        sid: int,
        subject_idx: int,
        skip: Set[int],
        n_times: int,
        n_rooms: int,
    ) -> Optional[Tuple[int, int, int, int]]:
        """Случайный допустимый (день, время, аудитория, учитель) для предмета в слоте sid; skip — индексы не учитывать."""
        d = sid // n_times
        tm = sid % n_times
        taken_r: Set[int] = set()
        busy_te: Set[int] = set()
        for k in range(chromosome.shape[0]):
            if k in skip:
                continue
            if self._slot_id(int(chromosome[k, 0]), int(chromosome[k, 1]), n_times) == sid:
                taken_r.add(int(chromosome[k, 2]))
                busy_te.add(int(chromosome[k, 3]))
        free_r = [rr for rr in range(n_rooms) if rr not in taken_r]
        allowed_arr = self._allowed_teachers_per_subject[subject_idx]
        teach_cand = [int(te) for te in allowed_arr if int(te) not in busy_te]
        if not free_r or not teach_cand:
            return None
        return (d, tm, int(rng.choice(free_r)), int(rng.choice(teach_cand)))

    def _try_swap_with_group_mate(
        self, rng: np.random.Generator, chromosome: np.ndarray, i: int, g: int
    ) -> bool:
        """Обмен слотами с другим занятием группы + новые ауд./уч. под каждый предмет (не просто swap строк)."""
        Tm = len(self.problem.times)
        R = len(self.problem.rooms)
        siblings = [j for j in self._classes_by_group[g] if j != i]
        if not siblings:
            return False
        j = int(rng.choice(siblings))
        sid_i = self._slot_id(int(chromosome[i, 0]), int(chromosome[i, 1]), Tm)
        sid_j = self._slot_id(int(chromosome[j, 0]), int(chromosome[j, 1]), Tm)
        if sid_i == sid_j:
            return False
        si = int(self.class_subjects_[i])
        sj = int(self.class_subjects_[j])
        skip = {i, j}
        tup_i = self._pick_feasible_tuple_at_slot(rng, chromosome, sid_j, si, skip, Tm, R)
        if tup_i is None:
            return False
        tup_j = self._pick_feasible_tuple_at_slot(rng, chromosome, sid_i, sj, skip, Tm, R)
        if tup_j is None:
            return False
        chromosome[i, 0], chromosome[i, 1], chromosome[i, 2], chromosome[i, 3] = tup_i
        chromosome[j, 0], chromosome[j, 1], chromosome[j, 2], chromosome[j, 3] = tup_j
        return True

    def _try_mutate_room_same_slot(
        self, rng: np.random.Generator, chromosome: np.ndarray, i: int, n_times: int, n_rooms: int
    ) -> bool:
        d = int(chromosome[i, 0])
        t = int(chromosome[i, 1])
        r_cur = int(chromosome[i, 2])
        sid = self._slot_id(d, t, n_times)
        taken: Set[int] = set()
        for k in range(chromosome.shape[0]):
            if k == i:
                continue
            if self._slot_id(int(chromosome[k, 0]), int(chromosome[k, 1]), n_times) == sid:
                taken.add(int(chromosome[k, 2]))
        alternatives = [rr for rr in range(n_rooms) if rr not in taken and rr != r_cur]
        if not alternatives:
            return False
        chromosome[i, 2] = int(rng.choice(alternatives))
        return True

    def _try_mutate_teacher_same_slot(
        self, rng: np.random.Generator, chromosome: np.ndarray, i: int, n_times: int
    ) -> bool:
        d = int(chromosome[i, 0])
        t = int(chromosome[i, 1])
        te_cur = int(chromosome[i, 3])
        sid = self._slot_id(d, t, n_times)
        busy_te: Set[int] = set()
        for k in range(chromosome.shape[0]):
            if k == i:
                continue
            if self._slot_id(int(chromosome[k, 0]), int(chromosome[k, 1]), n_times) == sid:
                busy_te.add(int(chromosome[k, 3]))
        s = int(self.class_subjects_[i])
        allowed_arr = self._allowed_teachers_per_subject[s]
        candidates = [
            int(te) for te in allowed_arr if int(te) not in busy_te and int(te) != te_cur
        ]
        if not candidates:
            return False
        chromosome[i, 3] = int(rng.choice(candidates))
        return True

    def _try_reslot_gene(
        self,
        rng: np.random.Generator,
        chromosome: np.ndarray,
        i: int,
        g: int,
        n_times: int,
        n_rooms: int,
    ) -> bool:
        """Другой слот группы (день×время), свободный для остальных членов группы + комната и учитель."""
        sid_old = self._slot_id(int(chromosome[i, 0]), int(chromosome[i, 1]), n_times)
        used_by_others: Set[int] = set()
        for j in self._classes_by_group[g]:
            if j == i:
                continue
            used_by_others.add(
                self._slot_id(int(chromosome[j, 0]), int(chromosome[j, 1]), n_times)
            )
        candidates = [
            int(sid)
            for sid in self._all_slot_ids
            if int(sid) not in used_by_others and int(sid) != sid_old
        ]
        rng.shuffle(candidates)
        s = int(self.class_subjects_[i])
        skip = {i}
        for sid in candidates:
            picked = self._pick_feasible_tuple_at_slot(
                rng, chromosome, sid, s, skip, n_times, n_rooms
            )
            if picked is not None:
                chromosome[i, 0], chromosome[i, 1], chromosome[i, 2], chromosome[i, 3] = picked
                return True
        return False

    def _mutate_one_gene_feasible(self, rng: np.random.Generator, chromosome: np.ndarray, i: int) -> bool:
        """Одно случайное изменение гена с сохранением жёстких ограничений; True если изменили."""
        g = int(self.class_groups_[i])
        Tm = len(self.problem.times)
        R = len(self.problem.rooms)
        ops = ["swap", "room", "teacher", "reslot"]
        rng.shuffle(ops)
        for op in ops:
            if op == "swap" and self._try_swap_with_group_mate(rng, chromosome, i, g):
                return True
            if op == "room" and self._try_mutate_room_same_slot(rng, chromosome, i, Tm, R):
                return True
            if op == "teacher" and self._try_mutate_teacher_same_slot(rng, chromosome, i, Tm):
                return True
            if op == "reslot" and self._try_reslot_gene(rng, chromosome, i, g, Tm, R):
                return True
        return False

    @staticmethod
    def _count_conflicts(slots: np.ndarray, n_times: int) -> int:
        if slots.shape[0] <= 1:
            return 0
        # int16 в хромосоме: day * 16384 + time даёт переполнение при day >= 2 → ложные конфликты
        d = slots[:, 0].astype(np.int64, copy=False)
        tm = slots[:, 1].astype(np.int64, copy=False)
        encoded = d * int(n_times) + tm
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

    def _hard_penalty_value(self, chromosome: np.ndarray) -> int:
        """Только жёсткие нарушения (множитель 10_000). Быстрая проверка после кроссовера."""
        days = chromosome[:, 0]
        times = chromosome[:, 1]
        rooms = chromosome[:, 2]
        teachers = chromosome[:, 3]

        Tm = len(self.problem.times)
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
            penalty += 10_000 * self._count_conflicts(slots, Tm)

        for r in range(R):
            idx = np.nonzero(rooms == r)[0]
            if idx.size <= 1:
                continue
            r_days = days[idx]
            r_times = times[idx]
            slots = np.stack([r_days, r_times], axis=1)
            penalty += 10_000 * self._count_conflicts(slots, Tm)

        for t in range(Tch):
            idx = np.nonzero(teachers == t)[0]
            if idx.size <= 1:
                continue
            t_days = days[idx]
            t_times = times[idx]
            slots = np.stack([t_days, t_times], axis=1)
            penalty += 10_000 * self._count_conflicts(slots, Tm)

        return int(penalty)

    def _soft_penalty_value(self, chromosome: np.ndarray) -> int:
        days = chromosome[:, 0]
        times = chromosome[:, 1]
        teachers = chromosome[:, 3]

        D = len(self.problem.days)
        Tch = len(self.problem.teachers)
        G = len(self.problem.groups)

        penalty = 0

        for g in range(G):
            idx = np.nonzero(self.class_groups_ == g)[0]
            g_days = days[idx]
            g_times = times[idx]

            for d in range(D):
                penalty += 1 * self._count_gaps_for_group_on_day(g_times, g_days, d)

            unique_days = np.unique(g_days)
            if unique_days.size > 1:
                penalty += 1 * np.max(((unique_days.size - 3), 0))

        for t in range(Tch):
            idx = np.nonzero(teachers == t)[0]
            if idx.size <= 1:
                continue
            t_days = days[idx]
            t_times = times[idx]
            for d in range(D):
                penalty += 1 * self._count_gaps_for_group_on_day(t_times, t_days, d)

        return int(penalty)

    def _fitness_penalty(self, chromosome: np.ndarray) -> int:
        return int(self._hard_penalty_value(chromosome) + self._soft_penalty_value(chromosome))

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

    def _repair_crossover_quick(
        self,
        rng: np.random.Generator,
        chromosome: np.ndarray,
        focus_indices: np.ndarray,
        max_attempts: int,
    ) -> None:
        """Несколько допустимых мутаций; чаще трогаем гены из подмешанных групп."""
        n = chromosome.shape[0]
        if focus_indices.size == 0:
            focus_indices = np.arange(n, dtype=np.int32)
        for _ in range(max_attempts):
            if self._hard_penalty_value(chromosome) == 0:
                return
            if rng.random() < 0.7 and focus_indices.size > 0:
                i = int(rng.choice(focus_indices))
            else:
                i = int(rng.integers(0, n))
            self._mutate_one_gene_feasible(rng, chromosome, i)

    def _crossover_feasible_light(
        self,
        rng: np.random.Generator,
        parent1: np.ndarray,
        parent2: np.ndarray,
        *,
        max_repair_attempts: int,
    ) -> Tuple[np.ndarray, np.ndarray]:
        """
        Лёгкий кроссовер: комплементарный обмен несколькими целыми группами,
        короткий repair, при неудаче — копия родителя (всегда допустимо).
        """
        n = parent1.shape[0]
        if n < 2:
            return parent1.copy(), parent2.copy()

        Gn = len(self._classes_by_group)
        if Gn < 1:
            return parent1.copy(), parent2.copy()

        k = int(rng.integers(1, max(2, min(4, Gn + 1))))
        k = min(k, Gn)
        groups_pick = rng.choice(Gn, size=k, replace=False)

        focus: List[int] = []
        for g in groups_pick:
            focus.extend(self._classes_by_group[int(g)])
        focus_arr = np.array(focus, dtype=np.int32) if focus else np.arange(n, dtype=np.int32)

        child1 = parent1.copy()
        child2 = parent2.copy()
        for g in groups_pick:
            gg = int(g)
            for idx in self._classes_by_group[gg]:
                child1[idx] = parent2[idx]
                child2[idx] = parent1[idx]

        self._repair_crossover_quick(rng, child1, focus_arr, max_repair_attempts)
        self._repair_crossover_quick(rng, child2, focus_arr, max_repair_attempts)

        if self._hard_penalty_value(child1) > 0:
            child1 = parent1.copy()
        if self._hard_penalty_value(child2) > 0:
            child2 = parent2.copy()
        return child1, child2

    def _mutate(self, rng: np.random.Generator, chromosome: np.ndarray, mutation_rate: float) -> None:
        """Сохраняет жёсткие ограничения: обмен в группе, другая ауд./уч. в том же слоте, перенос слота."""
        for i in range(chromosome.shape[0]):
            if rng.random() < mutation_rate:
                self._mutate_one_gene_feasible(rng, chromosome, i)

    def _local_search(
        self,
        rng: np.random.Generator,
        chromosome: np.ndarray,
        max_attempts: int = 20,
    ) -> np.ndarray:
        """
        Локальный поиск (hill climbing) после мутации.
        Соседи только из допустимых ходов (как в _mutate_one_gene_feasible).
        """
        if max_attempts <= 0:
            return chromosome

        n = chromosome.shape[0]
        current = chromosome.copy()
        current_penalty = self._fitness_penalty(current)

        for _ in range(max_attempts):
            neighbor = current.copy()
            i = int(rng.integers(0, n))
            if not self._mutate_one_gene_feasible(rng, neighbor, i):
                continue

            neighbor_penalty = self._fitness_penalty(neighbor)

            if neighbor_penalty < current_penalty:
                current, current_penalty = neighbor, neighbor_penalty

        return current

    def _feasible_population(
        self,
        rng: np.random.Generator,
        pop_size: int,
        *,
        max_restarts_per_individual: int = 10_000,
    ) -> np.ndarray:
        n = self.n_classes_
        population = np.empty((pop_size, n, 4), dtype=np.int16)
        for i in range(pop_size):
            population[i] = self._build_feasible_chromosome(
                rng, max_restarts=max_restarts_per_individual
            )
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
        feasible_init_max_restarts: int = 10_000,
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
        feasible_init_max_restarts = int(max(1, feasible_init_max_restarts))

        rng = np.random.default_rng(random_seed)
        population = self._feasible_population(
            rng, pop_size, max_restarts_per_individual=feasible_init_max_restarts
        )

        best_chromosome = None
        best_penalty = None
        history: List[int] = []

        for gen in range(int(generations)):
            penalties = self._evaluate_population(population)

            gen_best_idx = int(np.argmin(penalties))
            gen_worst_idx = int(np.argmax(penalties))
            gen_best_penalty = int(penalties[gen_best_idx])
            gen_worst_penalty = int(penalties[gen_worst_idx])
            history.append(gen_best_penalty)

            print(f"{gen} - {gen_best_penalty} - {gen_worst_penalty}")

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