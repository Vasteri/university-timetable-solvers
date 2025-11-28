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
        this->data->SendData();
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
