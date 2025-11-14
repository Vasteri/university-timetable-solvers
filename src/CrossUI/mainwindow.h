#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "apiclient.h"
#include "graphic.h"
#include "tablish.h"
#include "inputdata.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString get_ip() const {return ip;};

private:
    QString ip;
    Graphic *graphic;
    ApiClient *api;
    Tablish *tablish;
    InputData *input_data;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
