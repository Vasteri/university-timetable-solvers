#ifndef TABLISH_H
#define TABLISH_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>

#include "QScheduleSortModel.cpp"
#include "globaldatatransition.h"

namespace Ui {
class Tablish;
}

class Tablish : public QWidget
{
    Q_OBJECT

public:
    explicit Tablish(QWidget *parent = nullptr, GlobalDataTransition* data = nullptr);
    ~Tablish();

private:
    Ui::Tablish *ui;
    GlobalDataTransition* data;
    bool fill_empty;

    QStandardItemModel *model;
    QScheduleSortModel *proxy;
    QStringList headers;

    void init_response_api(QLabel *lab_api, QPushButton *but_api);
    void init_table_view();
    void table_data_update(QJsonArray jsonArray);
    void onHeaderClicked(int column);
    void fill_or_clear_empty_in_table();
    QSet<QString> uniqueValuesForColumn(int column);
};

#endif // TABLISH_H
