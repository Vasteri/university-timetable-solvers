#ifndef RESHATOR_H
#define RESHATOR_H

#include <QWidget>
#include "globaldatatransition.h"

namespace Ui {
class Reshator;
}

class Reshator : public QWidget
{
    Q_OBJECT

public:
    explicit Reshator(QWidget *parent = nullptr, GlobalDataTransition* data = nullptr);
    ~Reshator();

private:
    GlobalDataTransition* data;
    Ui::Reshator *ui;
};

#endif // RESHATOR_H
