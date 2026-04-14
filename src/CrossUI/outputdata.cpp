#include "outputdata.h"
#include "ui_outputdata.h"
#include "globaldatatransition.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

//#include "xlsxdocument.h"

//QJsonObject OutputData::loadExcelFromFile(const QString &path) const

OutputData::OutputData(QWidget *parent, GlobalDataTransition* data)
    : QWidget(parent)
    , ui(new Ui::OutputData)
    , data(data)
    , currentIndex(-1)
{
    ui->setupUi(this);

    // QComboBox *combo_objects;
    // QTextEdit *text_display;
    // QPushButton *btn_save_json;
    // QPushButton *btn_save_csv;
    // QPushButton *btn_save_excel;
    // QPushButton *btn_load;
    // QLabel *lab_info;

    connect(ui->combo_objects, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OutputData::onObjectSelected);
    connect(ui->btn_save_json, &QPushButton::clicked, this, &OutputData::saveAsJson);
    connect(ui->btn_save_csv, &QPushButton::clicked, this, &OutputData::saveAsCsv);
    connect(ui->btn_load, &QPushButton::clicked, this, &OutputData::loadFromFile);
    connect(ui->btn_delete, &QPushButton::clicked, this, &OutputData::deleteSelected);
    //connect(ui->btn_save_excel, &QPushButton::clicked, this, &OutputData::saveAsExcel);

    // Сигнал получения нового решения
    connect(this->data, &GlobalDataTransition::Received, this, &OutputData::onReceived);
}

OutputData::~OutputData()
{
    delete ui;
}

// получение нового решения
void OutputData::onReceived()
{
    QJsonObject obj = this->data->GetData();
    addObject(obj);

    ui->lab_info->setText(this->data->message + " " + obj["status"].toString());
}

// Добавление нового объекта в хранилище
void OutputData::addObject(const QJsonObject &obj)
{
    // описание
    QString desc = (tr("#%1: ").arg(objects.size() + 1)
                    + obj["method"].toString() + " "
                    + obj["status"].toString());

    objects.append(obj);
    descriptions.append(desc);
    ui->combo_objects->addItem(desc);

    // Автоматически выбираем последний добавленный объект
    int newIndex = objects.size() - 1;
    ui->combo_objects->setCurrentIndex(newIndex);
    onObjectSelected(newIndex);
}

// Выбор объекта из выпадающего списка
void OutputData::onObjectSelected(int index)
{
    if (index < 0 || index >= objects.size()) {
        currentIndex = -1;
        ui->text_display->clear();
        return;
    }

    currentIndex = index;
    const QJsonObject &obj = objects[currentIndex];

    // Глобальная установка(во всех вкладках приложения)
    this->data->SetDataResult(obj);

    // Форматированный вывод JSON в QTextEdit
    QJsonDocument doc(obj);
    QString prettyJson = doc.toJson(QJsonDocument::Indented);
    ui->text_display->setPlainText(prettyJson);
}

void OutputData::deleteSelected()
{
    if (currentIndex < 0) {
        QMessageBox::warning(this, "Ошибка", "Нет выбранного объекта");
        return;
    }

    objects.removeAt(currentIndex);
    descriptions.removeAt(currentIndex);
    ui->combo_objects->removeItem(currentIndex);

    if (objects.isEmpty()) {
        currentIndex = -1;
        ui->text_display->clear();
    } else {
        int newIdx = qMin(currentIndex, objects.size() - 1);
        ui->combo_objects->setCurrentIndex(newIdx);
        onObjectSelected(newIdx);
    }
}

void OutputData::saveAsJson()
{
    if (currentIndex < 0) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не выбран объект для сохранения."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, tr("Сохранить как JSON"),
                                                QString(), tr("JSON files (*.json)"));
    if (path.isEmpty())
        return;

    if (!path.endsWith(".json", Qt::CaseInsensitive)) path += ".json";

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось создать файл."));
        return;
    }

    QJsonDocument doc(objects[currentIndex]);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, tr("Успех"), tr("Объект сохранён в JSON."));
}

void OutputData::saveAsCsv()
{
    if (currentIndex < 0) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не выбран объект для сохранения."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, tr("Сохранить как CSV"),
                                                QString(), tr("CSV files (*.csv)"));
    if (path.isEmpty())
        return;

    if (!path.endsWith(".csv", Qt::CaseInsensitive)) path += ".csv";

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось создать файл."));
        return;
    }

    QTextStream out(&file);
    const QJsonObject &obj = objects[currentIndex];

    // Таблица result
    QJsonArray resultArray = obj["result"].toArray();
    if (!resultArray.isEmpty()) {
        for (const QJsonValue &rowVal : resultArray) {
            QJsonArray row = rowVal.toArray();
            QStringList rowStrings;
            for (const QJsonValue &cell : row) {
                QString cellStr = cell.toString();
                // Экранирование: если есть запятая, кавычка или перевод строки
                if (cellStr.contains(',') || cellStr.contains('"') || cellStr.contains('\n'))
                    cellStr = '"' + cellStr.replace('"', "\"\"") + '"';
                rowStrings << cellStr;
            }
            out << rowStrings.join(',') << "\n";
        }
    }

    file.close();
    QMessageBox::information(this, tr("Успех"), tr("Объект сохранён в CSV."));
}

// Загрузка из файла (JSON, CSV, Excel)
void OutputData::loadFromFile()
{
    QString filter = tr("Все поддерживаемые (*.json *.csv *.xlsx);;JSON (*.json);;CSV (*.csv);;Excel (*.xlsx)");
    QString path = QFileDialog::getOpenFileName(this, tr("Загрузить объект"), QString(), filter);
    if (path.isEmpty())
        return;

    QJsonObject loadedObj;
    if (path.endsWith(".json", Qt::CaseInsensitive)) {
        loadedObj = loadJsonFromFile(path);
    } else if (path.endsWith(".csv", Qt::CaseInsensitive)) {
        loadedObj = loadCsvFromFile(path);
    } else if (path.endsWith(".xlsx", Qt::CaseInsensitive)) {
        throw std::exception();
        //loadedObj = loadExcelFromFile(path);
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Неподдерживаемый формат файла."));
        return;
    }

    if (loadedObj.isEmpty()) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось загрузить объект из файла."));
        return;
    }

    addObject(loadedObj);
    QMessageBox::information(this, tr("Успех"), tr("Объект загружен и добавлен в список."));
}

QJsonObject OutputData::loadJsonFromFile(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QJsonObject();

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject())
        return QJsonObject();

    return doc.object();
}

// CSV (первая строка - заголовки, вторая - значения)
QJsonObject OutputData::loadCsvFromFile(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QJsonObject();

    QTextStream in(&file);

    // Парсим таблицу
    QJsonArray resultArray;
    for (int i = 0; !in.atEnd(); ++i) {
        QString line = in.readLine();
        QStringList cells;
        bool inQuotes = false;
        QString current;
        for (int j = 0; j < line.size(); ++j) {
            QChar ch = line[j];
            if (ch == '"') {
                if (inQuotes && j+1 < line.size() && line[j+1] == '"') {
                    // экранированная кавычка
                    current += '"';
                    j++;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (ch == ',' && !inQuotes) {
                cells << current.trimmed();
                current.clear();
            } else {
                current += ch;
            }
        }
        cells << current.trimmed(); // последнее поле

        // Убираем обрамляющие кавычки
        for (QString &cell : cells) {
            if (cell.startsWith('"') && cell.endsWith('"'))
                cell = cell.mid(1, cell.length() - 2);
            cell.replace("\"\"", "\"");
        }

        QJsonArray rowArray;
        for (const QString &cell : cells)
            rowArray.append(cell);
        resultArray.append(rowArray);
    }

    QJsonObject obj;
    obj["result"] = resultArray;
    return obj;
}
