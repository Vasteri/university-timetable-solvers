#ifndef OUTPUTDATA_H
#define OUTPUTDATA_H

#include <QWidget>
#include "globaldatatransition.h"

namespace Ui {
class OutputData;
}

class OutputData : public QWidget
{
    Q_OBJECT

public:
    explicit OutputData(QWidget *parent = nullptr, GlobalDataTransition* data = nullptr);
    ~OutputData();

private:
    Ui::OutputData *ui;
    GlobalDataTransition* data;
};

#endif // OUTPUTDATA_H
