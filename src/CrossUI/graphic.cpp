#include "graphic.h"
#include "ui_graphic.h"

#include <algorithm>
#include <QFontMetrics>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsTextItem>
#include <QJsonDocument>
#include <QMap>
#include <QPainter>
#include <QSet>
#include <QTime>

namespace {
class HoverRectItem : public QGraphicsRectItem
{
public:
    HoverRectItem(const QRectF& rect, const QColor& color, const QString& tooltip)
        : QGraphicsRectItem(rect), baseColor(color)
    {
        setBrush(baseColor);
        setPen(QPen(Qt::black, 1));
        setAcceptHoverEvents(true);
        setToolTip(tooltip);
    }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent*) override { setBrush(baseColor.lighter(130)); }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override { setBrush(baseColor); }

private:
    QColor baseColor;
};

QString shortText(const QString& src, int maxLen = 10)
{
    if (src.size() <= maxLen) {
        return src;
    }
    return src.left(maxLen - 1) + QStringLiteral("…");
}
} // namespace

Graphic::Graphic(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Graphic)
{
    ui->setupUi(this);
    api = new ApiClient();

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

    // растягиваем таб с диаграммой на всё доступное пространство
    ui->verticalLayout->setStretch(0, 0); // панель управления
    ui->verticalLayout->setStretch(1, 1); // вкладки с диаграммами

    connect(ui->but_draw, &QPushButton::clicked, this, [this](){
        updateStatus(tr("Запрашиваю расписание…"), false);
        api->get(QUrl("http://127.0.0.1:8000/solve_pulp"));
    });

    connect(api, &ApiClient::jsonReceived, this, [this](const QJsonObject& obj){
        QVector<Lesson> lessons;
        if (!parseSchedule(obj, lessons)) {
            clearScenes();
            updateStatus(tr("Не удалось разобрать ответ сервера"), true);
            return;
        }
        renderAll(lessons);
        updateStatus(tr("Найдено пар: %1").arg(lessons.size()), false);
    });

    connect(api, &ApiClient::errorOccured, this, [this](const QString& err){
        clearScenes();
        updateStatus(err, true);
    });

    clearScenes();
    updateStatus(tr("Нажмите «Построить» для загрузки расписания"), false);
}

Graphic::~Graphic()
{
    delete ui;
    delete api;
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
    ui->lab_info->setStyleSheet(isError ? "color: #b00020;" : "color: #1a1a1a;");
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

        auto textItem = scene->addText(shortText(l.subject, 14));
        QRectF br = textItem->boundingRect();
        textItem->setPos(startX + slotWidth / 2.0 - br.width() / 2.0,
                         startY + slotHeight / 2.0 - br.height() / 2.0);
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
