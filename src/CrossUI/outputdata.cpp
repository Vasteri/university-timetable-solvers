#include "outputdata.h"
#include "ui_outputdata.h"

#include <QJsonArray>

OutputData::OutputData(QWidget *parent, GlobalDataTransition* data)
    : QWidget(parent)
    , ui(new Ui::OutputData)
{
    ui->setupUi(this);
    this->data = data;

    connect(this->data, &GlobalDataTransition::Received, this, [this](){

        QJsonObject obj = this->data->GetData();
        QJsonDocument doc(obj);
        ui->lab_info->setText(this->data->message + " " + obj["status"].toString());
        ui->lab_test->setText(doc.toJson(QJsonDocument::Compact));
        /*
        if (obj.contains("result") && obj["result"].isArray()) {
            QJsonArray jsonArray = obj["result"].toArray();
        }
        else if (obj.isEmpty()){
            lab_api->setText(QString("Статус: Нет данных"));
        }
        else {
            lab_api->setText(QString("Статус: Incorrect json format"));
        }
        */
    });
}

OutputData::~OutputData()
{
    delete ui;
}
