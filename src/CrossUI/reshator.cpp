#include "reshator.h"
#include "ui_reshator.h"

Reshator::Reshator(QWidget *parent, GlobalDataTransition* data)
    : QWidget(parent)
    , ui(new Ui::Reshator)
{
    ui->setupUi(this);

    this->data = data;

    connect(ui->but_send, &QPushButton::clicked, this, [this](){
        ui->lab_info->setText("Запрос...");
        this->data->ip = ui->edit_ip->text();
        QJsonObject extra;
        if (ui->comboBox->currentIndex() == 0) {
            extra.insert("method", "milp");
        } else {
            extra.insert("method", "ga");
            QJsonObject ga;
            ga.insert("pop_size", ui->spin_pop_size->value());
            ga.insert("generations", ui->spin_generations->value());
            ga.insert("crossover_rate", ui->dspin_crossover->value());
            ga.insert("mutation_rate", ui->dspin_mutation->value());
            ga.insert("elite_size", ui->spin_elite->value());
            ga.insert("tournament_size", ui->spin_tournament->value());
            int seed = ui->spin_seed->value();
            if (seed >= 0) {
                ga.insert("seed", seed);
            } else {
                ga.insert("seed", QJsonValue::Null);
            }
            extra.insert("ga_params", ga);
        }
        this->data->SendData(extra);
    });

    connect(this->data, &GlobalDataTransition::Received, this, [this](){
        ui->lab_info->setText(this->data->message);
    });
}

Reshator::~Reshator()
{
    delete ui;
    delete data;
}
