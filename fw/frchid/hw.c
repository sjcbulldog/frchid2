#include "hw.h"
#include "cyhal.h"
#include "hwconfig.h"
#include "cybsp.h"
#include <stdio.h>

static int adc_pins[FRC_HID_AXIS_COUNT] = 
{
   P10_0, P10_1, P10_2, P10_3,
   P10_4, P10_5
} ;

static int in_pins[FRC_HID_INPUT_COUNT] = 
{
   P0_5,  P1_0, P11_2, P11_3,
   P11_4, P11_5, P11_6, P11_7,
   P12_0, P12_1, P12_3, P12_4, 
   P12_5, P13_0, P13_1, P13_2, 
   P13_3, P13_4, P13_5, P13_6, 
   P5_4, P5_5, P5_6, P5_7
} ;

static int out_pins[FRC_HID_OUTPUT_COUNT] =
{
   P6_2, P6_3, P8_1, P8_2,
   P8_3, P8_4, P8_5, P8_6,
   P8_7, P9_0, P9_1, P9_2, 
   P9_3, P9_4, P9_5, P9_6, 
} ;

static cyhal_adc_t adcobj ;
static int adc_read_in_progress = 0 ;

/* ADC channel configuration */
static const cyhal_adc_channel_config_t channel_config = 
{
   .enable_averaging = false,       // Disable averaging for channel
   .min_acquisition_ns = 1000,      // Minimum acquisition time set to 1us
   .enabled = true                  // Sample this channel when ADC performs a scan
}; 

cyhal_adc_channel_t adc_chan_obj[FRC_HID_AXIS_COUNT ] ;

static int32_t result_arr[FRC_HID_AXIS_COUNT] ;
static int32_t result_cached[FRC_HID_AXIS_COUNT] ;

static inline int get_input_pin(int which)
{
   return in_pins[which] ;
}

void hw_write(uint8_t *outbuf, uint32_t size)
{
   for(int i = 0 ; i < FRC_HID_OUTPUT_COUNT ; i++)
   {
      int byteoffset = i / 8 ;
      int bitoffset = i % 8 ;
      int bit = (outbuf[byteoffset] >> bitoffset) & 0x01 ;
      cyhal_gpio_write(out_pins[i], bit) ;
   }
}

void hw_read(int16_t *data, uint32_t count, uint32_t *buttons)
{
//    if (!adc_read_in_progress) {
//       adc_read_in_progress = true ;
//       cyhal_adc_read_async_uv(&adcobj, 1, result_arr);
//    }

   for(int i = 0 ; i < FRC_HID_AXIS_COUNT - 2 ; i++) 
   {
      //
      // Result is in microvolts, scale to the right value
      //
      int32_t adcval = cyhal_adc_read(&adc_chan_obj[i]);
      data[i] = adcval * 16 ;
   }

   data[FRC_HID_AXIS_COUNT - 2] = 0x1425 ;
   data[FRC_HID_AXIS_COUNT - 1] = 0x7034 ;

   *buttons = 0 ;
   for(int i = 0 ; i < FRC_HID_INPUT_COUNT ; i++)
   {
      if (cyhal_gpio_read(get_input_pin(i)))
      {
         (*buttons) |= (1 << i) ;
      }
   }
}

static void adc_event_handler(void* arg, cyhal_adc_event_t event)
{
   memcpy(result_cached, result_arr, sizeof(result_arr));
   adc_read_in_progress = 0 ;
}

int hw_init()
{
   cy_rslt_t res ;

   res = cyhal_adc_init(&adcobj, P10_0, NULL) ;
   if (res != CY_RSLT_SUCCESS)
   {
        printf("FRCHID: ADC initialization failed. Error: %lx\n", (long unsigned int)res);
        return 0 ;
   }

   for(int i = 0 ; i < FRC_HID_AXIS_COUNT - 2 ; i++) {
      res = cyhal_adc_channel_init_diff(&adc_chan_obj[i], &adcobj, adc_pins[i], CYHAL_ADC_VNEG, &channel_config) ;
      if (res != CY_RSLT_SUCCESS)
      {
         printf("FRCHID: ADC channel %d initialization failed. Error: %lx\n", i, (long unsigned int)res);
         return 0 ;
      }
   }

   cyhal_adc_register_callback(&adcobj, &adc_event_handler, result_arr);
   cyhal_adc_enable_event(&adcobj, CYHAL_ADC_ASYNC_READ_COMPLETE, CYHAL_ISR_PRIORITY_DEFAULT, true);
   memset(result_cached, 0, sizeof(result_cached));

   for(int i = 0 ; i < FRC_HID_INPUT_COUNT ; i++) {
      int pin = get_input_pin(i) ;

      res = cyhal_gpio_init(pin, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLDOWN, false);
      if (res != CY_RSLT_SUCCESS)
      {
         printf("FRCHID: GPIO input pin %d initialization failed. Error: %lx\n", i, (long unsigned int)res);
         return 0 ;
      }
   }

   for(int i = 0 ; i < FRC_HID_OUTPUT_COUNT ; i++) {
      res = cyhal_gpio_init(out_pins[i], CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false);
      if (res != CY_RSLT_SUCCESS)
      {
         printf("FRCHID: GPIO output pin %d initialization failed. Error: %lx\n", i, (long unsigned int)res);
         return 0 ;
      }
   }

   return 1 ;
}
