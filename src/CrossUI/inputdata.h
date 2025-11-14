#ifndef INPUTDATA_H
#define INPUTDATA_H

#include <QWidget>
#include "apiclient.h"
#include <QStandardItemModel>

namespace Ui {
class InputData;
}

class InputData : public QWidget
{
    Q_OBJECT

public:
    explicit InputData(QWidget *parent = nullptr);
    ~InputData();

private:
    Ui::InputData *ui;
    QString ip;
    ApiClient *api;
    QStandardItemModel *model;
};

#endif // INPUTDATA_H
