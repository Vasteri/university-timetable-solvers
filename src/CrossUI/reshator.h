#ifndef RESHATOR_H
#define RESHATOR_H

#include <QWidget>
#include "apiclient.h"

namespace Ui {
class Reshator;
}

class Reshator : public QWidget
{
    Q_OBJECT

public:
    explicit Reshator(QWidget *parent = nullptr);
    ~Reshator();

private:
    QString ip;
    ApiClient *api;
    Ui::Reshator *ui;
};

#endif // RESHATOR_H
