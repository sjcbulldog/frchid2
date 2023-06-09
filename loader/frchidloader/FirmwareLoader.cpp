#include "FirmwareLoader.h"
#include <QtCore/QFile>

#include <iostream>

FirmwareLoader::FirmwareLoader(QObject* parent) : QObject(parent)
{
}

bool FirmwareLoader::setSerialPort(const QString& name)
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports)
    {
        if (info.portName() == name) {
            info_ = info;
            port_ = new QSerialPort(info_);
            if (!port_->open(QIODevice::ReadWrite)) 
            {
                std::cerr << "frchidloader: error: the port '" << name.toStdString() << "' cannot be opened" << std::endl;
                return false;
            }
            return true;
        }
    }

    std::cerr << "frchidloader: error: the port '" << name.toStdString() << "' is not a valid serial port." << std::endl;
    return false;
}

bool FirmwareLoader::setFirmwareFile(const QString& filename)
{
    QFile file(filename);

    if (!file.exists()) {
        std::cerr << "frchidloader: error: the file '" << filename.toStdString() << "' does not exist" << std::endl;
        return false;
    }

    if (!reader_.readFile(filename)) {
        std::cerr << "frchidloader: error: the file '" << filename.toStdString() << "' is not a valid hex file" << std::endl;
        return false;
    }

    return true ;
}

void FirmwareLoader::bytesWritten(qint64 data)
{
    std::cout << "frchidloader: bytes written " << data << std::endl;
}

void FirmwareLoader::readyRead()
{
    QByteArray data = port_->readAll();
    std::cout << "frchidloader: bytes read " << data.count() << std::endl;
}

void FirmwareLoader::sendNextRow()
{
    uint32_t addr = segaddrs_.at(segment_);
    const QByteArray& data = reader_.data(addr);

    qint64 towrite = data.size() - index_;
    if (towrite > flashRowSize)
        towrite = flashRowSize;

    QByteArray packet = data.mid(index_, towrite);

    QString packstr = "$" + QString("%1").arg(addr, 8, 16, QLatin1Char('0')) + "$";
    packstr += data.mid(index_, towrite).toHex() + "$\n";

    packet_sent_ = true;
    port_->write(packstr.toUtf8());
    port_->flush();
}

void FirmwareLoader::run()
{
    (void)connect(port_, &QSerialPort::bytesWritten, this, &FirmwareLoader::bytesWritten);
    (void)connect(port_, &QSerialPort::readyRead, this, &FirmwareLoader::readyRead);

    segaddrs_ = reader_.segmentAddrs();
    index_ = 0;
    segment_ = 0;
    packet_sent_ = false;

    sendNextRow();
}
