#include "inputdata.h"
#include "ui_inputdata.h"

#include <QFileDialog>
#include <QJsonArray>

void addJsonToItem(const QJsonValue &value, QStandardItem *parent);
QJsonValue modelItemToJson(QStandardItem *item);

InputData::InputData(QWidget *parent, GlobalDataTransition* data)
    : QWidget(parent)
    , ui(new Ui::InputData)
{
    ui->setupUi(this);
    this->data = data;
    filename = "";
    model = new QStandardItemModel();

    connect(ui->but_open_file, &QPushButton::clicked, this, &InputData::OpenFile);
    connect(ui->but_new_file, &QPushButton::clicked, this, &InputData::NewFile);
    connect(ui->but_save, &QPushButton::clicked, this, &InputData::SaveFile);
    connect(ui->but_cancel, &QPushButton::clicked, this, &InputData::ReopenFile);

    connect(model, &QStandardItemModel::itemChanged, this, [this]() {
        QJsonValue updatedJson = modelItemToJson(model->invisibleRootItem());
        ui->lab_info->setText(QJsonDocument(updatedJson.toObject()).toJson(QJsonDocument::Compact));
    });
}

InputData::~InputData()
{
    delete ui;
    delete data;
}

void InputData::NewFile() {
    QString fileName = QFileDialog::getSaveFileName(this, "Create JSON file", "", "*.json");
    if (fileName.isEmpty()) return;

    QString textjson = "{\"default_count\":2,\"groups\":[\"A-02-30\",\"A-05-30\",\"A-08-30\",\"A-16-30\",\"A-18-30\"],\"subjects\":[\"math\",\"physics\",\"english\",\"IT\",\"economic\"],\"teachers\":[\"T1\",\"T2\",\"T3\",\"T4\",\"T5\",\"T6\",\"T7\"],\"rooms\":[\"m700\",\"m707\",\"m200\"],\"days\":[\"Monday\",\"Tuesday\",\"Wednesday\",\"Thursday\",\"Friday\"],\"times\":[\"9:20\",\"11:10\",\"13:45\",\"15:35\"],\"subject_teachers\":{\"math\":[\"T1\"],\"physics\":[\"T2\"],\"english\":[\"T3\"],\"economic\":[\"T4\"],\"IT\":[\"T5\",\"T6\",\"T7\"]},\"subject_count\":{\"A-18-30\":{\"math\":2},\"A-16-30\":{\"math\":1}}}";
    QJsonDocument doc = QJsonDocument::fromJson(textjson.toUtf8());
    this->data->SetData(doc.object());

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        ui->lab_info->setText("Ошибка открытия файла для записи:" + file.errorString());
        return;
    }
    file.write(doc.toJson());
    file.close();

    QJsonObject jsonObject = doc.object();
    this->data->SetData(jsonObject);
    SetDataToModel(jsonObject);
    this->filename = fileName;
    ui->lab_info->setText("Файл создан.");
}

void InputData::OpenFile(){
    QString fileName = QFileDialog::getOpenFileName(this, "Select JSON file", "", "*.json");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        ui->lab_info->setText("Cannot open file");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        ui->lab_info->setText("JSON is not an object");
        return;
    }

    QJsonObject jsonObject = doc.object();
    this->data->SetData(jsonObject);
    SetDataToModel(jsonObject);
    this->filename = fileName;
    ui->lab_info->setText("Файл открыт.");
}

void InputData::SaveFile() {
    if (filename.isEmpty()) {
        ui->lab_info->setText("Файл не выбран");
        return;
    };
    QJsonObject jsonObject = modelItemToJson(model->invisibleRootItem()).toObject();
    QJsonObject normalized = NormalizedLists(jsonObject["subject_teachers"].toObject());
    jsonObject["subject_teachers"] = normalized;

    QJsonDocument doc = QJsonDocument(jsonObject);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        ui->lab_info->setText("Ошибка открытия файла для записи:" + file.errorString());
        return;
    }
    file.write(doc.toJson());
    file.close();

    this->data->SetData(jsonObject);
    SetDataToModel(jsonObject);
    ui->lab_info->setText("Файл сохранён.");
}

void InputData::ReopenFile() {
    if (filename.isEmpty()) {
        ui->lab_info->setText("Файл не выбран");
        return;
    };

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        ui->lab_info->setText("Cannot open file");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        ui->lab_info->setText("JSON is not an object");
        return;
    }

    QJsonObject jsonObject = doc.object();
    this->data->SetData(jsonObject);
    SetDataToModel(jsonObject);
    ui->lab_info->setText("Изменения отменены.");
}

void InputData::SetDataToModel(const QJsonObject& jsonObject) {
    model->clear();
    model->setHorizontalHeaderLabels({"Key", "Value"});
    addJsonToItem(jsonObject, model->invisibleRootItem());
    ui->treeView->setModel(model);
    ui->treeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    ui->treeView->expandAll();
    ui->treeView->show();
}

QJsonObject InputData::NormalizedLists(QJsonObject input) {
    QJsonObject normalized;
    for (auto it = input.begin(); it != input.end(); ++it) {
        const QString key = it.key();
        const QJsonValue value = it.value();

        if (value.isArray()) {
            normalized.insert(key, value);
        } else {
            QJsonArray arr;
            arr.append(value.toString());
            normalized.insert(key, arr);
        }
    }
    return normalized;
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
    // Если нет дочерних элементов или одна строка - это примитив
    if (!item->hasChildren() || (!item->child(0, 0)->hasChildren() && item->rowCount() == 1)) {
        QString text;
        if (item->rowCount() == 1)
            text = item->child(0, 1)->text();
        else
            text = item->text();
        bool is_num;
        int num = text.toInt(&is_num);
        if (is_num)
            return num;
        else
            return text;
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

