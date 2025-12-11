#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <QVector>
#include <QPushButton>
#include <QGraphicsRectItem>

#include "globaldatatransition.h"

namespace Ui {
class Graphic;
}

class Graphic : public QWidget
{
    Q_OBJECT

public:
    explicit Graphic(QWidget *parent = nullptr, GlobalDataTransition* data = nullptr);
    ~Graphic();

private:
    struct Lesson
    {
        QString group;
        QString day;
        QString time;
        QString subject;
        QString room;
        QString teacher;
    };
    GlobalDataTransition* data;

    void init_response_api(QPushButton *but_api);
    void clearScenes();
    void updateStatus(const QString& text, bool isError = false);
    bool parseSchedule(const QJsonObject& obj, QVector<Lesson>& lessons);
    QStringList orderedDays(const QVector<Lesson>& lessons) const;
    QStringList orderedTimes(const QVector<Lesson>& lessons) const;
    void renderAll(const QVector<Lesson>& lessons);
    void renderDiagram(QGraphicsScene* scene,
                       QGraphicsView* view,
                       const QVector<Lesson>& lessons,
                       const QStringList& days,
                       const QStringList& times,
                       const std::function<QString(const Lesson&)>& rowSelector,
                       const QString& emptyText);

    Ui::Graphic *ui;
    QGraphicsScene* sceneGroups;
    QGraphicsScene* sceneTeachers;
    QGraphicsScene* sceneRooms;
};

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

#endif // GRAPHIC_H
