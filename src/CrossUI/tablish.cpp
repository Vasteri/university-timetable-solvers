#include "tablish.h"
#include "ui_tablish.h"

#include <QJsonArray>
#include <QSortFilterProxyModel>
#include <QMenu>

//#include "QScheduleSortModel.cpp"

Tablish::Tablish(QWidget *parent, GlobalDataTransition* data)
    : QWidget(parent)
    , ui(new Ui::Tablish)
{
    ui->setupUi(this);
    this->data = data;
    fill_empty = false;
    init_response_api(ui->lab_info, ui->pushButton);
    init_table_view();

    connect(ui->but_sort_default, &QPushButton::clicked, this, [this](){
        ui->table_view_res->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        proxy->sort(2);
        proxy->sort(1);
        proxy->sort(0);
    });

    connect(ui->but_empty_rows, &QPushButton::clicked, this, &Tablish::fill_or_clear_empty_in_table);
}

Tablish::~Tablish()
{
    delete ui;
    delete data;
    delete model;
}

void Tablish::fill_or_clear_empty_in_table() {
    if (!fill_empty) {
        fill_empty = true;
        QSet<QString> groups = uniqueValuesForColumn(0);
        QSet<QString> days = uniqueValuesForColumn(1);
        QSet<QString> times = uniqueValuesForColumn(2);
        QSet<QString> existing;
        for (int r = 0; r < model->rowCount(); ++r) {
            QString key = model->item(r, 0)->text() + "|" +
                          model->item(r, 1)->text() + "|" +
                          model->item(r, 2)->text();
            existing.insert(key);
        }

        for (auto g = groups.begin(); g != groups.end(); ++g) {
            for (auto d = days.begin(); d != days.end(); ++d) {
                for (auto t = times.begin(); t != times.end(); ++t) {
                    QString key = *g + "|" + *d + "|" + *t;
                    if (!existing.contains(key)) {
                        QList<QStandardItem*> row;
                        row << new QStandardItem(*g)
                            << new QStandardItem(*d)
                            << new QStandardItem(*t)
                            << new QStandardItem("")
                            << new QStandardItem("")
                            << new QStandardItem("");
                        model->appendRow(row);
                    }
                }
            }
        }
    }
    else {
        fill_empty = false;
        for (int i = model->rowCount() - 1; i >= 0; --i) {
            bool isEmptyRow = true;
            for (int j = 3; j < model->columnCount(); ++j) {
                if (!model->item(i, j)->text().isEmpty()) {
                    isEmptyRow = false;
                    break;
                }
            }
            if (isEmptyRow)
                model->removeRow(i);
        }
    }
}

void Tablish::init_response_api(QLabel *lab_api, QPushButton *but_api) {
    connect(but_api, &QPushButton::clicked, this, [this, lab_api](){
        //lab_api->setText(err);
        QJsonObject obj = this->data->GetData();

        if (obj.contains("result") && obj["result"].isArray()) {
            QJsonArray jsonArray = obj["result"].toArray();
            table_data_update(jsonArray);
        }
        else if (obj.isEmpty()){
            lab_api->setText(QString("Статус: Нет данных"));
        }
        else {
            lab_api->setText(QString("Статус: Incorrect json format"));
        }
    });
}

void Tablish::init_table_view() {
    model = new QStandardItemModel;
    headers = {};
    model->setHorizontalHeaderLabels(headers);

    proxy = new QScheduleSortModel;
    proxy->setSourceModel(model);
    connect(ui->table_view_res->horizontalHeader(), &QHeaderView::sectionDoubleClicked, this, &Tablish::onHeaderClicked);
    proxy->setDynamicSortFilter(true);

    ui->table_view_res->setModel(proxy);
    ui->table_view_res->setSortingEnabled(true);
}

void Tablish::table_data_update(QJsonArray jsonArray) {
    model->clear();
    fill_empty = false;
    if (jsonArray.size() > 0) {
        //headers = jsonArray.first().toObject().keys();
        headers = {"group", "day", "time", "subject", "room", "teacher"};
        model->setHorizontalHeaderLabels(headers);
    }
    for (int i = 1; i < jsonArray.size(); ++i) {
        const QJsonArray elem = jsonArray.at(i).toArray();
        QList<QStandardItem*> row;
        for (int j = 0; j < headers.size(); ++j)
            row << new QStandardItem(elem.at(j).toString());
        model->appendRow(row);
    }
    ui->table_view_res->resizeColumnsToContents();
}



QSet<QString> Tablish::uniqueValuesForColumn(int column) {
    QSet<QString> res;
    for (int r = 0; r < model->rowCount(); ++r)
        res.insert(model->index(r, column).data().toString());
    return res;
}

void Tablish::onHeaderClicked(int column) {
    QSet<QString> values = uniqueValuesForColumn(column);

    QMenu menu;
    menu.setToolTipsVisible(true);

    QAction *selectAll = menu.addAction("Выбрать всё");
    selectAll->setCheckable(true);
    QAction *removeAll = menu.addAction("Убрать всё");
    removeAll->setCheckable(true);
    menu.addSeparator();

    // динамически создаём чекбоксы
    QList<QAction*> actions;
    for (auto v = values.begin(); v != values.end(); ++v) {
        QAction *a = menu.addAction(*v);
        a->setCheckable(true);
        QSet<QString> prev = proxy->allowValues.value(column);
        a->setChecked(prev.contains(*v));
        actions.append(a);
    }

    QAction *res = menu.exec(QCursor::pos());
    if (!res) return; // пользователь кликнул в пустое место

    // результат пользователя
    QSet<QString> chosen;
    if (selectAll->isChecked()) {
        for (QAction *a : actions)
            chosen.insert(a->text());
    }
    else if (!removeAll->isChecked()) {
        for (QAction *a : actions)
            if (a->isChecked())
                chosen.insert(a->text());
    }

    proxy->setAllowedValues(column, chosen);
}
