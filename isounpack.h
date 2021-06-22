#ifndef UNPACKISO_H
#define UNPACKISO_H

#include <QThread>
#include <QDir>
#include <QProcess>
#include <QTextStream>
#include <QDebug>
#include <QTemporaryDir>

class ISOUnpack : public QThread
{
    Q_OBJECT

public:
    ISOUnpack(QString iso,
               QString destanationPartition,
               QObject *parent = 0)
        : QThread(parent)
        , isoPath(iso)
        , destanationPart(destanationPartition)
    {}

    void run()
    {
        tempDir = new QTemporaryDir("tempMPoint"); //partition folder
        tempDir->setAutoRemove(false);

        emit statusBarMessage("Mounting partition...",0);

        QProcess::execute("mount",QStringList() << destanationPart << tempDir->path());

        QPair<QPair<QStringList,QStringList>,QStringList> filesList = getFilesNamesInISOFile(isoPath);
        pathCreate(filesList.second,tempDir->path());

        emit progressBarRangeSignal(0,filesList.first.first.length()); //sends numbers of files
        int currentProgressState = 1;

        for(int i = 0; i < filesList.first.first.size(); ++i)
        {
            emit statusBarMessage("Copying: " + filesList.first.first.at(i) + "(" + filesList.first.second.at(i) + ")" ,0); //to statusbar
            extractFile(filesList.first.first.at(i),tempDir->path() + "/" + filesList.first.first.at(i),isoPath);
            emit updateProgress(currentProgressState++);  //update state progressbar
        }
        //resets to default
        emit statusBarMessage("Synchronization...",0);
        QProcess::execute("sync");
        emit statusBarMessage("Unmounting partition...",0);
        QProcess::execute("umount",QStringList() << destanationPart);
        sleep(2);
        removeTempDir();
        emit progressBarRangeSignal(0,100);
        emit updateProgress(0);
        emit statusBarMessage("Done!",3000);
        emit finishCopy();
    }
    void pathCreate(QStringList dirList, QString dst)
    {
        QDir dir;
        foreach (QString d, dirList) {
            QString dst_path = dst + QDir::separator() + d;
            dir.mkpath(dst_path);
        }
    }
    QPair<QPair<QStringList,QStringList>,QStringList> getFilesNamesInISOFile(QString archivefile) //returns <<filenames,size>,directories>
    {
        QProcess p;
        p.start(sevenZCommand,QStringList() << "-bd" << "-slt" << "l" << archivefile);
        p.waitForFinished();

        QString processOutput = p.readAll();
        QTextStream processOutputTextStream(&processOutput);

        QStringList fileNameWithDirectoryList;
        QStringList directoriesList;
        QStringList fileSizesList; // formated size

        QString line;
        QString tmplsN;
        QString fileSize;

        QLocale locale; //to KiB,Mib,GiB

        while (!processOutputTextStream.atEnd())
        {
            line = processOutputTextStream.readLine();
            if (line.contains("Path = "))
            {
                if (line.contains("Path = [BOOT]"))
                    continue;
                if (line == QString("Path = %1").arg(QFileInfo(archivefile).absoluteFilePath()))
                    continue;
                if (line == QString("Path = %1").arg(QDir::toNativeSeparators(QFileInfo(archivefile).absoluteFilePath())))
                    continue;
                tmplsN = processOutputTextStream.readLine();
                if (tmplsN.contains("Folder = 1") || tmplsN.contains("Folder = +"))
                {
                    directoriesList.append(line.remove("Path = "));
                }
                else
                {
                    fileSize = QString(processOutputTextStream.readLine()).remove("Size = ").trimmed();
                    fileSizesList.append(locale.formattedDataSize(fileSize.toULongLong()));
                    fileNameWithDirectoryList.append(line.remove("Path = "));

                }
            }
        }
        return qMakePair(qMakePair(fileNameWithDirectoryList,fileSizesList),directoriesList);
    }

    void removeTempDir(){
        tempDir->remove();
    }
signals:
    void updateProgress(int i);
    void progressBarRangeSignal(int zero,int size);
    void statusBarMessage(QString message,int timeout);
    void finishCopy();
private:
    QTemporaryDir *tempDir;
    QString sevenZCommand = "7z";
    QString isoPath,destanationPart;

    QStringList filesName,filesSize,directories;

    void extractFile(QString fileName, QString outputFileName, QString archivefile){ //to unpack filename,outputdir+filename,iso
        QString destindir = QFileInfo(outputFileName).dir().absolutePath();
        QProcess::execute(sevenZCommand,QStringList() << "-bd" << "-aos" << "-o"+destindir << "e"
                          << archivefile << fileName);
    }

};


#endif // UNPACKISO_H
