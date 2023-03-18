#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QString"
#include "multhread.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    statusBar()->showMessage("Welcome to Sniffer");
    ui->toolBar->addAction(ui->actionRun_And_Stop);
    ui->toolBar->addAction(ui->actionClear);
    countNumber = 0;
    numberRow = -1;

    ShowNetworkCard();
    multhread *thread = new multhread;
    static bool index = false;
    connect(ui->actionRun_And_Stop, &QAction::triggered, this, [=](){
        index = !index;
        if(index){
            // 开始
            ui->tableWidget->clearContents();
            ui->tableWidget->setRowCount(0);
            countNumber = 0;
            int dataSize = this->pData.size();
            for(int i = 0;i < dataSize; i++){
                free((char*)(this->pData[i].pkt_content));
                this->pData[i].pkt_content = nullptr;
            }
            QVector<DataPackage>().swap(pData);

            int res = Capture();
            if(res != -1 && pointer){

                thread->setPointer(pointer);
                thread->setFlag();
                thread->start();
                ui->actionRun_And_Stop->setIcon(QIcon(":/src/pause.png"));
                ui->comboBox->setEnabled(false);
            }else{
                index = !index;
                countNumber = 0;
            }
        }
        else{
            // 结束
            thread->resetFlag();
            thread->quit();
            thread->wait();
            ui->actionRun_And_Stop->setIcon(QIcon(":/src/start.png"));
            ui->comboBox->setEnabled(true);
            pcap_close(pointer);
            pointer = nullptr;
        }
    });

    connect(thread, &multhread::send, this, &MainWindow::HandleMessage);
    ui->toolBar->setMovable(false);
    ui->tableWidget->setColumnCount(7);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(30);
    QStringList title = {"序号", "时间", "源地址", "目的地址", "协议", "长度", "信息"};
    ui->tableWidget->setHorizontalHeaderLabels(title);
    ui->tableWidget->setColumnWidth(0, 30);
    ui->tableWidget->setColumnWidth(1, 60);
    ui->tableWidget->setColumnWidth(2, 150);
    ui->tableWidget->setColumnWidth(3, 150);
    ui->tableWidget->setColumnWidth(4, 40);
    ui->tableWidget->setColumnWidth(5, 50);
    ui->tableWidget->setColumnWidth(6, 700);

    ui->tableWidget->setShowGrid(false);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->treeWidget->setHeaderHidden(true);
}

MainWindow::~MainWindow()
{
    int dataSize = pData.size();
    for(int i = 0; i < dataSize; i++){
        free((char*)(this->pData[i].pkt_content));
        this->pData[i].pkt_content = nullptr;
    }
    QVector<DataPackage>().swap(pData);
    delete ui;
}

void MainWindow::ShowNetworkCard(){
    int n = pcap_findalldevs(&all_device, errbuf);
    if(n == -1){
        ui->comboBox->addItem("error:" + QString(errbuf));
    }
    else{
        ui->comboBox->clear();
        ui->comboBox->addItem("Please Choose Card!");
        for(device = all_device; device != nullptr; device = device->next){
            QString  device_name = device->name;
            device_name.replace("\\Device\\", "");
            QString des = device->description;
            QString item = device_name + des;
            ui->comboBox->addItem(item);
        }
    }
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    int i = 0;
    if(index != 0){
        for(device = all_device; i < index - 1; device = device->next, i++){}
    }
    return;
}

int MainWindow::Capture(){
    if(device){
        pointer = pcap_open_live(device->name, 65536, 1, 1000, errbuf);
    }
    else return -1;
    if(!pointer){
        pcap_freealldevs(all_device);
        device = nullptr;
        return -1;
    }
    else {
        if(pcap_datalink(pointer) != DLT_EN10MB){
            pcap_close(pointer);
            pcap_freealldevs(all_device);
            device = nullptr;
            pointer = nullptr;
            return -1;
        }
        statusBar()->showMessage(device->name);
    }
    return 0;
}

void MainWindow::HandleMessage(DataPackage data){
    ui->tableWidget->insertRow(countNumber);
    this->pData.push_back(data);
    QString type = data.getPackageType();
    QColor color;
    if(type == "TCP")
        color = QColor(216, 191, 216);
    else if(type == "UDP")
        color = QColor(144, 238, 144);
    else if(type == "ARP")
        color = QColor(238, 238, 0);
    else if(type == "DNS")
        color = QColor(255, 255, 224);
    else
        color = QColor(255, 218, 185);

    ui->tableWidget->setItem(countNumber, 0, new QTableWidgetItem(QString::number(countNumber)));
    ui->tableWidget->setItem(countNumber, 1, new QTableWidgetItem(data.getTimeStamp()));
    ui->tableWidget->setItem(countNumber, 2, new QTableWidgetItem(data.getSource()));
    ui->tableWidget->setItem(countNumber, 3, new QTableWidgetItem(data.getDestination()));
    ui->tableWidget->setItem(countNumber, 4, new QTableWidgetItem(type));
    ui->tableWidget->setItem(countNumber, 5, new QTableWidgetItem(data.getDataLength()));
    ui->tableWidget->setItem(countNumber, 6, new QTableWidgetItem(data.getInfo()));
    for(int i=0; i < 7; i++){
        ui->tableWidget->item(countNumber, i)->setBackground(color);
    }
    countNumber++;

}

void MainWindow::on_tableWidget_cellClicked(int row, int column)
{
    if(row == numberRow || row < 0){
        return;
    }
    else{
        ui->treeWidget->clear();
        numberRow = row;
        if(numberRow < 0 || numberRow > countNumber){
            return;
        }
        QString desMac = pData[numberRow].getDesMacAddr();
        QString srcMac = pData[numberRow].getSrcMacAddr();
        QString type = pData[numberRow].getMacType();
        QString tree = "Ethernet, Src: " + srcMac + " Dst: " + desMac;
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << tree);
        ui->treeWidget->addTopLevelItem(item);
        item->addChild(new QTreeWidgetItem(QStringList() << "Source: " + srcMac));
        item->addChild(new QTreeWidgetItem(QStringList() << "Destinaiton: " + desMac));
        item->addChild(new QTreeWidgetItem(QStringList() << "Type: " + type));
    }


}

