#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "graphic.h"
#include "tablish.h"
#include "inputdata.h"
#include "reshator.h"
#include "globaldatatransition.h"
#include "outputdata.h"

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
    GlobalDataTransition *data;
    Graphic *graphic;
    Tablish *tablish;
    InputData *input_data;
    OutputData *output_data;
    Reshator *reshator;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
