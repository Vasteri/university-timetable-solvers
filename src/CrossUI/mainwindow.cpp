#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    data = new GlobalDataTransition(this);
    graphic = new Graphic(this, data);
    tablish = new Tablish(this, data);
    input_data = new InputData(this, data);
    reshator = new Reshator(this, data);

    //this->setCentralWidget(graphic);
    ui->tabWidget->widget(0)->layout()->addWidget(input_data);
    ui->tabWidget->widget(1)->layout()->addWidget(reshator);
    ui->tabWidget->widget(2)->layout()->addWidget(graphic);
    ui->tabWidget->widget(3)->layout()->addWidget(tablish);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete graphic;
    delete tablish;
    delete input_data;
    delete reshator;
    delete data;
}
