#include "reshator.h"
#include "ui_reshator.h"

Reshator::Reshator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Reshator)
{
    ui->setupUi(this);

    api = new ApiClient(this);

    connect(ui->but_send, &QPushButton::clicked, this, [this](){
        ui->lab_info->setText("Запрос...");
        ip = ui->edit_ip->text();
        api->get(QUrl("http://" + ip + ":8000"));
    });

    connect(api, &ApiClient::jsonReceived, this, [this](const QJsonObject& obj){
        ui->lab_info->setText(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    });

    connect(api, &ApiClient::errorOccured, this, [this](const QString& err){
        ui->lab_info->setText(err);
    });
}

Reshator::~Reshator()
{
    delete ui;
    delete api;
}
