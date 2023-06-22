#include "FirmwareBootloader.h"
#include "HexFileReader.h"
#include <QtCore/QDebug>

FirmwareBootloader::FirmwareBootloader(HexFileReader& rdr, QSerialPort& port, int rowsize) : reader_(rdr), port_(port)
{
    computeRows();

    segaddrs_ = reader_.segmentAddrs();
    index_ = 0;
    segment_ = 0;
    packet_sent_ = false;
    flash_row_size_ = rowsize;

    (void)connect(&port_, &QSerialPort::readyRead, this, &FirmwareBootloader::readyRead);
}

FirmwareBootloader::~FirmwareBootloader()
{
    (void)disconnect(&port_, &QSerialPort::readyRead, this, &FirmwareBootloader::readyRead);
}

void FirmwareBootloader::computeRows()
{
    total_rows_ = 0;

    for (uint32_t addr : reader_.segmentAddrs())
    {
        const QByteArray& data = reader_.data(addr);
        total_rows_ += data.count() / flash_row_size_;
    }
}

bool FirmwareBootloader::nextRow()
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

void FirmwareBootloader::readyRead()
{
    QByteArray data = port_.readAll();
    QString str = QString::fromUtf8(data);

    qDebug() << "readyRead: '" << str << "'";

    if (str.startsWith('*'))
    {
        emit errorMessage(str.mid(1));
    }
    else if (str.startsWith('&'))
    {
        emit userMessage(str.mid(1));
    }
    else if (str.startsWith('#'))
    {
        emit finished();
    }
    else if (str.startsWith('$'))
    {
        sent_rows_++;

        emit progress(sent_rows_);

        if (!nextRow())
        {
            port_.write("#done#\n");
            return;
        }
        else
        {
            sendRow();
        }
    }
}

void FirmwareBootloader::sendRow()
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

    qDebug() << "sendRow: address = " << rowaddr;

    //
    // Compute the amount of data to send, should never exceed a flash row size
    //
    bytes_sent_ = data.size() - index_;
    if (bytes_sent_ > flash_row_size_)
        bytes_sent_ = flash_row_size_;

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
    port_.write(packstr.toUtf8());
    port_.flush();


}

void FirmwareBootloader::start()
{
    sendRow();
}