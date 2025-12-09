#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <QVector>
#include <functional>

#include "apiclient.h"

namespace Ui {
class Graphic;
}

class Graphic : public QWidget
{
    Q_OBJECT

public:
    explicit Graphic(QWidget *parent = nullptr);
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
    ApiClient *api;
    QGraphicsScene* sceneGroups;
    QGraphicsScene* sceneTeachers;
    QGraphicsScene* sceneRooms;
};

#endif // GRAPHIC_H
