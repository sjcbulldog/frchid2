#include "FirmwareLoader.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Task parented to the application so that it
    // will be deleted by the application.
    FirmwareLoader task = new FirmwareLoader(&a);

    // This will cause the application to exit when
    // the task signals finished.    
    QObject::connect(&task, SIGNAL(finished()), &a, SLOT(quit()));

    argc--;
    argv++;

    if (argc != 2) {
        std::cerr << "frchidloader: error: expects two arguments" << std::endl;
        std::cerr << "                     arg1 - the name of the serial port" << std::endl;
        std::cerr << "                     arg2 - the name of the hex file to loader" << std::endl;
        return 1;
    }

    if (!task.setSerialPort(*argv)) {
        return 1;
    }

    argc--;
    argv++;

    if (!task.setFirmwareFile(*argv))
        return 1;

    // This will run the task from the application event loop.
    QTimer::singleShot(0, &task, SLOT(run()));

    return a.exec();
}
