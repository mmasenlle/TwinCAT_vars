
#include <QTableWidget>
#include <QHeaderView>
#include <QTimer>
//#include <QToolButton>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "adscomm.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QTableWidget *tableWidget = ui->tableWidget; //new QTableWidget(this);
    //tableWidget->setRowCount(10);
    tableWidget->setColumnCount(9);
    tableWidget->setColumnWidth(0, 200);
    QStringList tableHeader; tableHeader<<"Name"<<"Group"<<"@Mem"<<"dataType"<<"Size"<<"Type"<<"Comment"<<"Value"<<"Force";
     tableWidget->setHorizontalHeaderLabels(tableHeader);
     tableWidget->verticalHeader()->setVisible(false);

//     setCentralWidget(tableWidget);

     AdsComm *adsComm = new AdsComm(tableWidget);
     QObject::connect(tableWidget, SIGNAL(cellChanged(int, int)), adsComm, SLOT(change(int, int)));

     adsComm->label = ui->label;
     adsComm->setAddr("");
     ui->lineEdit->setText(adsComm->getAddr());
     connect(ui->lineEdit, &QLineEdit::editingFinished, [adsComm,this](){
         adsComm->setAddr(ui->lineEdit->text());
         ui->lineEdit->setText(adsComm->getAddr());
        adsComm->init();});

     adsComm->init();

     QTimer *timer = new QTimer(this);
     QObject::connect(timer, SIGNAL(timeout()), adsComm, SLOT(update()));

     connect(ui->pushButton, &QPushButton::clicked, [timer](){timer->stop();});
     connect(ui->pushButton_2, &QPushButton::clicked, [timer](){timer->start(100);});

     ui->label->setText("OK");

     timer->start(100);
}

MainWindow::~MainWindow()
{
    delete ui;
}
