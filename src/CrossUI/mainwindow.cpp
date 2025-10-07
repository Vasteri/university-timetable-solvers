#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    api = new ApiClient(this);

    connect(ui->but_send, &QPushButton::clicked, this, [this](){
        QJsonObject data;
        data["x1"] = ui->edit_1->text().toFloat();
        data["x2"] = ui->edit_2->text().toFloat();
        ui->label_res->setText("");
        api->postJson(QUrl("http://" + ui->edit_ip->text() + ":8000/predict"), data);
    });

    connect(api, &ApiClient::jsonReceived, this, [this](const QJsonObject& obj){
        ui->label_res->setText(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    delete api;
}
