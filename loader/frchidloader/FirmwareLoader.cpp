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

void FirmwareLoader::sendSegment(uint32_t addr, const QVector<uint8_t>& data)
{
    static const int flashRowSize = 512;

    uint32_t total = data.size();
    uint32_t index = 0;
    QString str;

    QByteArray packet;

    while (total > 0) {
        uint32_t towrite = total;
        if (towrite > flashRowSize)
            towrite = flashRowSize;

        packet.clear();

        for (int i = 0; i < towrite; i++) {
            if (index > data.size())
                break;

            packet.append(data[index++]);
        }

        str = "$" + QString::number(addr, 16) + "$";
        str += ":" + packet.toHex() + ":\n";

        port_->write(str.toUtf8());
        port_->flush();

        addr += packet.count();
        total -= packet.count();

        //
        // Now, wait for a response
        //
        QString respstr;
        while (true) {
            QByteArray resp = port_->readAll();
            respstr += QString::fromUtf8(resp);
            if (respstr.contains("$OK$\n"))
                break;
        }
    }
}

void FirmwareLoader::run()
{
    emit finished();
}
