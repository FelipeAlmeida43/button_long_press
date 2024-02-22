#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/xtensa_api.h"
#include "freertos/xtensa_config.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#define LED_STATUS  2
#define BTN_AP      4 // Replace with your actual GPIO pin for BTN_AP
#define LONG_PRESS_TIME 3000 // Button long press time in milliseconds

portMUX_TYPE ISR_MUX = portMUX_INITIALIZER_UNLOCKED;

static bool keyPressDetected = false,
            keyPressDown = false;

TickType_t keyPressStartTime = 0;
uint32_t counterLedblink =0;
enum INPUT_CHANNELS{
  ID_0,
  NUMBER_OF_INPUT_CHANNELS
};
struct DigitalChannels {

  unsigned int pin;
  volatile long  count;
  volatile long  debounce;
  volatile long  debounce_target;
  volatile bool last_state;
  volatile bool current_state;
  volatile bool isPressed;
  void* id;

};
struct DigitalChannels Channel_0 = { BTN_AP, 0, 0, 50, 1, 1, (void*)ID_0 ,0};
// Prototypes
void IRAM_ATTR ISR_Read_Inputs(void* channel);
//***********************************************************
void button(void *pvParameter) {
    while (1) {
        if (keyPressDetected) {
            
            TickType_t keyPressDuration = xTaskGetTickCount() - keyPressStartTime;
            if (keyPressDuration >= pdMS_TO_TICKS(LONG_PRESS_TIME) && gpio_get_level(BTN_AP) ==0) {
                keyPressDown = true;
                
            }
        }

        if (keyPressDown) {
            
            printf("LONG PRESS\r\n");
            keyPressDown = false;
            keyPressDetected = false;
            
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void IRAM_ATTR ISR_Read_Inputs(void* channel) {
    if (channel == Channel_0.id) {
        portENTER_CRITICAL(&ISR_MUX);
        Channel_0.last_state = 0;
         Channel_0.debounce =1;
        
         keyPressStartTime = xTaskGetTickCount();
        gpio_isr_handler_remove(BTN_AP);
        portEXIT_CRITICAL(&ISR_MUX);
    }
}

void initializeButton() {
    // Replace BTN_AP_PIN with the actual GPIO pin for your button
    gpio_config_t buttonConfig = {
        .pin_bit_mask = (1ULL<<BTN_AP),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&buttonConfig);

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(BTN_AP, ISR_Read_Inputs, (void *)Channel_0.id);

    gpio_set_direction(LED_STATUS, GPIO_MODE_OUTPUT);
	gpio_config_t io_config;                     // Descriptor Variable of GPIO driver
	io_config.intr_type = GPIO_INTR_DISABLE; // Disable interrupt resource
	io_config.mode = GPIO_MODE_OUTPUT;
	io_config.pin_bit_mask =(1ULL<<LED_STATUS) ;
	gpio_config(&io_config);
    
    
    xTaskCreatePinnedToCore(button, "button", 8192, NULL, 10, NULL,1);
}
static void ISR_HW_Timer_0(void *arg) {

	portENTER_CRITICAL(&ISR_MUX);
	  if (Channel_0.debounce > 0)
	  	  {
	  		  Channel_0.debounce++;
	  		  
	  	     if (Channel_0.debounce > Channel_0.debounce_target)
	  	     {

	  	    	 Channel_0.current_state = gpio_get_level(BTN_AP);
	  	    	 
	  	    	 if (Channel_0.last_state == Channel_0.current_state)
	  	    	 {
	  	    		 	
	  	    		 	Channel_0.isPressed = true;
                        keyPressDetected = true;
	  	                Channel_0.last_state = Channel_0.current_state;
	  	                Channel_0.count++;
	  	                Channel_0.debounce = 0;
                       
	  	                gpio_isr_handler_add(BTN_AP, ISR_Read_Inputs, (void *)Channel_0.id);

	  	         }

	  	      }

	  	   }
      counterLedblink++;
	  portEXIT_CRITICAL(&ISR_MUX);
}
void initializeTimer0(){

	 const esp_timer_create_args_t timer1_args = {
			  		      .callback = &ISR_HW_Timer_0,
			  		      .name = "Timer 1"};
			  		esp_timer_handle_t timer1_handler;
			  		ESP_ERROR_CHECK(esp_timer_create(&timer1_args, &timer1_handler));
			  		ESP_ERROR_CHECK(esp_timer_start_periodic(timer1_handler, 80));

}
void app_main() {
    initializeButton();
    initializeTimer0();
     int status =0;
    while(1){
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if(counterLedblink>5000){
            counterLedblink=0;
            status = !status;
           //printf("BLINKING\r\n");
            gpio_set_level(LED_STATUS,status);
        }
    }
    
}
// ... rest of the code (initializeTimer0, initializeButton, app_main) ...

// Add this function if needed
void handleLongPress() {
    // Perform actions here when a long press is detected
    // ...
}
