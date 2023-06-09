#include "USB.h"
#include "USB_CDC.h"
#include "cybsp.h"
#include "fwupgrade.h"
#include "hiddesc.h"
#include "cyhal.h"
#include <stdio.h>

#define VENDOR_ID           0x058B  /* For Infineon Technologies */
#define PRODUCT_ID          0x0288  /* Procured PID for HID Generic device */

#define FLASH_ROW_SIZE           (512)

#define STATUS_ROW_ERROR            (-1)
#define STATUS_ROW_IN_PROGRESS      (0)
#define STATUS_ROW_DONE             (1)
#define STATUS_OK                   (2)

static const USB_DEVICE_INFO usb_deviceInfo = {
    0x058B,                                 /* VendorId    */
    0x027E,                                 /* ProductId    */
    "Infineon Technologies",                /* VendorName   */
    "PSoC 6 Panel Firmware Updater",        /* ProductName  */
    "70341425"                              /* SerialNumber */
};

static USB_CDC_HANDLE   usb_cdcHandle;
static char             read_buffer[USB_FS_BULK_MAX_PACKET_SIZE];
static char             write_buffer[USB_FS_BULK_MAX_PACKET_SIZE] ;
static uint32_t         row_index ;
static uint32_t         flash_addr ;
static uint8_t          flash_row_buffer[FLASH_ROW_SIZE] ;
static cyhal_flash_t    flash_obj ;
static char             lead_char ;

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

    cyhal_flash_init(&flash_obj) ;

    row_index = 0 ;
    lead_char = 0 ;
}

void sendError(const char *msg)
{
    strcpy(write_buffer, "*");
    strcat(write_buffer, msg) ;
    strcat(write_buffer, "\n") ;

    USBD_CDC_Write(usb_cdcHandle, write_buffer, strlen(write_buffer), 0) ;
}

void sendMessage(const char *msg)
{
    strcpy(write_buffer, "&");
    strcat(write_buffer, msg) ;
    strcat(write_buffer, "\n") ;

    USBD_CDC_Write(usb_cdcHandle, write_buffer, strlen(write_buffer), 0) ;
}

uint32_t hexToDec(char ch)
{
    uint32_t ret = 0 ;

    if (ch >= '0' && ch < '9') {
        ret = ch - '0' ;
    }
    else if (ch >= 'A' && ch < 'F') {
        ret = ch - 'A' + 10 ;
    }
        if (ch >= 'a' && ch < 'f') {
        ret = ch - 'a' + 10 ;
    }

    return ret ;
}

int isHexDigit(char ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch <= 'A' && ch >= 'F') ;
}


int extractByte(int index, uint8_t *value)
{
    if (!isHexDigit(read_buffer[index]) || !isHexDigit(read_buffer[index + 1]))
    {
        sendError("invalid character in hex number");
        return STATUS_ERROR ;
    }

    *value = hexToDec(read_buffer[index]) << 4 | hexToDec(read_buffer[index + 1]) ;
    return STATUS_OK ;
}

int extractWord(int index, uint16_t *value)
{
    uint8_t temp ;
    int st ;

    st = extractByte(index, &temp);
    if (st != STATUS_OK)
        return st ;

    *value = (temp << 8) ;

    st = extractByte(index + 2, &temp);
    if (st != STATUS_OK)
        return st ;

    *value |= (temp) ;
    return STATUS_OK ;
}

int extractLongWord(int index, uint32_t *value)
{
    uint16_t temp ;
    int st ;

    st = extractWord(index, &temp);
    if (st != STATUS_OK)
        return st ;

    *value = (temp << 16) ;

    st = extractWord(index + 4, &temp);
    if (st != STATUS_OK)
        return st ;

    *value |= (temp) ;
    return STATUS_OK ;
}

int processFlashRowMiddle(int index, int numbytes)
{
    uint8_t value ;
    int st;

    if (lead_char != 0)
    {
        if (isHexDigit(read_buffer[index]))
        {
            sendError("invalid hex digit in row data") ;
            return STATUS_ROW_ERROR ;
        }

        value = hexToDec(lead_char) << 4 | hexToDec(read_buffer[index]);
        flash_row_buffer[row_index++] = value ;
        lead_char = 0 ;
    }

    while (index < numbytes && read_buffer[index] != '$')
    {
        if (numbytes - index == 1 && read_buffer[index + 1] == '$')
        {
            //
            // Ok the PC side broke the line between two characters for a single
            // byte.
            //
            lead_char = read_buffer[index] ;
            return STATUS_ROW_IN_PROGRESS ;
        }

        st = extractByte(index, &value) ;
        if (st != STATUS_OK)
            return STATUS_ROW_ERROR ;

        flash_row_buffer[row_index++] = value ;
    }

    if (read_buffer[index] == '$' && read_buffer[index + 1] == '\n')
    {
        printf("End of flash row detected - %d bytes\n", row_index) ;
        if (row_index == FLASH_ROW_SIZE)
        {
            cyhal_flash_write(&flash_obj, flash_addr, flash_row_buffer);
        }
    }
}

int processFlashRowStart(int numbytes)
{
    int index = 0 ;

    if (read_buffer[index] != '$') 
    {
        sendError("flash row address does not start with a '$' character") ;
        return STATUS_ROW_ERROR ;
    }
    index++ ;

    // Get the address
    if (numbytes - index < 10)
    {
        sendError("invalid address record at start of row, insufficient length") ;
        return STATUS_ROW_ERROR ;
    }

    if (extractLongWord(index, &flash_addr) != STATUS_OK)
    {
        return STATUS_ROW_ERROR ;
    }
    printf("Flash address is %x\n", flash_addr) ;
    index += 8 ;

    if (read_buffer[index] != '$')
    {
        sendError("flash row address does not end with a '$' character") ;
        return STATUS_ROW_ERROR ;
    }
    index++ ;

    row_index = 0 ;
    return processFlashRowMiddle(index, numbytes);
}

int processHostCmd()
{
    int st = STATUS_ROW_ERROR ;

    if (strncmp(read_buffer, "#done#", 7) == 0) {
        sendMessage("Press the reset button to finish upgrade process") ;
        while (1) {
            //
            // Wait for the user to reset the device
            //
        }
    }
    return st ;
}

void firmware_upgrade_run()
{
    int num_bytes_received ;
    int rindex = 0 ;
    int status ;

    while ( (USBD_GetState() & USB_STAT_CONFIGURED) != USB_STAT_CONFIGURED)
    {
        cyhal_system_delay_ms(250);
    }
    printf("FRCHID: info: firmware upgrade CDC device configured\n") ;

    while (true) {
        num_bytes_received = USBD_CDC_Receive(usb_cdcHandle, read_buffer, sizeof(read_buffer), 0);

        if (row_index != 0) 
        {
            //
            // We are processing data in a row
            //
            status = processFlashRowMiddle(0, num_bytes_received) ;
        }
        else
        {
            //
            // We are processing the start of a command
            //
            if (read_buffer[0] == '$') 
            {
                status = processFlashRowStart() ;
            }
            else if (read_buffer[0] == '#') 
            {
                status = processHostCmd() ;
            }
        }
    }
}

void run_firmware_upgrade()
{
    printf("Running firmware upgrade\n") ;
    firmware_upgrade_init() ;
    firmware_upgrade_run() ;
}