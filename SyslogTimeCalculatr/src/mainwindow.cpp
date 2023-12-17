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

QString calc(QList<double> timeStamps)
{

    double sum = std::accumulate(timeStamps.constBegin(), timeStamps.constEnd(), 0.0);
    double average = sum / timeStamps.size();

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

    return  "AVG="+QString::number(average)+" std="+QString::number(standardDeviation)+
            " min="+QString::number(minimum)+" max="+QString::number(maximum)+" count="+QString::number(timeStamps.length());


}
void MainWindow::on_BtnCalc_clicked()
{
    QString input=ui->TxtInput->toPlainText().trimmed();
    QList<double> timeStamps;


    QStringList lines=input.split("\n");
    for(int i=1;i<lines.count();i++)
    {

        if(lines[i].contains(ui->TxtServiceName->text()) && lines[i].contains("sec="))
        {

            timeStamps.push_back(lines[i].split("sec=")[1].toDouble()-lines[i-1].split("sec=")[1].toDouble());
        }

    }

    ui->TxtResults->setText(calc(timeStamps));


}
