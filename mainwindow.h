#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "driveprep.h"
#include "isounpack.h"

#include <QDir>
#include <QFile>
#include <QMainWindow>
#include <QTextEdit>
#include <QThread>
#include <QDebug>
#include <QProgressBar>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_refreshDevicesListButton_pressed();
    void on_deviceInfoButton_pressed();
    void on_writeButton_clicked();
    void on_chooseImageButton_clicked();

    QStringList getUSBDrivesList(bool partition);
    void umountAllPartitions(QString drive);


    void on_stopButton_clicked();

    void blockUIElements();
    void unblockUIElements();
private:
    Ui::MainWindow *ui;
    QProgressBar *progressBar;
    QStatusBar *statusBar;

    DrivePrep *drivePrepare;
    ISOUnpack *unpackIso;
};

#endif // MAINWINDOW_H
