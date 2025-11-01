#include "apiclient.h"

void ApiClient::get(const QUrl& url){
    qDebug() << "Apiclient get";
    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);
    connectReply(reply);
}

void ApiClient::postJson(const QUrl& url, const QJsonObject& json){
    qDebug() << "Apiclient postJson";
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = manager->post(request, QJsonDocument(json).toJson());
    connectReply(reply);
}

void ApiClient::putJson(const QUrl& url, const QJsonObject& json){
    qDebug() << "Apiclient putJson";
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = manager->put(request, QJsonDocument(json).toJson());
    connectReply(reply);
}

void ApiClient::del(const QUrl& url){
    qDebug() << "Apiclient del";
    QNetworkRequest request(url);
    QNetworkReply* reply = manager->deleteResource(request);
    connectReply(reply);
}

void ApiClient::postFile(const QUrl& url, const QString& filePath){
    QFile* file = new QFile(filePath, this);
    if (!file->open(QIODevice::ReadOnly)) {
        emit errorOccured("Не удалось открыть файл: " + filePath);
        delete file;
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

    QNetworkReply* reply = manager->post(request, file);
    file->setParent(reply); // освободится автоматически
    connectReply(reply);
}



void ApiClient::connectReply(QNetworkReply* reply){
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleReply(reply);
    });
}

void ApiClient::handleReply(QNetworkReply* reply){
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error apiclient: " << reply->errorString() << "\n";
        emit errorOccured(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray response = reply->readAll();
    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();

    if (contentType.contains("application/json")) {
        qDebug() << "Received json";
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            emit jsonReceived(doc.object());
        } else {
            qDebug() << "error json";
            emit errorOccured("Некорректный JSON");
        }
    } else if (contentType.startsWith("text/")) {
        qDebug() << "Received text";
        emit textReceived(QString::fromUtf8(response));
    } else {
        qDebug() << "Received binary";
        emit binaryReceived(response);
    }

    reply->deleteLater();
}
