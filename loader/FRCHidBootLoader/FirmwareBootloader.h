#pragma once

#include <QtCore/QObject>
#include <QtSerialPort/QSerialPort>

class HexFileReader;

class FirmwareBootloader : public QObject
{
	Q_OBJECT

public:
    FirmwareBootloader(HexFileReader &hexfile, QSerialPort& port, int rowsize);
    virtual ~FirmwareBootloader();

    int totalRows() {
        return total_rows_;
    }

    void start();

signals:
    void finished();
    void progress(int rows);
    void errorMessage(const QString& msg);
    void userMessage(const QString& msg);

private:
    void computeRows();
    bool nextRow();
    void readyRead();
    void sendRow();

private:
    HexFileReader& reader_;
    QSerialPort& port_;
    int flash_row_size_;

    bool packet_sent_;
    int segment_;
    int index_;
    int bytes_sent_;
    QVector<uint32_t> segaddrs_;
    int total_rows_;
    int sent_rows_;
};

