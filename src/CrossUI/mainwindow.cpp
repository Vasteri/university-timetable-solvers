#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ip = ui->edit_ip->text();
    api = new ApiClient(this);
    graphic = new Graphic(this);
    tablish = new Tablish(this);
    input_data = new InputData(this);

    //this->setCentralWidget(graphic);
    ui->tabWidget->widget(1)->layout()->addWidget(input_data);
    ui->tabWidget->widget(2)->layout()->addWidget(graphic);
    ui->tabWidget->widget(3)->layout()->addWidget(tablish);

    connect(ui->but_send, &QPushButton::clicked, this, [this](){
        ui->label_res->setText("Запрос...");
        ip = ui->edit_ip->text();
        api->get(QUrl("http://" + ip + ":8000"));
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
    delete input_data;
}
