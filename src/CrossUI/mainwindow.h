#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "apiclient.h"
#include "graphic.h"
#include "tablish.h"

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

private:
    Graphic *graphic;
    ApiClient *api;
    Tablish *tablish;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
