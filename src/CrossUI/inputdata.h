#ifndef INPUTDATA_H
#define INPUTDATA_H

#include <QWidget>
#include <QStandardItemModel>
#include "globaldatatransition.h"

namespace Ui {
class InputData;
}

class InputData : public QWidget
{
    Q_OBJECT

public:
    explicit InputData(QWidget *parent = nullptr, GlobalDataTransition* data = nullptr);
    ~InputData();

private:
    QString filename;
    GlobalDataTransition* data;
    Ui::InputData *ui;
    QStandardItemModel *model;
    void SetDataToModel(const QJsonObject& jsonObject);
    QJsonObject NormalizedLists(QJsonObject input);
    void NewFile();
    void OpenFile();
    void SaveFile();
    void ReopenFile();
};

#endif // INPUTDATA_H
