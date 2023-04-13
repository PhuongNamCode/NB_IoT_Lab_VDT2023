/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "freertos/timers.h"
#include <driver/gpio.h>
#include "input_iot.h"
#include "freertos/event_groups.h"

TimerHandle_t xTimers[2];
EventGroupHandle_t xEventGroup;

#define BIT_EVENT_BUTTON_PRESS	( 1 << 0 )
#define BIT_EVENT_UART	        ( 1 << 1 )

void vTask1(void *pvParameters){
    for(;;){
        EventBits_t uxBits = xEventGroupWaitBits(
            xEventGroup,   /* The event group being tested. */
            BIT_EVENT_BUTTON_PRESS | BIT_EVENT_UART, /* The bits within the event group to wait for. */
            pdTRUE,        /* BIT_0 & BIT_4 should be cleared before returning. */
            pdFALSE,       /* Don't wait for both bits, either bit will do. */
            portMAX_DELAY);/* Wait a maximum of 100ms for either bit to be set. */
    
    if(uxBits & BIT_EVENT_BUTTON_PRESS)
    {
        printf("Button press\n");
        static int x;
        gpio_set_level(2,x);
        x = 1- x;
    }
    if(uxBits & BIT_EVENT_UART)
    {
        printf("UART\n");
    }
    
    }
  

}

// void vTimerCallback( TimerHandle_t xTimer )
// {
//     configASSERT( xTimer );
//     int ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );
//     if( ulCount==0)
//     {
//         static int x;
//         gpio_set_level(2,x);
//         x = 1- x;
//     }
//     else if( ulCount==1)
//     {
//         printf(" Hi \n");
//     }

// }
void button_callback(int pin)
{
    BaseType_t pxHigherPriorityTaskWoken;
    if( pin== GPIO_NUM_0)
    {
        xEventGroupSetBitsFromISR(xEventGroup, BIT_EVENT_BUTTON_PRESS, &pxHigherPriorityTaskWoken );
    }
}

void app_main(void)
{

    gpio_pad_select_gpio(2);
    gpio_set_direction(2, GPIO_MODE_INPUT_OUTPUT); 

    input_io_create(0,HI_TO_LO);
    input_set_callback(button_callback);

    // xTimers[0] = xTimerCreate( "TimerBlink",pdMS_TO_TICKS( 500 ),pdTRUE,( void * ) 0,vTimerCallback);
    // xTimers[1] = xTimerCreate( "TimerPrintf",pdMS_TO_TICKS( 500 ),pdTRUE,( void * ) 1,vTimerCallback);
    // xTimerStart(xTimers[0],0);
    // xTimerStart(xTimers[1],0);

    xEventGroup = xEventGroupCreate();

    xTaskCreate (vTask1, "Task1",1024,NULL,4,NULL);
 
}
