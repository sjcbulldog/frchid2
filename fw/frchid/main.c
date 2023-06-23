#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "hw.h"
#include "hiddev.h"
#include "hwconfig.h"
#include "fwupgrade.h"
#include "set_img_ok.h"
#include <watchdog.h>
#include <string.h>

extern int set_img_ok(int, int) ;

static void init_failed(int msec)
{
    while (1)
    {
        cyhal_gpio_toggle(CYBSP_USER_LED) ;
        cyhal_system_delay_ms(msec) ;
    }
}

static void run_normal_hid()
{
    int16_t axis[FRC_HID_AXIS_COUNT] ;
    uint8_t outbuf[FRC_HID_OUTPUT_COUNT / 8 + 1];
    uint32_t outbufsize ;
    uint32_t buttons ;

    if (hw_init() == 0) {
        printf("FRCHID: hardware initialization failed\n");
        init_failed(500) ;
    }

    printf("FRCHID: hardware initialization completed\n") ;

    if (hid_init() == 0) {
        printf("FRCHID: USB HID initialization failed") ;
        init_failed(100) ;
    }

    printf("FRCHID: USBHID initialization completed\n") ;

    while (true) {
        outbufsize = sizeof(outbuf) ;

        hw_read(axis, FRC_HID_AXIS_COUNT, &buttons) ;
        hid_run(axis, FRC_HID_AXIS_COUNT, buttons, outbuf, &outbufsize) ;
        if (outbufsize > 0) {
            hw_write(outbuf, outbufsize) ;
        }
        cyhal_system_delay_ms(10) ;
    }    
}

int main(void)
{
    int upgrade = 0 ;

    cy_rslt_t result;
    cyhal_resource_inst_t scbrsc = { CYHAL_RSC_SCB, 5, 0 } ;
    cyhal_resource_inst_t txrsc = { CYHAL_RSC_GPIO, 5, 0} ;
    cyhal_resource_inst_t rxrsc = { CYHAL_RSC_GPIO, 5, 1} ;

    cy_wdg_kick();
    cy_wdg_free();

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    cyhal_hwmgr_free(&scbrsc);   
    cyhal_hwmgr_free(&txrsc);  
    cyhal_hwmgr_free(&rxrsc);  

    /* Initialize the User LED */
    result = cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    /* GPIO init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }    

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
    /* retarget-io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        init_failed(3000) ;
    }

    printf("\x1b[2J\x1b[;H");
    printf("FRCHID: firmware version %d.%d.%d\n", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD) ;
    printf("FRCHID: bsp/retarget io initialization completed\n") ;

#if defined(UPGRADE_IMAGE)
    upgrade = 1 ;
#endif

    printf("FRCHID: upgrade is %d\n", upgrade) ;

    if (upgrade) 
    {
        uint32_t img_ok_addr;

        img_ok_addr = USER_APP_START + USER_APP_SIZE - USER_SWAP_IMAGE_OK_OFFS;

        if (*((uint8_t *)img_ok_addr) != USER_SWAP_IMAGE_OK)
        {
            printf("FRCHID: settings image ok\n") ;
            result = set_img_ok(img_ok_addr, USER_SWAP_IMAGE_OK);
            if (CY_RSLT_SUCCESS == result)
            {
                printf("FRCHID: Updated firmware image was set.\n");
            }
            else
            {
                printf("FRCHID: Updated firmware image failed to set.\n");
            }
        }
        else
        {
            printf("FRCHID: image is already in confirmed state\r\n");
        }
    }

    result = cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT, CYBSP_USER_BTN_DRIVE, true);
    if (cyhal_gpio_read(CYBSP_USER_BTN) == 1) {
        run_normal_hid() ;
    }
    else {
        run_firmware_upgrade() ;
    }

}
