#include "inputdata.h"
#include "ui_inputdata.h"

#include "mainwindow.h"

#include <QFileDialog>
#include <QJsonArray>

void addJsonToItem(const QJsonValue &value, QStandardItem *parent);
QJsonValue modelItemToJson(QStandardItem *item);

InputData::InputData(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InputData)
{
    ui->setupUi(this);
    api = new ApiClient();
    model = new QStandardItemModel();

    connect(ui->but_open_file, &QPushButton::clicked, this, [this](){

        QString fileName = QFileDialog::getOpenFileName(this, "Select JSON file", "", "*.json");
        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot open file";
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!doc.isObject()) {
            qWarning() << "JSON is not an object";
            return;
        }

        QJsonObject jsonObject = doc.object();

        //==================================
        model->clear();
        model->setHorizontalHeaderLabels({"Key", "Value"});
        addJsonToItem(jsonObject, model->invisibleRootItem());
        ui->treeView->setModel(model);
        ui->treeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
        ui->treeView->expandAll();
        ui->treeView->show();
        //=============================

        ui->lab_info->setText("Sending...");
        api->postJson(QUrl("http://127.0.0.1:8000/solve_pulp_2"), jsonObject);
    });

    connect(model, &QStandardItemModel::itemChanged, this, [this]() {
        QJsonValue updatedJson = modelItemToJson(model->invisibleRootItem());
        ui->lab_info->setText(QJsonDocument(updatedJson.toObject()).toJson(QJsonDocument::Compact));
    });

    connect(api, &ApiClient::jsonReceived, this, [this](const QJsonObject& obj){
        ui->lab_info->setText(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    });

    connect(api, &ApiClient::errorOccured, this, [this](const QString& err){
        ui->lab_info->setText(err);
    });

    auto *mw = qobject_cast<MainWindow*>(parentWidget());
    if (mw) {
        ip = mw->get_ip();
        ui->lab_info->setText(ip);
    }
}

InputData::~InputData()
{
    delete ui;
    delete api;
}


void addJsonToItem(const QJsonValue &value, QStandardItem *parent) {
    if (value.isObject()) {
        QJsonObject obj = value.toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QStandardItem *keyItem = new QStandardItem(it.key());
            QStandardItem *valueItem = new QStandardItem("");
            parent->appendRow({keyItem, valueItem});
            addJsonToItem(it.value(), keyItem); // рекурсивно добавляем вложенные объекты
        }
    } else if (value.isArray()) {
        QJsonArray arr = value.toArray();
        for (int i = 0; i < arr.size(); ++i) {
            addJsonToItem(arr[i], parent);
        }
    } else {
        QStandardItem *valueItem = new QStandardItem(value.toVariant().toString());
        parent->appendRow({new QStandardItem(""), valueItem});
    }
}


QJsonValue modelItemToJson(QStandardItem *item) {
    // Если нет дочерних элементов - это примитив
    if (!item->hasChildren()) {
        return QJsonValue(item->text());
    }
    if (item->rowCount() == 1) {
        return QJsonValue(item->child(0, 1)->text());
    }

    bool allEmptyKeys = true;
    for (int i = 0; i < item->rowCount(); ++i) {
        if (!item->child(i, 0)->text().isEmpty()) {
            allEmptyKeys = false;
            break;
        }
    }

    if (allEmptyKeys) {
        // Это массив
        QJsonArray arr;
        for (int i = 0; i < item->rowCount(); ++i) {
            QStandardItem *childValue = item->child(i, 1);
            arr.append(modelItemToJson(childValue));
        }
        return arr;
    } else {
        // Это объект
        QJsonObject obj;
        for (int i = 0; i < item->rowCount(); ++i) {
            QString key = item->child(i, 0)->text();
            QStandardItem *childValue = item->child(i, 0);
            obj[key] = modelItemToJson(childValue);
        }
        return obj;
    }
}

