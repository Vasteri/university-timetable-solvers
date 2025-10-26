#include "graph.h"
#include "ui_graph.h"

graph::graph(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::graph)
{
    ui->setupUi(this);
}

graph::~graph()
{
    delete ui;
}
