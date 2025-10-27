#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    api = new ApiClient(this);
    graphic = new Graphic(this);
    tablish = new Tablish(this);

    //this->setCentralWidget(graphic);
    ui->tabWidget->widget(1)->layout()->addWidget(graphic);

    ui->tabWidget->widget(2)->layout()->addWidget(tablish);

    connect(ui->but_send, &QPushButton::clicked, this, [this](){
        ui->label_res->setText("");
        api->get(QUrl("http://" + ui->edit_ip->text() + ":8000"));
    });

    connect(api, &ApiClient::jsonReceived, this, [this](const QJsonObject& obj){
        ui->label_res->setText(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    });

    connect(api, &ApiClient::errorOccured, this, [this](const QString& err){
        ui->label_res->setText(err);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    delete api;
    delete graphic;
    delete tablish;
}
