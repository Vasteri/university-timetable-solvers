#include "graphic.h"
#include "ui_graphic.h"

Graphic::Graphic(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Graphic)
{
    ui->setupUi(this);
    api = new ApiClient();

    connect(ui->but_draw, &QPushButton::clicked, this, [this](){
        ui->lab_info->setText("");
        api->get(QUrl("http://127.0.0.1:8000/solve_pulp"));
    });

    connect(api, &ApiClient::jsonReceived, this, [this](const QJsonObject& obj){
        ui->lab_info->setText(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    });

    connect(api, &ApiClient::errorOccured, this, [this](const QString& err){
        ui->lab_info->setText(err);
    });
}

Graphic::~Graphic()
{
    delete ui;
    delete api;
}
