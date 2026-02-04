#include "graphic.h"
#include "ui_graphic.h"

#include <QFontMetrics>
#include <QGraphicsSceneHoverEvent>
#include <QJsonDocument>
#include <QMap>
#include <QPainter>
#include <QSet>
#include <QTime>

Graphic::Graphic(QWidget *parent, GlobalDataTransition* data)
    : QWidget(parent)
    , ui(new Ui::Graphic)
{
    ui->setupUi(this);
    this->data = data;
    init_response_api(ui->but_draw);

    sceneGroups = new QGraphicsScene(this);
    sceneTeachers = new QGraphicsScene(this);
    sceneRooms = new QGraphicsScene(this);

    const auto initView = [](QGraphicsView* view){
        view->setRenderHint(QPainter::Antialiasing);
        view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    };

    initView(ui->view_groups);
    initView(ui->view_teachers);
    initView(ui->view_rooms);

    ui->view_groups->setScene(sceneGroups);
    ui->view_teachers->setScene(sceneTeachers);
    ui->view_rooms->setScene(sceneRooms);

    // масштабирование: кнопки + и -
    const double zoomFactorIn  = 1.2;
    const double zoomFactorOut = 1.0 / zoomFactorIn;

    auto applyZoom = [this](double factor){
        ui->view_groups->scale(factor, factor);
        ui->view_teachers->scale(factor, factor);
        ui->view_rooms->scale(factor, factor);
    };

    connect(ui->but_zoom_in, &QPushButton::clicked, this, [applyZoom, zoomFactorIn](){
        applyZoom(zoomFactorIn);
    });
    connect(ui->but_zoom_out, &QPushButton::clicked, this, [applyZoom, zoomFactorOut](){
        applyZoom(zoomFactorOut);
    });

    // растягиваем таб с диаграммой на всё доступное пространство
    ui->verticalLayout->setStretch(0, 0); // панель управления
    ui->verticalLayout->setStretch(1, 1); // вкладки с диаграммами

    clearScenes();
    updateStatus(tr("Нажмите «Построить» для загрузки расписания"), false);
}

Graphic::~Graphic()
{
    delete ui;
}

void Graphic::init_response_api(QPushButton *but_api) {
    connect(but_api, &QPushButton::clicked, this, [this](){
        qDebug() << "1";
        QJsonObject obj = this->data->GetData();
        clearScenes();
        qDebug() << "1.5";
        if (obj.contains("result") && obj["result"].isArray()) {
            qDebug() << "2";
            QVector<Lesson> lessons;
            if (!parseSchedule(obj, lessons)) {
                clearScenes();
                updateStatus(tr("Не удалось разобрать ответ сервера"), true);
                return;
            }
            qDebug() << "2.5";
            renderAll(lessons);
            qDebug() << "2.6";
            updateStatus(tr("Найдено пар: %1").arg(lessons.size()), false);
        }
        else if (obj.isEmpty()){
            qDebug() << "3";
            updateStatus(QString("Статус: Нет данных"), true);
        }
        else {
            qDebug() << "4";
            updateStatus(QString("Статус: Incorrect json format"), true);
        }
        qDebug() << "end";
    });
}

void Graphic::clearScenes()
{
    auto reset = [](QGraphicsScene* scene, const QString& text){
        scene->clear();
        auto item = scene->addText(text);
        item->setDefaultTextColor(Qt::gray);
        scene->setSceneRect(QRectF(0, 0, 400, 120));
    };

    reset(sceneGroups, tr("Нет данных для групп"));
    reset(sceneTeachers, tr("Нет данных для преподавателей"));
    reset(sceneRooms, tr("Нет данных для кабинетов"));
}

void Graphic::updateStatus(const QString& text, bool isError)
{
    ui->lab_info->setStyleSheet(isError ? "color: #b00020;" : "");
    ui->lab_info->setText(text);
}

bool Graphic::parseSchedule(const QJsonObject& obj, QVector<Lesson>& lessons)
{
    lessons.clear();
    const QJsonValue resVal = obj.value(QStringLiteral("result"));
    if (!resVal.isArray()) {
        return false;
    }

    const QJsonArray arr = resVal.toArray();
    if (arr.isEmpty()) {
        return false;
    }

    const auto parseArrayRow = [](const QJsonArray& row, Lesson& l){
        if (row.size() < 6) {
            return false;
        }
        l.group = row.at(0).toString();
        l.day = row.at(1).toString();
        l.time = row.at(2).toString();
        l.subject = row.at(3).toString();
        l.room = row.at(4).toString();
        l.teacher = row.at(5).toString();
        return !(l.group.isEmpty() || l.day.isEmpty() || l.time.isEmpty());
    };

    for (int i = 0; i < arr.size(); ++i) {
        const QJsonValue rowVal = arr.at(i);
        Lesson l;

        if (rowVal.isArray()) {
            const QJsonArray row = rowVal.toArray();
            // пропускаем строку-заголовок если есть
            if (i == 0 && row.size() >= 3 && row.at(0).toString().toLower().contains("group")) {
                continue;
            }
            if (!parseArrayRow(row, l)) {
                continue;
            }
        } else if (rowVal.isObject()) {
            const QJsonObject rowObj = rowVal.toObject();
            l.group = rowObj.value("group").toString();
            l.day = rowObj.value("day").toString();
            l.time = rowObj.value("time").toString();
            l.subject = rowObj.value("subject").toString();
            l.room = rowObj.value("room").toString();
            l.teacher = rowObj.value("teacher").toString();
            if (l.group.isEmpty() || l.day.isEmpty() || l.time.isEmpty()) {
                continue;
            }
        } else {
            continue;
        }

        lessons.push_back(l);
    }

    return !lessons.isEmpty();
}

QStringList Graphic::orderedDays(const QVector<Lesson>& lessons) const
{
    static const QStringList defaultDays = {
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday",
        QString::fromUtf8("Понедельник"), QString::fromUtf8("Вторник"),
        QString::fromUtf8("Среда"), QString::fromUtf8("Четверг"),
        QString::fromUtf8("Пятница"), QString::fromUtf8("Суббота"), QString::fromUtf8("Воскресенье")
    };

    QStringList seen;
    QSet<QString> used;
    for (const auto& l : lessons) {
        if (!used.contains(l.day)) {
            used.insert(l.day);
            seen.push_back(l.day);
        }
    }

    QStringList ordered;
    QSet<QString> added;
    for (const QString& d : defaultDays) {
        if (used.contains(d) && !added.contains(d)) {
            ordered.push_back(d);
            added.insert(d);
        }
    }
    for (const QString& d : seen) {
        if (!added.contains(d)) {
            ordered.push_back(d);
            added.insert(d);
        }
    }
    return ordered.isEmpty() ? seen : ordered;
}

QStringList Graphic::orderedTimes(const QVector<Lesson>& lessons) const
{
    static const QStringList defaultTimes = { "9:20", "11:10", "13:45", "15:35", "17:25", "19:00" };

    QStringList seen;
    QSet<QString> used;
    for (const auto& l : lessons) {
        if (!used.contains(l.time)) {
            used.insert(l.time);
            seen.push_back(l.time);
        }
    }

    auto timeLess = [](const QString& a, const QString& b){
        QTime ta = QTime::fromString(a, "H:mm");
        QTime tb = QTime::fromString(b, "H:mm");
        if (ta.isValid() && tb.isValid()) {
            return ta < tb;
        }
        return a < b;
    };

    QStringList ordered;
    QSet<QString> added;
    for (const QString& t : defaultTimes) {
        if (used.contains(t) && !added.contains(t)) {
            ordered.push_back(t);
            added.insert(t);
        }
    }
    for (const QString& t : seen) {
        if (!added.contains(t)) {
            added.insert(t);
            ordered.push_back(t);
        }
    }
    std::sort(ordered.begin(), ordered.end(), timeLess);
    return ordered.isEmpty() ? seen : ordered;
}

void Graphic::renderAll(const QVector<Lesson>& lessons)
{
    const QStringList days = orderedDays(lessons);
    const QStringList times = orderedTimes(lessons);

    renderDiagram(sceneGroups, ui->view_groups, lessons, days, times,
                  [](const Lesson& l){ return l.group; },
                  tr("Нет занятий по группам"));

    renderDiagram(sceneTeachers, ui->view_teachers, lessons, days, times,
                  [](const Lesson& l){ return l.teacher; },
                  tr("Нет занятий по преподавателям"));

    renderDiagram(sceneRooms, ui->view_rooms, lessons, days, times,
                  [](const Lesson& l){ return l.room; },
                  tr("Нет занятий по кабинетам"));
}

void Graphic::renderDiagram(QGraphicsScene* scene,
                            QGraphicsView* view,
                            const QVector<Lesson>& lessons,
                            const QStringList& days,
                            const QStringList& times,
                            const std::function<QString(const Lesson&)>& rowSelector,
                            const QString& emptyText)
{
    scene->clear();

    if (lessons.isEmpty() || days.isEmpty() || times.isEmpty()) {
        auto item = scene->addText(emptyText);
        item->setDefaultTextColor(Qt::gray);
        scene->setSceneRect(QRectF(0, 0, 400, 120));
        return;
    }

    QStringList rows;
    QSet<QString> usedRows;
    for (const auto& l : lessons) {
        const QString key = rowSelector(l);
        if (!usedRows.contains(key)) {
            usedRows.insert(key);
            rows.push_back(key);
        }
    }

    if (rows.isEmpty()) {
        auto item = scene->addText(emptyText);
        item->setDefaultTextColor(Qt::gray);
        scene->setSceneRect(QRectF(0, 0, 400, 120));
        return;
    }

    const double leftPadding = 4.0;   // меньше отступ слева
    const double topPadding = 12.0;
    const double labelWidth = 96.0;  // еще меньше отступ для подписи
    const double slotWidth = 70.0;    // еще чуть компактнее
    const double slotHeight = 68.0;   // чуть выше для читаемости
    const double slotGap = 3.0;       // плотнее между слотами
    const double rowGap = 10.0;
    const double dayGap = 8.0;        // меньше зазор между днями
    const double axisMargin = 20.0;   // меньше пустоты снизу

    const QVector<QColor> palette = {
        QColor("#4CAF50"), QColor("#FF9800"), QColor("#03A9F4"),
        QColor("#E91E63"), QColor("#9C27B0"), QColor("#009688"),
        QColor("#8BC34A"), QColor("#FFC107"), QColor("#00BCD4"),
        QColor("#607D8B")
    };

    QFontMetrics fm(view->font());

    const double dayBlock = times.size() * (slotWidth + slotGap);

    QMap<QString, int> dayIndex;
    for (int i = 0; i < days.size(); ++i) {
        dayIndex.insert(days.at(i), i);
    }
    QMap<QString, int> timeIndex;
    for (int i = 0; i < times.size(); ++i) {
        timeIndex.insert(times.at(i), i);
    }

    // подписи слева
    for (int row = 0; row < rows.size(); ++row) {
        const double y = topPadding + row * (slotHeight + rowGap);
        auto textItem = scene->addText(shortText(rows.at(row), 18));
        textItem->setDefaultTextColor(Qt::white);
        QRectF br = textItem->boundingRect();
        textItem->setPos(leftPadding, y + slotHeight / 2.0 - br.height() / 2.0);
    }

    // клетки с занятиями
    for (const auto& l : lessons) {
        const QString rowKey = rowSelector(l);
        const int rowPos = rows.indexOf(rowKey);
        if (rowPos < 0) continue;

        const int dIdx = dayIndex.value(l.day, 0);
        const int tIdx = timeIndex.value(l.time, 0);

        const double startX = leftPadding + labelWidth + dIdx * dayBlock + dIdx * dayGap
                              + tIdx * (slotWidth + slotGap);
        const double startY = topPadding + rowPos * (slotHeight + rowGap);

        const QColor color = palette.at(qAbs(qHash(l.subject)) % palette.size());
        const QString tooltip = tr("Предмет: %1\nГруппа: %2\nПреподаватель: %3\nАудитория: %4\nДень: %5\nВремя: %6")
                                    .arg(l.subject, l.group, l.teacher, l.room, l.day, l.time);

        auto rectItem = new HoverRectItem(QRectF(startX, startY, slotWidth, slotHeight), color, tooltip);
        scene->addItem(rectItem);

        // Нужно ли показывать ФИО преподавателя в ячейке
        const bool isTeachersTab = (scene == sceneTeachers);

        const QString subjectText  = shortText(l.subject, 14);
        const QString teacherText  = shortText(l.teacher, 14);

        // Отдельный текстовый элемент для предмета
        auto subjectItem = scene->addText(subjectText);
        QRectF brSub = subjectItem->boundingRect();
        double subjectY;
        if (isTeachersTab) {
            // во вкладке «Преподаватели» центрируем предмет по вертикали
            subjectY = startY + slotHeight / 2.0 - brSub.height() / 2.0;
        } else {
            // в остальных вкладках оставляем место под преподавателя
            subjectY = startY + slotHeight / 2.0 - brSub.height();
        }
        subjectItem->setPos(startX + slotWidth / 2.0 - brSub.width() / 2.0,
                            subjectY);

        // Отдельный текстовый элемент для преподавателя — только не во вкладке «Преподаватели»
        if (!isTeachersTab) {
            auto teacherItem = scene->addText(teacherText);
            QRectF brTeach = teacherItem->boundingRect();
            teacherItem->setPos(startX + slotWidth / 2.0 - brTeach.width() / 2.0,
                                startY + slotHeight / 2.0);
        }
    }

    // ось времени снизу
    const double axisY = topPadding + rows.size() * (slotHeight + rowGap) + 2.0;
    for (int d = 0; d < days.size(); ++d) {
        for (int t = 0; t < times.size(); ++t) {
            const double x = leftPadding + labelWidth + d * dayBlock + d * dayGap
                             + t * (slotWidth + slotGap) + slotWidth / 2.0;
            const QString label = days.at(d) + "\n" + times.at(t);
            auto axisText = scene->addText(label);
            QRectF br = axisText->boundingRect();
            axisText->setPos(x - br.width() / 2.0, axisY);
        }
    }

    const double width = leftPadding + labelWidth
            + days.size() * dayBlock + (days.size() - 1) * dayGap;
    const double height = axisY + axisMargin + 40.0;
    scene->setSceneRect(0, 0, width, height);
    view->centerOn(0, 0);
}
