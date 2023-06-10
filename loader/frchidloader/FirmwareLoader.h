#pragma once

#include "HexFileReader.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

class FirmwareLoader : public QObject
{
    Q_OBJECT
public:
    FirmwareLoader(QObject* parent = 0);

    bool setFirmwareFile(const QString& filename);
    bool setSerialPort(const QString& port);

public slots:
    void run();

signals:
    void finished();

private:
    void bytesWritten(qint64 count);
    void readyRead();
    void sendRow();
    bool nextRow();
    void computeRows();

private:
    static constexpr const qint64 flashRowSize = 512;

private:
    QString filename_;
    QSerialPort *port_;
    QSerialPortInfo info_;
    HexFileReader reader_;

    bool packet_sent_;
    int segment_;
    int index_;
    int bytes_sent_;
    int total_rows_;
    int sent_rows_;
    QVector<uint32_t> segaddrs_;
};

