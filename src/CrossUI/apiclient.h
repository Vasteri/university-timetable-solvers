#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

class ApiClient: public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject* parent = nullptr)
        : QObject(parent), manager(new QNetworkAccessManager(this)) {}

    void get(const QUrl& url);
    void postJson(const QUrl& url, const QJsonObject& json);
    void putJson(const QUrl& url, const QJsonObject& json);
    void del(const QUrl& url);
    void postFile(const QUrl& url, const QString& filePath);

signals:
    void jsonReceived(const QJsonObject& obj);
    void textReceived(const QString& text);
    void binaryReceived(const QByteArray& data);
    void errorOccured(const QString& err);

private:
    QNetworkAccessManager* manager;

    void connectReply(QNetworkReply* reply);
    void handleReply(QNetworkReply* reply);
};

#endif // APICLIENT_H
