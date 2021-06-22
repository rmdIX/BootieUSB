#include "driveprep.h"
#include "isounpack.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QProcess>
#include <QResource>
#include <QTemporaryFile>
#include <QTemporaryDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    progressBar = ui->progressBar;
    statusBar = ui->statusBar;
    statusBar->showMessage("Ready");
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_refreshDevicesListButton_pressed()
{
    QStringList usbDrivesList = getUSBDrivesList(false);

        ui->devicesListComboBox->clear();
        if(!usbDrivesList.isEmpty()){
            ui->devicesListComboBox->addItems(usbDrivesList);
            ui->imageSelectGroupBox->setEnabled(true);
        }
        else{
            ui->imageSelectGroupBox->setEnabled(false);
        }


}
void MainWindow::on_deviceInfoButton_pressed() //later
{
}
void MainWindow::on_chooseImageButton_clicked()
{
    ui->imagePath->setText(QFileDialog::getOpenFileName(this, tr("Choose Image"),"/home",tr("ISO Image (*.iso)")));
    if(!ui->imagePath->text().isEmpty()){
        ui->writeOptionsGroupBox->setEnabled(true);
        ui->writeButton->setEnabled(true);
    }
    else{
        ui->writeOptionsGroupBox->setEnabled(false);
        ui->writeButton->setEnabled(false);

    }

}



void MainWindow::on_writeButton_clicked()
{
    QString currentDrive = ui->devicesListComboBox->currentText();
//    QStringRef currentDrive(&cbSelected, 1, 8);

    QString isoFileName = ui->imagePath->text();

    switch(ui->systemComboBox->currentIndex()){
        case 0: //Windows
            if(ui->biosRadioButton->isChecked()) {
                    drivePrepare = new DrivePrep(currentDrive,false);
                    unpackIso = new ISOUnpack(isoFileName,currentDrive+"1");
            }
            else if(ui->uefiRadioButton->isChecked()){
                    drivePrepare = new DrivePrep(currentDrive,true);
                    unpackIso = new ISOUnpack(isoFileName,currentDrive+"1");
            }
            break;
    }



    umountAllPartitions(currentDrive); //before do something,u gotta umount all mounted partititons,since OS automount partititons when u connect usb

    connect(unpackIso,SIGNAL(progressBarRangeSignal(int,int)),progressBar,SLOT(setRange(int,int)));
    connect(unpackIso,SIGNAL(updateProgress(int)),progressBar,SLOT(setValue(int)));
    connect(unpackIso,SIGNAL(statusBarMessage(QString,int)),statusBar,SLOT(showMessage(QString,int)));
    connect(unpackIso,SIGNAL(finishCopy()),this,SLOT(unblockUIElements()));

    connect(drivePrepare,SIGNAL(setRangeProgressBar(int,int)),progressBar,SLOT(setRange(int,int)));
    connect(drivePrepare,SIGNAL(updateProcess(int)),progressBar,SLOT(setValue(int)));
    connect(drivePrepare,SIGNAL(updateStatusMessage(QString,int)),statusBar,SLOT(showMessage(QString,int)));
    connect(drivePrepare,SIGNAL(drivePrepDone()),unpackIso,SLOT(start()));

    blockUIElements();

    drivePrepare->start();

}

void MainWindow::on_stopButton_clicked()
{
    QString currentDrive = ui->devicesListComboBox->currentText();
//    QStringRef currentDrive(&cbSelected, 1, 8);
    if(drivePrepare->isRunning()){
        drivePrepare->removeTempDir();
        drivePrepare->terminate();
    }
    if(unpackIso->isRunning()){
        unpackIso->terminate();
        umountAllPartitions(currentDrive);
        unpackIso->removeTempDir();
    }
    unblockUIElements();
    progressBar->reset();
    statusBar->showMessage("Aborted!",3000);
}

void MainWindow::blockUIElements(){
        ui->writeButton->setEnabled(false);
        ui->stopButton->setEnabled(true);
        ui->imageSelectGroupBox->setEnabled(false);
        ui->usbDevicesGroupBox->setEnabled(false);
        ui->writeOptionsGroupBox->setEnabled(false);
}
void MainWindow::unblockUIElements(){
        ui->writeButton->setEnabled(true);
        ui->stopButton->setEnabled(false);
        ui->imageSelectGroupBox->setEnabled(true);
        ui->usbDevicesGroupBox->setEnabled(true);
        ui->writeOptionsGroupBox->setEnabled(true);
}

QStringList MainWindow::getUSBDrivesList(bool partition){ //returns usb devices list(use 1 to get partitions of device,0 to get device)
    QStringList usbDrivesList;

    QDir pathToAllDrives("/dev/disk/by-id/");
    QFileInfoList pathListWithDrives = pathToAllDrives.entryInfoList(QDir::NoDotAndDotDot|QDir::Files);

        for (int i = 0; i < pathListWithDrives.size(); ++i)
            {
            if (pathListWithDrives.at(i).fileName().contains(QRegExp("^usb-\\S{1,}$")) ||
                pathListWithDrives.at(i).fileName().contains(QRegExp("^mmc-\\S{1,}$")))
                {
//                    usbDrivesList.append(pathListWithDrives.at(i).canonicalFilePath()); //canonicalFilePath(),not fileName()!!!
                switch (partition) {
                case 0:
                    if(!pathListWithDrives.at(i).fileName().contains(QRegExp("part")))
                    {
                        usbDrivesList.append(pathListWithDrives.at(i).canonicalFilePath()); //canonicalFilePath(),not fileName()!!!
                    }
                    break;
                case 1:
                    if(pathListWithDrives.at(i).fileName().contains(QRegExp("part")))
                    {
                        usbDrivesList.append(pathListWithDrives.at(i).canonicalFilePath()); //canonicalFilePath(),not fileName()!!!
                    }
                    break;
                     }
                }
            }
    return usbDrivesList;
}

void MainWindow::umountAllPartitions(QString drive){ //umount all partitions on drive(ex:/dev/sda has /dev/sda1,2,3 etc...umount /dev/sda1,2,3)
   if(!ui->devicesListComboBox->count() == 0){
    QStringList allListPartitions = getUSBDrivesList(true);
    QStringList listPartitions;
    if(!allListPartitions.empty()){
    for (int i = 0; i < allListPartitions.size(); ++i) {
        QString x = allListPartitions[i];
        if(x.contains(drive,Qt::CaseSensitive)){
           listPartitions.append(x);
        }
    }

    QProcess process;
    for (QString x : listPartitions) {
          process.start("umount",QStringList() << x);
          process.waitForFinished();
          process.close();
        }
      }
    else{
        qDebug() << "Can't find partitions for unmount. Skip";
    }
   }
}

