#include "globaldatatransition.h"

GlobalDataTransition::GlobalDataTransition(QObject* parent): QObject(parent) {
    ip = "127.0.0.1";
    status = 0;
    message = "";
    api = new ApiClient(this);
    init_connect();
}

GlobalDataTransition::~GlobalDataTransition() {
    delete api;
}

void GlobalDataTransition::init_connect() {

    connect(api, &ApiClient::jsonReceived, this, [this](const QJsonObject& obj){
        status = 0;
        message = "Получен json.";
        output_data = obj;
        //ui->lab_info->setText(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
        emit Received();
    });

    connect(api, &ApiClient::errorOccured, this, [this](const QString& err){
        status = 1;
        message = err;
        emit Received();
    });
}

void GlobalDataTransition::SetData(const QJsonObject& obj) {
    input_data = obj;
}

void GlobalDataTransition::SendData(const QJsonObject& extra) {
    if (!input_data.isEmpty()) {
        message = "Запрос...";
        QJsonObject payload = input_data;
        for (auto it = extra.begin(); it != extra.end(); ++it) {
            payload.insert(it.key(), it.value());
        }
        api->postJson(QUrl("http://" + ip + ":8000/solve"), payload);
    }
    else {
        status = 1;
        message = "Не выбран файл, или файл пуст";
        emit Received();
    }
}
