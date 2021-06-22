#ifndef DRIVEPREP_H
#define DRIVEPREP_H

#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <QDebug>

class DrivePrep : public QThread{
    Q_OBJECT
public:
    DrivePrep(QString drive,bool isuefi ,QObject *parent = 0)
        :QThread(parent),driveName(drive),isUEFI(isuefi)
    {
    }
    void run(){
        getFullSize();

        mssysfile = new QFile(":/tools/ms-sys");
        uefintfs = new QFile(":/tools/uefi-ntfs.img");
        tempDir = new QTemporaryDir("bootieTools"); //partition folder

        QFile::copy(mssysfile->fileName(),tempDir->path() + "/ms-sys");
        QFile::copy(uefintfs->fileName(),tempDir->path() + "/uefi-ntfs.img");

        tempDir->setAutoRemove(false);

        //sleep is important!must be a little pause between operations
        emit setRangeProgressBar(0,7);
        emit updateProcess(1);
        emit updateStatusMessage("Wiping partitions...",0);
        QProcess::execute("wipefs",QStringList() << "-a" << driveName);
        emit updateProcess(2);
        sleep(2);

        emit updateProcess(3);
        emit updateStatusMessage("Creating partition...",0);
        if(isUEFI==true){
            QProcess::execute("parted",QStringList() << driveName << "mklabel" << "gpt" <<
                              "mkpart" << "primary" << "ntfs" << "2048s" << ntfsSize <<
                              "mkpart" << "primary" << "fat16" << fatSize << endSize
                              << "-s");
            emit updateProcess(4);
            sleep(2);
            emit updateProcess(5);
            emit updateStatusMessage("Formating...",0);
            QProcess::execute("mkfs.ntfs",QStringList() << "-Q" << driveName+"1");
            QProcess::execute("ntfslabel",QStringList() << driveName+"1" << "WBOOTUSB");
            sleep(2);
            QProcess::execute("mkfs.fat",QStringList() << "-F16" << driveName+"2");
            emit updateStatusMessage("Writing UEFI-NTFS...",0);
            QProcess::execute("dd",QStringList() << "if="+tempDir->path()+"/uefi-ntfs.img" << "of="+driveName+"2");
            emit updateProcess(6);
        }
        else{

            QProcess::execute("chmod",QStringList() << "+x" << tempDir->path()+"/ms-sys");

            QProcess::execute("parted",QStringList() << driveName << "mklabel" << "msdos" <<
                                           "mkpart" << "primary" << "ntfs" << "0%" << "100%" << "set" << "1" << "boot" << "on" << "-s");    
            emit updateProcess(4);
            sleep(2);
            emit updateProcess(5);
            emit updateStatusMessage("Formating...",0);
            QProcess::execute("mkfs.ntfs",QStringList() << "-f" << driveName+"1");
            QProcess::execute("ntfslabel",QStringList() << driveName+"1" << "WBOOTUSB");
            emit updateProcess(6);
            sleep(2);
            emit updateStatusMessage("Setting up MBR...",0);
            QProcess::execute(tempDir->path()+"/ms-sys",QStringList() << "-7" << driveName);
            sleep(2);
            QProcess::execute(tempDir->path()+"/ms-sys",QStringList() << "-n" << driveName+"1");
        }

        emit updateProcess(7);
        removeTempDir();
        emit updateStatusMessage("Done!Starting copying...",2000);
        sleep(2);
        emit drivePrepDone();
    }
    void removeTempDir(){
        tempDir->remove();
    }

    void getFullSize(){
        QProcess p;
        p.start("parted",QStringList() << driveName << "unit" << "s" << "print");
        p.waitForFinished();
        QString processOutput = p.readAll();
        QTextStream processOutputTextStream(&processOutput);
        QString line;
            while(!processOutputTextStream.atEnd()){
            line = processOutputTextStream.readLine();
            if(line.contains("Disk " + driveName + ":")){
                QStringList words = line.split(" ");
                QString number_str = words.at(words.length() - 1);
                number_str.chop(1);
                long fullS = number_str.toInt();

                long ntfsS = fullS - 2049; //counting 1st partitions size(sectors 512b)
                //than we've got 512 kb for fat16 part
                long fatS = ntfsS + 1; //start sector for fatpart.
                long endS = fullS - 1025; //leave 512 kb of free mem.

                fullSize = QString::number(fullS)+"s";
                ntfsSize = QString::number(ntfsS)+"s";
                fatSize = QString::number(fatS)+"s";
                endSize = QString::number(endS)+"s";
                qDebug() << fullS;

            }
        }
    }

signals:
    void updateProcess(int i);
    void updateStatusMessage(QString s,int i);
    void drivePrepDone();
    void setRangeProgressBar(int zero,int value);
private:
    QTemporaryDir *tempDir;
    QFile *mssysfile,*uefintfs;
    QString driveName,blocksNumber;
    QString fullSize,ntfsSize,fatSize,endSize;
    bool isUEFI;

};

#endif // DRIVEPREP_H
