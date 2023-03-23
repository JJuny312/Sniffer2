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

    connect(ui->actionClear, &QAction::triggered,this,[=]{
            if(!isStart){

                ui->tableWidget->clearContents();
                ui->tableWidget->setRowCount(0);
                ui->treeWidget->clear();
                countNumber = 0;
                numberRow = -1;
                int dataSize = this->pData.size();
                for(int i = 0;i < dataSize;i++){
                    free((char*)(this->pData[i].pkt_content));
                    this->pData[i].pkt_content = nullptr;
                }
                QVector<DataPackage>().swap(pData);

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
        color = QColor(0xFF, 0x1F, 0x1F);
    else if(type == "UDP")
        color = QColor(0xFF, 0xA7, 0x0F);
    else if(type == "ARP")
        color = QColor(0xE0, 0xFF, 0x09);
    else if(type == "DNS")
        color = QColor(0x0A, 0xFF, 0x2B);
    else if(type == "TLS" || type == "SSL")
        color = QColor(0x10, 0xED, 0xFF);
    else
        color = QColor(0xE8, 0xF7, 0xFF);

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

        QString packageType = pData[numberRow].getPackageType();
        // arp package analysis
        if(packageType == "ARP"){
            QString ArpType = pData[numberRow].getArpOperationCode();
            QTreeWidgetItem*item2 = new QTreeWidgetItem(QStringList()<<"Address Resolution Protocol " + ArpType);
            ui->treeWidget->addTopLevelItem(item2);
            QString HardwareType = pData[numberRow].getArpHardwareType();
            QString protocolType = pData[numberRow].getArpProtocolType();
            QString HardwareSize = pData[numberRow].getArpHardwareLength();
            QString protocolSize = pData[numberRow].getArpProtocolLength();
            QString srcMacAddr = pData[numberRow].getArpSourceEtherAddr();
            QString desMacAddr = pData[numberRow].getArpDestinationEtherAddr();
            QString srcIpAddr = pData[numberRow].getArpSourceIpAddr();
            QString desIpAddr = pData[numberRow].getArpDestinationIpAddr();

            item2->addChild(new QTreeWidgetItem(QStringList()<<"Hardware type:" + HardwareType));
            item2->addChild(new QTreeWidgetItem(QStringList()<<"Protocol type:" + protocolType));
            item2->addChild(new QTreeWidgetItem(QStringList()<<"Hardware size:" + HardwareSize));
            item2->addChild(new QTreeWidgetItem(QStringList()<<"Protocol size:" + protocolSize));
            item2->addChild(new QTreeWidgetItem(QStringList()<<"Opcode:" + ArpType));
            item2->addChild(new QTreeWidgetItem(QStringList()<<"Sender MAC address:" + srcMacAddr));
            item2->addChild(new QTreeWidgetItem(QStringList()<<"Sender IP address:" + srcIpAddr));
            item2->addChild(new QTreeWidgetItem(QStringList()<<"Target MAC address:" + desMacAddr));
            item2->addChild(new QTreeWidgetItem(QStringList()<<"Target IP address:" + desIpAddr));
            return;
        }else { // ip package analysis
            QString srcIp = pData[numberRow].getSrcIpAddr();
            QString desIp = pData[numberRow].getDesIpAddr();

            QTreeWidgetItem*item3 = new QTreeWidgetItem(QStringList()<<"Internet Protocol Version 4, Src:" + srcIp + ", Dst:" + desIp);
            ui->treeWidget->addTopLevelItem(item3);

            QString version = pData[numberRow].getIpVersion();
            QString headerLength = pData[numberRow].getIpHeaderLength();
            QString Tos = pData[numberRow].getIpTos();
            QString totalLength = pData[numberRow].getIpTotalLength();
            QString id = "0x" + pData[numberRow].getIpIdentification();
            QString flags = pData[numberRow].getIpFlag();
            if(flags.size()<2)
                flags = "0" + flags;
            flags = "0x" + flags;
            QString FragmentOffset = pData[numberRow].getIpFragmentOffset();
            QString ttl = pData[numberRow].getIpTTL();
            QString protocol = pData[numberRow].getIpProtocol();
            QString checksum = "0x" + pData[numberRow].getIpCheckSum();
            int pDataLengthofIp = totalLength.toUtf8().toInt() - 20;
            item3->addChild(new QTreeWidgetItem(QStringList()<<"0100 .... = Version:" + version));
            item3->addChild(new QTreeWidgetItem(QStringList()<<".... 0101 = Header Length:" + headerLength));
            item3->addChild(new QTreeWidgetItem(QStringList()<<"TOS:" + Tos));
            item3->addChild(new QTreeWidgetItem(QStringList()<<"Total Length:" + totalLength));
            item3->addChild(new QTreeWidgetItem(QStringList()<<"Identification:" + id));

            QString reservedBit = pData[numberRow].getIpReservedBit();
            QString DF = pData[numberRow].getIpDF();
            QString MF = pData[numberRow].getIpMF();
            QString FLAG = ",";

            if(reservedBit == "1"){
                FLAG += "Reserved bit";
            }
            else if(DF == "1"){
                FLAG += "Don't fragment";
            }
            else if(MF == "1"){
                FLAG += "More fragment";
            }
            if(FLAG.size() == 1)
                FLAG = "";
            QTreeWidgetItem*bitTree = new QTreeWidgetItem(QStringList()<<"Flags:" + flags + FLAG);
            item3->addChild(bitTree);
            QString temp = reservedBit == "1"?"Set":"Not set";
            bitTree->addChild(new QTreeWidgetItem(QStringList()<<reservedBit + "... .... = Reserved bit:" + temp));
            temp = DF == "1"?"Set":"Not set";
            bitTree->addChild(new QTreeWidgetItem(QStringList()<<"." + DF + ".. .... = Don't fragment:" + temp));
            temp = MF == "1"?"Set":"Not set";
            bitTree->addChild(new QTreeWidgetItem(QStringList()<<".." + MF + ". .... = More fragment:" + temp));

            item3->addChild(new QTreeWidgetItem(QStringList()<<"Fragment Offset:" + FragmentOffset));
            item3->addChild(new QTreeWidgetItem(QStringList()<<"Time to Live:" + ttl));
            item3->addChild(new QTreeWidgetItem(QStringList()<<"Protocol:" + protocol));
            item3->addChild(new QTreeWidgetItem(QStringList()<<"Header checksum:" + checksum));
            item3->addChild(new QTreeWidgetItem(QStringList()<<"Source Address:" + srcIp));
            item3->addChild(new QTreeWidgetItem(QStringList()<<"Destination Address:" + desIp));
        }
    }
}

void MainWindow::on_lineEdit_returnPressed()
{
    QString text = ui->lineEdit->text();
    text = text.toUpper();
    QString target = "#";
    if(text == "" || text == "UDP" || text == "TCP" || text == "DNS" || text == "ARP"|| text == "ICMP"|| text == "SSL" || text == "TLS"){
        ui->lineEdit->setStyleSheet("QLineEdit {background-color: rgb(154,255,154);}");
        target = text;
    }else{
        ui->lineEdit->setStyleSheet("QLineEdit {background-color: rgb(250,128,114);}");
    }
    int count = 0;
    int number = ui->tableWidget->rowCount();
    if(!isStart && target != "#"){
        if(target!=""){
            for(int i = 0;i < number;i++){
                if(ui->tableWidget->item(i,4)->text() != target){
                    ui->tableWidget->setRowHidden(i,true);
                }else{
                    ui->tableWidget->setRowHidden(i,false);
                    count++;
                }
            }
        }else{
            int number = ui->tableWidget->rowCount();
            for(int i = 0;i < number;i++){
                ui->tableWidget->setRowHidden(i,false);
                count++;
            }
        }
    }

    double res = 0;
    if(number != 0)
        res = (count*100.0)/number;
    statusBar()->showMessage("Have show (" + QString::number(count) + ") " +QString::number(res,10,2) + "%");
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    QString text = arg1;
    text = text.toLower();
    if(text == "" || text == "udp" || text == "tcp" || text == "dns" || text == "arp" || text == "icmp" || text == "tls" || text == "ssl"){
        ui->lineEdit->setStyleSheet("QLineEdit {background-color: rgb(154,255,154);}");
    }else{
        ui->lineEdit->setStyleSheet("QLineEdit {background-color: rgb(250,128,114);}");
    }
}

void MainWindow::on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if((currentRow != previousRow) && previousRow >= 0){
        on_tableWidget_cellClicked(currentRow,currentColumn);
    }else return;
}
