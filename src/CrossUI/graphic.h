#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <QWidget>

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
    Ui::Graphic *ui;
    ApiClient *api;
};

#endif // GRAPHIC_H
