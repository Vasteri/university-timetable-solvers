#ifndef TABLISH_H
#define TABLISH_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include "apiclient.h"

namespace Ui {
class Tablish;
}

class Tablish : public QWidget
{
    Q_OBJECT

public:
    explicit Tablish(QWidget *parent = nullptr);
    ~Tablish();

private:
    Ui::Tablish *ui;
    ApiClient *api;

    void init_response_api(QLabel *lab_api, QPushButton *but_api);
};

#endif // TABLISH_H
