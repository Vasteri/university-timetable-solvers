#ifndef OUTPUTDATA_H
#define OUTPUTDATA_H

#include <QWidget>
#include <QJsonObject>
#include <QVector>

class GlobalDataTransition;

namespace Ui {
class OutputData;
}

class OutputData : public QWidget
{
    Q_OBJECT

public:
    explicit OutputData(QWidget *parent, GlobalDataTransition* data);
    ~OutputData();

private slots:
    void onReceived();                     // Обработка сигнала Received
    void onObjectSelected(int index);      // Выбор объекта из списка
    void saveAsJson();                     // Сохранение в JSON
    void saveAsCsv();                      // Сохранение в CSV
    //void saveAsExcel();                    // Сохранение в Excel (XLSX)
    void loadFromFile();                   // Загрузка объекта из файла
    void deleteSelected();

private:
    void addObject(const QJsonObject &obj);               // Добавление объекта в хранилище
    QString jsonToCsvString(const QJsonObject &obj) const; // Преобразование JSON → CSV строка
    QJsonObject loadJsonFromFile(const QString &path) const;   // Загрузка JSON из файла
    QJsonObject loadCsvFromFile(const QString &path) const;     // Загрузка CSV → JSON
    //QJsonObject loadExcelFromFile(const QString &path) const;   // Загрузка Excel → JSON

    Ui::OutputData *ui;
    GlobalDataTransition *data;            // Указатель на источник данных
    QVector<QJsonObject> objects;          // Хранилище объектов
    QVector<QString> descriptions;         // Список description для отображения
    int currentIndex;                      // Индекс выбранного объекта (-1 если нет)
};

#endif // OUTPUTDATA_H
