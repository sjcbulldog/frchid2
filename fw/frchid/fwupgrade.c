#include "USB.h"
#include "USB_CDC.h"
#include "cybsp.h"
#include "fwupgrade.h"
#include "hiddesc.h"
#include "cyhal.h"
#include <stdio.h>

#define VENDOR_ID           0x058B  /* For Infineon Technologies */
#define PRODUCT_ID          0x0288  /* Procured PID for HID Generic device */

static const USB_DEVICE_INFO usb_deviceInfo = {
    0x058B,                                 /* VendorId    */
    0x027E,                                 /* ProductId    */
    "Infineon Technologies",                /* VendorName   */
    "PSoC 6 Panel Firmware Updater",        /* ProductName  */
    "70341425"                              /* SerialNumber */
};

static USB_CDC_HANDLE usb_cdcHandle;
static char           read_buffer[USB_FS_BULK_MAX_PACKET_SIZE];
// static char           write_buffer[USB_FS_BULK_MAX_PACKET_SIZE];

static void usb_add_cdc(void) {
    static U8             OutBuffer[USB_FS_BULK_MAX_PACKET_SIZE];
    USB_CDC_INIT_DATA     InitData;
    USB_ADD_EP_INFO       EPBulkIn;
    USB_ADD_EP_INFO       EPBulkOut;
    USB_ADD_EP_INFO       EPIntIn;

    memset(&InitData, 0, sizeof(InitData));
    EPBulkIn.Flags          = 0;                             /* Flags not used */
    EPBulkIn.InDir          = USB_DIR_IN;                    /* IN direction (Device to Host) */
    EPBulkIn.Interval       = 0;                             /* Interval not used for Bulk endpoints */
    EPBulkIn.MaxPacketSize  = USB_FS_BULK_MAX_PACKET_SIZE;   /* Maximum packet size (64B for Bulk in full-speed) */
    EPBulkIn.TransferType   = USB_TRANSFER_TYPE_BULK;        /* Endpoint type - Bulk */
    InitData.EPIn  = USBD_AddEPEx(&EPBulkIn, NULL, 0);

    EPBulkOut.Flags         = 0;                             /* Flags not used */
    EPBulkOut.InDir         = USB_DIR_OUT;                   /* OUT direction (Host to Device) */
    EPBulkOut.Interval      = 0;                             /* Interval not used for Bulk endpoints */
    EPBulkOut.MaxPacketSize = USB_FS_BULK_MAX_PACKET_SIZE;   /* Maximum packet size (64B for Bulk in full-speed) */
    EPBulkOut.TransferType  = USB_TRANSFER_TYPE_BULK;        /* Endpoint type - Bulk */
    InitData.EPOut = USBD_AddEPEx(&EPBulkOut, OutBuffer, sizeof(OutBuffer));

    EPIntIn.Flags           = 0;                             /* Flags not used */
    EPIntIn.InDir           = USB_DIR_IN;                    /* IN direction (Device to Host) */
    EPIntIn.Interval        = 64;                            /* Interval of 8 ms (64 * 125us) */
    EPIntIn.MaxPacketSize   = USB_FS_INT_MAX_PACKET_SIZE ;   /* Maximum packet size (64 for Interrupt) */
    EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         /* Endpoint type - Interrupt */
    InitData.EPInt = USBD_AddEPEx(&EPIntIn, NULL, 0);

    usb_cdcHandle = USBD_CDC_Add(&InitData);
}

void firmware_upgrade_init()
{
    /* Initializes the USB stack */
    USBD_Init();

    /* Endpoint Initialization for CDC class */
    usb_add_cdc();

    /* Set device info used in enumeration */
    USBD_SetDeviceInfo(&usb_deviceInfo);

    /* Start the USB stack */
    USBD_Start();
}

void firmware_upgrade_run()
{
    int num_bytes_received ;

    while ( (USBD_GetState() & USB_STAT_CONFIGURED) != USB_STAT_CONFIGURED)
    {
        cyhal_system_delay_ms(250);
    }
    printf("FRCHID: info: firmware upgrade CDC device configured\n") ;

    while (true) {
        num_bytes_received = USBD_CDC_Receive(usb_cdcHandle, read_buffer, sizeof(read_buffer), 0);
        bool lf = false ;
        printf("received:") ;
        for(int i = 0 ; i < num_bytes_received ; i++) {
            if ((i % 32) == 0 && i != 0) {
                printf("\n        :") ;
            }
            if (read_buffer[i] == '\n' || read_buffer[i] == 0x0d)
                lf = true ;

            if ((i % 2) == 0)
                printf(" ") ;
                
            printf("%x", read_buffer[i]) ;

        }
        printf("\n") ;

        if (lf) {
            printf("  Writing response\n") ;
            int len = USBD_CDC_Write(usb_cdcHandle, "$OK$\n", 5, 0) ;
            USBD_CDC_WaitForTX(usb_cdcHandle, 0);
            printf("  Writing response - written %d\n", len) ;
        }
    }
}

void run_firmware_upgrade()
{
    printf("Running firmware upgrade\n") ;
    firmware_upgrade_init() ;
    firmware_upgrade_run() ;
}