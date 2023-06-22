#include "FRCHidBootLoader.h"
#include "FirmwareBootloader.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

FRCHidBootLoader::FRCHidBootLoader(QWidget* parent) : QMainWindow(parent)
{
    ui.setupUi(this);

    initSerialPorts();
    initFileName();

    ui.progress->setValue(0);

    (void)connect(ui.program, &QPushButton::pressed, this, &FRCHidBootLoader::program);
    (void)connect(ui.browse, &QPushButton::pressed, this, &FRCHidBootLoader::browse);
}

FRCHidBootLoader::~FRCHidBootLoader()
{
}

void FRCHidBootLoader::closeEvent(QCloseEvent* ev)
{
    if (!ui.filename->text().isEmpty())
    {
        settings_.setValue(FileNameKey, ui.filename->text());
    }

    if (ui.ports->currentIndex() != -1)
    {
        settings_.setValue(PortNameKey, ui.ports->currentText());
    }
}

void FRCHidBootLoader::initSerialPorts()
{
    int index = -1;
    QString defport;

    if (settings_.contains(PortNameKey))
    {
        defport = settings_.value(PortNameKey).toString();
    }

    int curidx = 0;
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports)
    {
        ui.ports->addItem(info.portName());
        if (!defport.isEmpty() && info.portName() == defport)
        {
            index = curidx;
        }
        curidx++;
    }

    if (index != -1)
    {
        ui.ports->setCurrentIndex(index);
    }
}

void FRCHidBootLoader::initFileName()
{
    if (settings_.contains(FileNameKey))
    {
        ui.filename->setText(settings_.value(FileNameKey).toString());
    }
}

void FRCHidBootLoader::browse()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Firmware File"), "", tr("Hex File (*.hex);;All Files (*)"));
    if (filename.length() == 0)
        return;

    ui.filename->setText(filename);
}

void FRCHidBootLoader::program()
{
    if (ui.ports->currentIndex() == -1)
    {
        QMessageBox::critical(this, "Error", "No serial port selected");
        return;
    }

    port_ = new QSerialPort(ui.ports->currentText());

    if (!port_->open(QIODevice::ReadWrite))
    {
        QMessageBox::critical(this, "Error", "Serial port could not be opened");
        return;
    }

    if (ui.filename->text().isEmpty())
    {
        QMessageBox::critical(this, "Error", "No firmware file selected");
        return;
    }

    if (!reader_.readFile(ui.filename->text()))
    {
        QMessageBox::critical(this, "Error", "Could not open firmware file for reading");
        return;
    }
    reader_.normalize(flashRowSize);

    loader_ = new FirmwareBootloader(reader_, *port_, flashRowSize);
    ui.progress->setMinimum(0);
    ui.progress->setMaximum(loader_->totalRows());

    loader_->start();
}