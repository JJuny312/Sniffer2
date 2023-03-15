#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <stdio.h>
#include "pcap.h"
#include "winsock2.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void ShowNetworkCard();
    int Capture();

private slots:
    void on_comboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    pcap_if_t *all_device;
    pcap_if_t *device;
    pcap_t *pointer;
    char errbuf[PCAP_ERRBUF_SIZE];
};
#endif // MAINWINDOW_H
