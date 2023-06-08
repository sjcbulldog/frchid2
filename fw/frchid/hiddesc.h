#include <stdint.h>

extern uint8_t usb_hid_report[] ;
extern uint32_t usb_hid_report_size ;

static inline void start_descriptor()
{
    // USAGE_PAGE (Generic Desktop)
    usb_hid_report[usb_hid_report_size++] = 0x05 ;
    usb_hid_report[usb_hid_report_size++] = 0x01 ;

    // USAGE (Game Pad)
    usb_hid_report[usb_hid_report_size++] = 0x09 ;
    usb_hid_report[usb_hid_report_size++] = 0x05 ;
}

static inline void start_collection(uint8_t type)
{
    // COLLECTION
    usb_hid_report[usb_hid_report_size++] = 0xA1 ;
    usb_hid_report[usb_hid_report_size++] = type ;
}

static inline void end_collection()
{
    usb_hid_report[usb_hid_report_size++] = 0xC0;
}

static inline void add_digital_inputs(int count)
{
    // USAGE_PAGE (Button)
    usb_hid_report[usb_hid_report_size++] = 0x05 ;
    usb_hid_report[usb_hid_report_size++] = 0x09 ;

    // USAGE_MINIMUM (1)
    usb_hid_report[usb_hid_report_size++] = 0x19 ;
    usb_hid_report[usb_hid_report_size++] = 0x01 ;

    // USAGE_MAXIMUM (count)
    usb_hid_report[usb_hid_report_size++] = 0x29 ;
    usb_hid_report[usb_hid_report_size++] = count ;

    // LOGICAL_MINIMUM (0)
    usb_hid_report[usb_hid_report_size++] = 0x15 ;
    usb_hid_report[usb_hid_report_size++] = 0x00 ; 

    // LOGICAL_MAXIMUM(1)
    usb_hid_report[usb_hid_report_size++] = 0x25 ;
    usb_hid_report[usb_hid_report_size++] = 0x01 ;         

    // REPORT_SIZE (1 bit)
    usb_hid_report[usb_hid_report_size++] = 0x75 ;
    usb_hid_report[usb_hid_report_size++] = 0x01 ;     

    // REPORT_COUNT (count bit)
    usb_hid_report[usb_hid_report_size++] = 0x95 ;
    usb_hid_report[usb_hid_report_size++] = count ;

    // INPUT (Data, Var, Abs)
    usb_hid_report[usb_hid_report_size++] = 0x81 ;
    usb_hid_report[usb_hid_report_size++] = 0x02 ; 

    if (count < 32) {
        // REPORT_SIZE (1 bit)
        usb_hid_report[usb_hid_report_size++] = 0x75 ;
        usb_hid_report[usb_hid_report_size++] = 32 - count ;

        // REPORT_COUNT (count bit)
        usb_hid_report[usb_hid_report_size++] = 0x95 ;
        usb_hid_report[usb_hid_report_size++] = 1 ;

        // INPUT (Data, Var, Abs)
        usb_hid_report[usb_hid_report_size++] = 0x81 ;
        usb_hid_report[usb_hid_report_size++] = 0x03 ; 
    }
}

static inline void add_analog_inputs(uint8_t count)
{
    // USAGE_PAGE (Generic Desktop)
    usb_hid_report[usb_hid_report_size++] = 0x05 ;
    usb_hid_report[usb_hid_report_size++] = 0x01 ;

    for(int i = 0 ; i < count ; i++) {
        usb_hid_report[usb_hid_report_size++] = 0x09 ;
        usb_hid_report[usb_hid_report_size++] = 0x30 + i ;
    }

    // MINIMUM VALUE (0x00)
    usb_hid_report[usb_hid_report_size++] = 0x15 ;
    usb_hid_report[usb_hid_report_size++] = 0x00 ; 

    // MAXIMUM VALUE (32767)
    usb_hid_report[usb_hid_report_size++] = 0x26 ;
    usb_hid_report[usb_hid_report_size++] = 0xFF;
    usb_hid_report[usb_hid_report_size++] = 0x7F;

    // REPORT_COUNT (count fields)
    usb_hid_report[usb_hid_report_size++] = 0x95 ;
    usb_hid_report[usb_hid_report_size++] = count ; 

    // REPORT_SIZE (16 bits)
    usb_hid_report[usb_hid_report_size++] = 0x75 ;
    usb_hid_report[usb_hid_report_size++] = 0x10 ; 

    // INPUT
    usb_hid_report[usb_hid_report_size++] = 0x81 ;
    usb_hid_report[usb_hid_report_size++] = 0x02 ;
}

static inline void add_digital_outputs(int count)
{
    // USAGE
    usb_hid_report[usb_hid_report_size++] = 0x09 ;
    usb_hid_report[usb_hid_report_size++] = 0x00 ;  

    // REPORT_SIZE (1 bit)
    usb_hid_report[usb_hid_report_size++] = 0x75 ;
    usb_hid_report[usb_hid_report_size++] = 0x01 ;     

    // REPORT_COUNT (count bit)
    usb_hid_report[usb_hid_report_size++] = 0x95 ;
    usb_hid_report[usb_hid_report_size++] = count ;

    // LOGICAL_MINIMUM (0)
    usb_hid_report[usb_hid_report_size++] = 0x15 ;
    usb_hid_report[usb_hid_report_size++] = 0x00 ; 

    // LOGICAL_MAXIMUM(1)
    usb_hid_report[usb_hid_report_size++] = 0x25 ;
    usb_hid_report[usb_hid_report_size++] = 0x01 ;  

    // OUTPUT
    usb_hid_report[usb_hid_report_size++] = 0x91 ;
    usb_hid_report[usb_hid_report_size++] = 0x02 ; 

    if ((count % 8) != 0) {
        //
        // Add in the extra bits to pad to an eight bit number
        //
        int bytes = count / 8 ;
        int bits = count - bytes * 8 ;

        // REPORT_SIZE (1 bit)
        usb_hid_report[usb_hid_report_size++] = 0x75 ;
        usb_hid_report[usb_hid_report_size++] = 8 - bits ;    

        // REPORT_COUNT (count bit)
        usb_hid_report[usb_hid_report_size++] = 0x95 ;
        usb_hid_report[usb_hid_report_size++] = 1 ;

        // INPUT (Data, Var, Abs)
        usb_hid_report[usb_hid_report_size++] = 0x91 ;
        usb_hid_report[usb_hid_report_size++] = 0x03 ; 
    }
}

static inline void add_firmware_update()
{
    // USAGE_PAGE (Vendor Defined 1)
    usb_hid_report[usb_hid_report_size++] = 0x06 ;
    usb_hid_report[usb_hid_report_size++] = 0x00 ; 
    usb_hid_report[usb_hid_report_size++] = 0xFF ;

    // USAGE (Vendor Usage 1)
    usb_hid_report[usb_hid_report_size++] = 0x09 ;
    usb_hid_report[usb_hid_report_size++] = 0x01 ;      

    // REPORT_SIZE (8 bits)
    usb_hid_report[usb_hid_report_size++] = 0x75 ;
    usb_hid_report[usb_hid_report_size++] = 8 ;

    // REPORT_COUNT (56 bytes)
    usb_hid_report[usb_hid_report_size++] = 0x95 ;
    usb_hid_report[usb_hid_report_size++] = 56 ;

    // OUTPUT
    usb_hid_report[usb_hid_report_size++] = 0x91 ;
    usb_hid_report[usb_hid_report_size++] = 0x02 ;
}
