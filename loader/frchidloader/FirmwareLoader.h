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
    void sendSegment(uint32_t addr, const QVector<uint8_t>& data);

private:
    QString filename_;
    QSerialPort *port_;
    QSerialPortInfo info_;
    HexFileReader reader_;
};

