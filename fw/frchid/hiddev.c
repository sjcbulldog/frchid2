#include "USB.h"
#include "USB_HID.h"
#include "hiddev.h"
#include "hiddesc.h"
#include "hwconfig.h"
#include <stdint.h>
#include <stdio.h>

#define VENDOR_ID           0x058B  /* For Infineon Technologies */
#define PRODUCT_ID          0x0288  /* Procured PID for HID Generic device */

#define INPUT_REPORT_SIZE   64      /* Defines the input (device -> host) report size */
#define OUTPUT_REPORT_SIZE  64      /* Defines the output (Host -> device) report size */
#define VENDOR_PAGE_ID      0x12    /* Defines the vendor specific page that */

static const USB_DEVICE_INFO device_info = {
    VENDOR_ID,                              /* VendorId */
    PRODUCT_ID,                             /* ProductId */
    "Infineon Technologies",                /* VendorName */
    "PSoC 6 Panel Device",                  /* ProductName */
    "14257034"                              /* SerialNumber */
};

static int prev_valid = 0 ;
static int16_t prev_axis[FRC_HID_AXIS_COUNT] ;
static uint32_t prev_buttons ;

static uint8_t usb_hid_data[1024] ;
static uint32_t usb_hid_size ;

static uint32_t usb_out_size ;

static USB_HID_HANDLE       hid_instance_handle;

static void prep_descriptor()
{
    usb_hid_report_size = 0 ;
    usb_hid_size = 0 ;
    usb_out_size = 0 ;

    start_descriptor();
    start_collection(0x01) ;                    // COLLECTION (Application)
    start_collection(0x00) ;                    // COLLECTION (Physical)

    add_digital_inputs(FRC_HID_INPUT_COUNT) ;
    usb_hid_size += sizeof(uint32_t) ;

    add_analog_inputs(FRC_HID_AXIS_COUNT) ;
    usb_hid_size += FRC_HID_AXIS_COUNT * 2 ;

    add_digital_outputs(FRC_HID_OUTPUT_COUNT) ;
    usb_out_size += FRC_HID_OUTPUT_COUNT / 8 ;

    end_collection() ;                          // END COLLECTION (Physical)  

    end_collection() ;                          // END COLLECTION (Application)
}

static void usbd_hid_init(void) {
    static uint8_t              out_buffer[USB_HS_INT_MAX_PACKET_SIZE];
    USB_HID_INIT_DATA_EX        init_data;
    USB_ADD_EP_INFO             ep_int_in;
    USB_ADD_EP_INFO             ep_int_out;

    memset(&init_data, 0, sizeof(init_data));
    ep_int_in.Flags             = 0;                             /* Flags not used. */
    ep_int_in.InDir             = USB_DIR_IN;                    /* IN direction (Device to Host) */
    ep_int_in.Interval          = 1;                             /* Interval of 125 us (1 ms in full-speed) */
    ep_int_in.MaxPacketSize     = USB_HS_INT_MAX_PACKET_SIZE;    /* Maximum packet size (64 for Interrupt). */
    ep_int_in.TransferType      = USB_TRANSFER_TYPE_INT;         /* Endpoint type - Interrupt. */
    init_data.EPIn = USBD_AddEPEx(&ep_int_in, NULL, 0);

    ep_int_out.Flags            = 0;                             /* Flags not used. */
    ep_int_out.InDir            = USB_DIR_OUT;                   /* OUT direction (Host to Device) */
    ep_int_out.Interval         = 1;                             /* Interval of 125 us (1 ms in full-speed) */
    ep_int_out.MaxPacketSize    = USB_HS_INT_MAX_PACKET_SIZE;    /* Maximum packet size (64 for Interrupt). */
    ep_int_out.TransferType     = USB_TRANSFER_TYPE_INT;         /* Endpoint type - Interrupt. */
    init_data.EPOut = USBD_AddEPEx(&ep_int_out, out_buffer, sizeof(out_buffer));

    init_data.pReport           = usb_hid_report;
    init_data.NumBytesReport    = usb_hid_report_size ;
    init_data.pInterfaceName    = "FRCPanel";
    hid_instance_handle         = USBD_HID_AddEx(&init_data);
}

static int prev_equal(int16_t *axis, uint32_t count,  uint32_t buttons)
{
    if (!prev_valid)
        return 0 ;

    if (buttons != prev_buttons)
        return 0 ;

    for(int i=  0 ; i < FRC_HID_AXIS_COUNT ; i++) {
        if (axis[i] != prev_axis[i])
            return 0 ;
    }

    return 1 ;
}

static void copy_prev(int16_t *axis, uint32_t count,  uint32_t buttons)
{
    memcpy(prev_axis, axis, sizeof(int16_t) * FRC_HID_AXIS_COUNT) ;
    prev_buttons = buttons ;
    prev_valid = 1 ;
}

static void prep_report(int16_t *axis, uint32_t count,  uint32_t buttons)
{
    uint32_t index = 0 ;

    memcpy(&usb_hid_data[index], &buttons, sizeof(buttons));
    index += 4 ;

    memcpy(&usb_hid_data[index], axis, sizeof(int16_t) * count) ;
    index += sizeof(int16_t) * count ;
}

void hid_run(int16_t *axis, uint32_t count, uint32_t buttons, uint8_t *outbuf, uint32_t *size)
{
    int cnt ;

    if (*size < usb_out_size) {
        printf("FRCHID: error: invalid out report buffer size, must be at least %ld bytes\n", usb_out_size) ;
        return ;
    }

    if ((USBD_GetState() & USB_STAT_CONFIGURED) != USB_STAT_CONFIGURED) 
    {
        //
        // We are not connnected to a host, we don't need to process
        // and USB HID requests
        //
        return ;
    }

    cnt = USBD_HID_Read(hid_instance_handle, outbuf, 64, 10) ;
    if (cnt == usb_out_size && cnt > 0) {
#ifdef DEBUG_HID_OUT_REPORTS
        printf("FRCHID: received HID output report %d bytes\n", cnt);
        printf("FRCHID: ") ;
        for(int i = 0 ; i < cnt ; i++) {
            printf("%02x", outbuf[i]);
        }
        printf("\n") ;
#endif
        *size = cnt ;
    }
    
    //
    // See if we need to write a new HID report
    // 
    if (prev_equal(axis, count, buttons) == 0)
    {
        uint32_t bytes = USBD_HID_GetNumBytesRemToWrite(hid_instance_handle) ;
        if (bytes == 0) 
        {
            prep_report(axis, count, buttons) ;
            // memset(usb_hid_data, 0xcc, usb_hid_size) ;
            USBD_HID_Write(hid_instance_handle, usb_hid_data, usb_hid_size, -1) ;
            copy_prev(axis, count, buttons) ;

#ifdef DEBUG_HID_IN_REPORTS
            printf("FRCHID: writing HID input report %ld bytes\n", usb_hid_size);
            printf("FRCHID: ") ;
            for(int i = 0 ; i < usb_hid_size ; i++) {
                printf("%02x", usb_hid_data[i]);
            }
            printf("\n") ;
#endif
        }
    }
}

int hid_init()
{
    /* Setup the HID report descriptor */
    prep_descriptor() ;

    /* Initialize the emUSB device stack */
    USBD_Init();

    /* Initialize the endpoint for HID class */
    usbd_hid_init();

    /* Set the device info for enumeration*/
    USBD_SetDeviceInfo(&device_info);

    /* Start the USB stack*/
    USBD_Start();
    
    return 1 ;
}