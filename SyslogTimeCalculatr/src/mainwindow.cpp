#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <cmath>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::calc(QList<double> timeStamps)
{

    double sum = std::accumulate(timeStamps.constBegin(), timeStamps.constEnd(), 0.0);
    double average = sum / timeStamps.size();
    double avg=0;

    // Calculate maximum and minimum
    double maximum = *std::max_element(timeStamps.constBegin(), timeStamps.constEnd());
    double minimum = *std::min_element(timeStamps.constBegin(), timeStamps.constEnd());

    // Calculate standard deviation
    double sumSquares = 0.0;
    for (double timestamp : timeStamps) {
        sumSquares += std::pow(timestamp - average, 2);
    }

    double variance = sumSquares / timeStamps.size();
    double standardDeviation = std::sqrt(variance);
    avg=average;
    switch (ui->CmbDimension->currentIndex()) {
    case 0:

        break;
    case 1:
      average*=1000;
      standardDeviation*=1000;
      minimum*=1000;
      maximum*=1000;
        break;

case 2:
        average*=1000000;
        standardDeviation*=1000000;
        minimum*=1000000;
        maximum*=1000000;
        break;

    case 3:
        average*=1000000000;
        standardDeviation*=1000000000;
        minimum*=1000000000;
        maximum*=1000000000;
        break;

    }
    return  " AVG(F)="+QString::number(1/avg,'f', 2)+" Hz AVG(T)="+QString::number(average,'f', 9)+" std(T)="+QString::number(standardDeviation,'f', 9)+
            " min(T)="+QString::number(minimum,'f', 9)+" Tmax(T)="+QString::number(maximum,'f', 9)+" count="+QString::number(timeStamps.length());


}
void MainWindow::on_BtnCalc_clicked()
{
    QString input=ui->TxtInput->toPlainText().trimmed();
    QList<double> timeStamps;
    QList<double> deltaT;


    QStringList lines=input.split("\n");
    for(int i=0;i<lines.count();i++) {
        if(lines[i].contains(ui->TxtServiceName->text()) && lines[i].contains("sec=")){
               timeStamps.push_back(lines[i].split("sec=")[1].toDouble());
    }
    }
    for(int i=1;i<timeStamps.count();i++)
    {
    deltaT.push_back(timeStamps[i]-timeStamps[i-1]);
    }

    ui->TxtResults->setText(calc(deltaT));


}

void MainWindow::on_BtnClear_clicked()
{
    ui->TxtInput->clear();
}
