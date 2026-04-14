#ifndef GLOBALDATATRANSITION_H
#define GLOBALDATATRANSITION_H

#include <QObject>
#include <QJsonObject>
#include "apiclient.h"

class GlobalDataTransition: public QObject {
    Q_OBJECT
private:
    ApiClient *api;
    QJsonObject input_data;
    QJsonObject output_data;
    void init_connect();

public:
    QString ip;
    int status;
    QString message;
    explicit GlobalDataTransition(QObject* parent = nullptr);
    ~GlobalDataTransition();
    void SetData(const QJsonObject& obj);
    void SetDataResult(const QJsonObject& obj);
    void SendData(const QJsonObject& extra = QJsonObject());

    QJsonObject GetData() const {return output_data;};

signals:
    void Received();
};

#endif // GLOBALDATATRANSITION_H
