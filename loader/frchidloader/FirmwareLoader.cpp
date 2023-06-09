#include "FirmwareLoader.h"
#include <QtCore/QFile>

#include <iostream>

FirmwareLoader::FirmwareLoader(QObject* parent) : QObject(parent)
{
    index_ = 0;
    packet_sent_ = false;
    port_ = nullptr;
    segment_ = -1;
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
    QString str = QString::fromUtf8(data);
    if (str.startsWith('*')) 
    {
        // Error message
        std::cerr << "frchidloader: bootloader: error: " << str.mid(1).toStdString() << std::endl;
    }
    else if (str.startsWith('&'))
    {
        // Information message
        std::cerr << "frchidloader: bootloader: info: " << str.mid(1).toStdString() << std::endl;
    }
    else if (str.startsWith('$'))
    {
        std::cout << "DONE PACKET: " << str.toStdString() << std::endl;
        uint32_t addr = segaddrs_.at(segment_);
        const QByteArray& data = reader_.data(addr);

        index_ += bytes_sent_;
        if (index_ > data.size())
        {
            std::cout << "NEW SEGMENT BEING SENT" << std::endl;
            segment_++;
            if (segment_ == segaddrs_.count())
            {
                //
                // We are done sending data, should tell the other end we are done
                //
                port_->write("#done#\n");
                return;
            }
        }

        sendNextRow();
    }
}

void FirmwareLoader::sendNextRow()
{
    uint32_t addr = segaddrs_.at(segment_);
    const QByteArray& data = reader_.data(addr);
    uint32_t rowaddr = addr + index_;

    bytes_sent_ = data.size() - index_;
    if (bytes_sent_ > flashRowSize)
        bytes_sent_ = flashRowSize;

    QByteArray packet = data.mid(index_, bytes_sent_);

    QString packstr = "$" + QString("%1").arg(rowaddr, 8, 16, QLatin1Char('0')) + "$";
    packstr += data.mid(index_, bytes_sent_).toHex() + "$\n";

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
