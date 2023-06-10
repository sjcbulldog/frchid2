#include "FirmwareLoader.h"
#include <QtCore/QFile>
#include <iostream>

FirmwareLoader::FirmwareLoader(QObject* parent) : QObject(parent)
{
    index_ = 0;
    packet_sent_ = false;
    port_ = nullptr;
    segment_ = -1;
    bytes_sent_ = 0;
    sent_rows_ = 0;
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

    reader_.normalize(512);
    computeRows();

    std::cout << "firhidloader: programming " << reader_.segmentAddrs().count() << " unique segment ranges" << std::endl;
    int count = 1;
    for (uint32_t addr : reader_.segmentAddrs())
    {
        const QByteArray& data = reader_.data(addr);
        std::cout << "              range " << count++;
        std::cout << std::hex << ": address " << addr;
        std::cout << std::dec << ", size " << data.count() << " bytes ";
        std::cout << "( " << data.count() / flashRowSize << " flash rows)";
        std::cout << std::endl;
    }

    return true ;
}

void FirmwareLoader::bytesWritten(qint64 data)
{
}

bool FirmwareLoader::nextRow()
{
    bool ret = true;

    uint32_t addr = segaddrs_.at(segment_);
    const QByteArray& data = reader_.data(addr);

    index_ += bytes_sent_;
    if (index_ >= data.size())
    {
        segment_++;
        index_ = 0;
        if (segment_ == segaddrs_.count())
        {
            ret = false;
        }
    }
    return ret;
}

void FirmwareLoader::readyRead()
{
    QByteArray data = port_->readAll();
    QString str = QString::fromUtf8(data);
    if (str.startsWith('*')) 
    {
        std::cout << std::endl;
        std::cout << std::endl;

        // Error message
        std::cerr << "frchidloader: bootloader: error: " << str.mid(1).toStdString() << std::endl;
    }
    else if (str.startsWith('&'))
    {
        std::cout << std::endl;
        std::cout << std::endl;

        // Information message
        std::cerr << "frchidloader: bootloader: info: " << str.mid(1).toStdString() << std::endl;
    }
    else if (str.startsWith('#'))
    {
        emit finished();
    }
    else if (str.startsWith('$'))
    {
        sent_rows_++;

        std::cout << "Progress: " << sent_rows_ << " of " << total_rows_ << "\r" << std::flush;

        if (!nextRow())
        {
            port_->write("#done#\n");
            return;
        }
        else
        {
            sendRow();
        }
    }
}

void FirmwareLoader::sendRow()
{
    //
    // The base address of the current block of data from the hex file
    //
    uint32_t addr = segaddrs_.at(segment_);

    //
    // The current block of data from the hex file
    //
    const QByteArray& data = reader_.data(addr);

    //
    // The row address in flash for the current row
    //
    uint32_t rowaddr = addr + index_;

    //
    // Compute the amount of data to send, should never exceed a flash row size
    //
    bytes_sent_ = data.size() - index_;
    if (bytes_sent_ > flashRowSize)
        bytes_sent_ = flashRowSize;

    //
    // Format the packet to send.  It is of the form $AAAAAAAA$DDDDDDDDDDDDDDD...$
    // where the address is AAAAAAAA as eight digits.  DDDD... is the data and there
    // are always a flash row size of data.
    //
    QString packstr = "$" + QString("%1").arg(rowaddr, 8, 16, QLatin1Char('0')) + "$";
    packstr += data.mid(index_, bytes_sent_).toHex() + "$\n";

    //
    // Write the packet to the serial port device.  Note, it is not a real serial port
    // but rather a USB based serial port.
    //
    packet_sent_ = true;
    port_->write(packstr.toUtf8());
    port_->flush();
}

void FirmwareLoader::computeRows()
{
    total_rows_ = 0;

    for (uint32_t addr : reader_.segmentAddrs())
    {
        const QByteArray& data = reader_.data(addr);

        // Normalizing the hex file should make this true
        assert((addr % flashRowSize) == 0);
        assert((data.count() % flashRowSize) == 0);

        total_rows_ += data.count() / flashRowSize;
    }
}

void FirmwareLoader::run()
{
    (void)connect(port_, &QSerialPort::bytesWritten, this, &FirmwareLoader::bytesWritten);
    (void)connect(port_, &QSerialPort::readyRead, this, &FirmwareLoader::readyRead);

    // Setup the first row to be sent
    segaddrs_ = reader_.segmentAddrs();
    index_ = 0;
    segment_ = 0;
    packet_sent_ = false;

    //
    // Send the first row.  The response from this row will be processed and
    // cause the next rows to be sent until the end
    //
    sendRow();
}
