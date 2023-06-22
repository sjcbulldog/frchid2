#pragma once

#include "HexFileReader.h"
#include <QtCore/QSettings>
#include <QtWidgets/QMainWindow>
#include <QtSerialPort/QSerialPort>
#include "ui_FRCHidBootLoader.h"

class FirmwareBootloader;

class FRCHidBootLoader : public QMainWindow
{
    Q_OBJECT

public:
    FRCHidBootLoader(QWidget *parent = nullptr);
    ~FRCHidBootLoader();

protected:
    void closeEvent(QCloseEvent* ev) override;

private:
    static constexpr const char* FileNameKey = "FirmwareFilename";
    static constexpr const char* PortNameKey = "PortName";
    static constexpr const qint64 flashRowSize = 512;

private:
    void initSerialPorts();
    void initFileName();
    void program();
    void browse();

private:
    QSettings settings_;
    Ui::FRCHidBootLoaderClass ui;
    HexFileReader reader_;
    QSerialPort* port_;
    FirmwareBootloader* loader_;
};
