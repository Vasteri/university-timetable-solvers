## Кодировка (как данные превращаются в "гены")

```mermaid
flowchart LR
  %% =========================
  %% INPUT -> ENCODING OBJECTS
  %% =========================

  subgraph IN["InputData (вход JSON/схема)"]
    Days["days: List[str]"]
    Times["times: List[str]"]
    Rooms["rooms: List[str]"]
    Groups["groups: List[str]"]
    Subjects["subjects: List[str]"]
    Teachers["teachers: List[str]"]
    DefaultCount["default_count: int"]
    SubjectCount["subject_count: Dict[group][subject] -> int"]
    SubjectTeachers["subject_teachers: Dict[subject] -> List[teacher]"]
  end

  subgraph ENC["Кодировка в индексные структуры (numpy)"]
    CG["class_groups_: int32[n_classes]<br/>каждый элемент = индекс группы g"]
    CS["class_subjects_: int32[n_classes]<br/>каждый элемент = индекс предмета s"]
    AL["allowed_: int8[S x T]<br/>allowed_[s,t]=1 если teacher t может вести subject s"]
    ATS["_allowed_teachers_per_subject: List[np.ndarray]<br/>для каждого s: массив допустимых t"]
    CBG["_classes_by_group: List[List[int]]<br/>для каждой группы g: список индексов i классов этой группы"]
    SLOT["_n_day_time_slots = D*Tm<br/>_all_slot_ids = 0..(D*Tm-1)"]
  end

  subgraph GENE["Ген (строка хромосомы)"]
    Gene["chromosome[i] = [day_idx, time_idx, room_idx, teacher_idx]<br/>int16"]
  end

  IN -->|_build_class_list| CG
  IN -->|_build_class_list| CS
  IN -->|_build_allowed_matrix| AL
  AL -->|_build_allowed_teachers_per_subject| ATS
  CG -->|_build_classes_by_group| CBG
  Days --> SLOT
  Times --> SLOT

  CS -->|subject of class i| Gene
  CG -->|group of class i| Gene
  ATS -->|teacher chosen from allowed for subject| Gene
  Rooms -->|room_idx| Gene
  Days -->|day_idx| Gene
  Times -->|time_idx| Gene
```


## Создание начальной популяции

```mermaid
flowchart TB
  %% ==========================================
  %% FEASIBLE INIT: HOW ONE CHROMOSOME IS BUILT
  %% ==========================================

  subgraph Fixed["Фиксированные структуры кодировки"]
    CBG["_classes_by_group[g] = список i"]
    CS["class_subjects_[i] = s"]
    ATS["_allowed_teachers_per_subject[s] = {t...}"]
    AllSlots["_all_slot_ids = 0..(D*Tm-1)"]
  end

  subgraph OneInd["Сборка 1 особи (chrom)"]
    Chrom["chrom: int16[n_classes x 4]<br/>строки = гены i"]
    RoomAtSlot["room_at_slot[sid] = {room_idx...}<br/>(какие аудитории уже заняты в слоте sid)"]
    TeacherBusy["teacher_busy[teacher] = {sid...}<br/>(в каких слотах занят преподаватель)"]
  end

  subgraph SlotMath["Связь sid ↔ (day,time)"]
    SID["sid = day*Tm + time"]
    Unpack["day = sid // Tm<br/>time = sid % Tm"]
  end

  Fixed --> OneInd
  SlotMath --> OneInd

  %% Placement logic (conceptual)
  Step1["Берём группы в случайном порядке<br/>(groups_order)"] --> Step2["Для каждой группы g:<br/>берём indices = _classes_by_group[g]<br/>и тасуем их"]
  Step2 --> Step3["available_list = список всех sid<br/>(перемешан)"]
  Step3 --> Step4["Для каждого idx (класса i) этой группы:<br/>перебираем sid из available_list"]
  Step4 --> CheckRoom["Выбираем room r так, чтобы<br/>r не в room_at_slot[sid]"]
  Step4 --> CheckTeacher["Выбираем teacher te из<br/>_allowed_teachers_per_subject[s]<br/>так, чтобы sid не в teacher_busy[te]"]
  CheckRoom --> Place["Записываем chrom[i]=[day,time,r,te]<br/>и обновляем room_at_slot/teacher_busy<br/>и удаляем sid из available_list (у группы)"]
  CheckTeacher --> Place
  Place --> Step4
  Step4 -->|если не нашлось места| Restart["failed=True → restart whole chromosome<br/>(до max_restarts)"]

  Step1 --> Chrom
```


## Объяснение кроссовера (обмен целыми группами + восстановление)

```mermaid
flowchart LR
  %% ==========================================
  %% CROSSOVER: GROUP-CHUNK EXCHANGE
  %% ==========================================

  subgraph Parents[Родители]
    P1["parent1: chrom[n_classes x 4]"]
    P2["parent2: chrom[n_classes x 4]"]
  end

  subgraph Map[Карта классов по группам]
    CBG["_classes_by_group[g] -> [i1,i2,i3,...]"]
  end

  subgraph Pick[Выбор групп для обмена]
    Gpick["groups_pick = случайные g (size=k)"]
    Focus["focus_arr = объединение индексов i\nвсех выбранных групп"]
  end

  subgraph Children[Дети до repair]
    C1pre["child1 = parent1.copy()"]
    C2pre["child2 = parent2.copy()"]
    Swap["Для каждого g в groups_pick:\nдля каждого i in _classes_by_group[g]:\nchild1[i]=parent2[i]\nchild2[i]=parent1[i]"]
  end

  subgraph Repair[Repair/проверка жёстких ограничений]
    Repair1["_repair_crossover_quick(child1, focus_arr)"]
    Repair2["_repair_crossover_quick(child2, focus_arr)"]
    Check1{"hard_penalty(child1)==0?"}
    Check2{"hard_penalty(child2)==0?"}
    Fallback1["если нет → child1 = parent1.copy()"]
    Fallback2["если нет → child2 = parent2.copy()"]
  end

  P1 --> C1pre
  P2 --> C2pre
  CBG --> Gpick
  Gpick --> Focus
  C1pre --> Swap
  C2pre --> Swap
  Swap --> Repair1 --> Check1 -->|нет| Fallback1
  Swap --> Repair2 --> Check2 -->|нет| Fallback2
```



## Визуальное объяснение каждой мутации (4 типа)


### Мутация room (смена аудитории в том же слоте)

```mermaid
flowchart TB
  i["выбран ген i"] --> Slot["sid = day*Tm + time (текущий слот)"]
  Slot --> Taken["taken = {room всех генов k≠i<br/>у которых slot_id(k)==sid}"]
  Taken --> Alt["alternatives = rooms - taken - {room_i}"]
  Alt -->|если есть| Set["chrom[i,2] = random(alternatives)"]
  Alt -->|если пусто| Fail["операция не применима"]
```

### Мутации teacher (смена преподавателя в том же слоте)

```mermaid
flowchart TB
  i["выбран ген i"] --> Slot["sid = day*Tm + time (текущий слот)"]
  i --> S["s = class_subjects_[i] (предмет)"]
  Slot --> Busy["busy_te = {teacher всех k≠i<br/>у которых slot_id(k)==sid}"]
  S --> Allowed["allowed = _allowed_teachers_per_subject[s]"]
  Busy --> Cand["candidates = allowed - busy_te - {teacher_i}"]
  Allowed --> Cand
  Cand -->|если есть| Set["chrom[i,3] = random(candidates)"]
  Cand -->|если пусто| Fail["операция не применима"]
```


### Мутация reslot (перенос в другой слот + подбор room/teacher)

```mermaid
flowchart TB
  i["выбран ген i"] --> g["g = class_groups_[i] (группа)"]
  i --> s["s = class_subjects_[i] (предмет)"]
  g --> Used["used_by_others = {slot_id(j)\nдля всех j в _classes_by_group[g], j≠i}"]
  Used --> Cands["candidates sid: все _all_slot_ids\nкроме used_by_others и sid_old"]
  Cands --> Pick["перебор sid-кандидатов"]
  Pick --> Try["_pick_feasible_tuple_at_slot(sid, subject=s, skip={i})\nищет свободную room и свободного teacher\nв этом sid"]
  Try -->|нашёл| Set["chrom[i] = (day,time,room,teacher) из picked"]
  Try -->|не нашёл| Next["берём следующий sid"]
```


### Мутация swap (обмен слотами с товарищем по группе + пересбор room/teacher)

```mermaid
flowchart TB
  i["выбран ген i"] --> g["g = class_groups_[i]"]
  g --> Siblings["siblings = {j in _classes_by_group[g] | j≠i}"]
  Siblings --> PickJ["выбираем случайный j"]
  i --> sidI["sid_i = slot_id(i)"]
  PickJ --> sidJ["sid_j = slot_id(j)"]
  sidI --> Diff{"sid_i != sid_j ?"}
  sidJ --> Diff
  Diff -->|нет| Fail["swap не имеет смысла"]
  Diff -->|да| si["s_i = class_subjects_[i]"]
  Diff -->|да| sj["s_j = class_subjects_[j]"]
  si --> PickTi["_pick_feasible_tuple_at_slot(sid_j, subject=s_i, skip={i,j})"]
  sj --> PickTj["_pick_feasible_tuple_at_slot(sid_i, subject=s_j, skip={i,j})"]
  PickTi -->|оба нашли| Apply["присваиваем chrom[i]=tup_i\nchrom[j]=tup_j"]
  PickTi -->|не нашёл| Fail
  PickTj -->|не нашёл| Fail
```