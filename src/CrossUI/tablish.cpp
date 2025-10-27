#include "tablish.h"
#include "ui_tablish.h"
#include <QJsonArray>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

Tablish::Tablish(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tablish)
{
    ui->setupUi(this);
    api = new ApiClient();
    init_response_api(ui->lab_info, ui->pushButton);
}

Tablish::~Tablish()
{
    delete ui;
    delete api;
}


void Tablish::init_response_api(QLabel *lab_api, QPushButton *but_api) {
    connect(but_api, &QPushButton::clicked, this, [this, lab_api](){
        lab_api->setText("Sending...");
        api->get(QUrl("http://127.0.0.1:8000/solve_pulp"));
    });

    connect(api, &ApiClient::jsonReceived, this, [this, lab_api](const QJsonObject& obj){
        lab_api->setText("Received: ");
        //lab_api->setText(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
        if (obj.contains("result") && obj["result"].isArray()) {
            QJsonArray jsonArray = obj["result"].toArray();
            qDebug() << jsonArray;
            //
            QStandardItemModel *model = new QStandardItemModel;

            QStringList headers = {"group", "subject", "day", "time", "room", "teacher"};
            model->setHorizontalHeaderLabels(headers);

            for (const QJsonValue &val : jsonArray) {
                QJsonObject obj = val.toObject();
                QList<QStandardItem*> row;
                for (const QString &key : headers)
                    row << new QStandardItem(obj[key].toString());
                model->appendRow(row);
            }

            QSortFilterProxyModel *proxy = new QSortFilterProxyModel;
            proxy->setSourceModel(model);
            proxy->setDynamicSortFilter(true);

            ui->table_view_res->setModel(proxy);
            ui->table_view_res->setSortingEnabled(true);
            //
        }
        else {
            lab_api->setText(lab_api->text() + QString("Incorrect json format"));
        }
    });

    connect(api, &ApiClient::errorOccured, this, [this, lab_api](const QString& err){
        lab_api->setText(err);
    });
}
