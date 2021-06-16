#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "jerryscript-port.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

static QueueHandle_t uart_queue;
static void uart_event_task(void *pvParameters)
{
  uart_event_t event;
  uint8_t *dtmp = (uint8_t *)malloc(1024 * 2);
  for (;;) {
    // Waiting for UART event.
    if (xQueueReceive(uart_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
      bzero(dtmp, 1024 * 2);
      switch (event.type) {
      /** 
       * We'd better handler data event fast, there would be much more data events than
       * other types of events. If we take too much time on data event, the queue might
       * be full.
       */
      case UART_DATA:
        uart_read_bytes(UART_NUM_0, dtmp, event.size, portMAX_DELAY);
        /* Setup Global scope code */
        jerry_value_t parsed_code = jerry_parse(dtmp, event.size, JERRY_PARSE_NO_OPTS);

        if (!jerry_value_is_error(parsed_code)) {
          /* Execute the parsed source code in the Global scope */
          jerry_value_t ret_value = jerry_run(parsed_code);

          /* Returned value must be freed */
          jerry_release_value(ret_value);
        } else {
          const char *ohno = "something was wrong!";
          uart_write_bytes(UART_NUM_0, ohno, strlen(ohno));
        }

        /* Parsed source code must be freed */
        jerry_release_value(parsed_code);
        // free(dtmp);
        break;
      //Event of UART ring buffer full
      case UART_BUFFER_FULL:
        // If buffer full happened, you should consider encreasing your buffer size
        // As an example, we directly flush the rx buffer here in order to read more data.
        uart_flush_input(UART_NUM_0);
        xQueueReset(uart_queue);
        break;
      //Others
      default:
        break;
      }
    }
  }
  free(dtmp);
  dtmp = NULL;
  vTaskDelete(NULL);
}

/**
 * Configure parameters of an UART driver, communication pins and install the driver
 * 
 * - Port: UART0
 * - Baudrate: 115200
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: on
 * - Pin assignment: TxD (default), RxD (default)
 */
static void handle_uart_input()
{
  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
  uart_param_config(UART_NUM_0, &uart_config);

  //Set UART pins (using UART0 default pins ie no changes.)
  uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  //Install UART driver, and get the queue.
  uart_driver_install(UART_NUM_0, 1024 * 2, 1024 * 2, 20, &uart_queue, 0);

  //Create a task to handler UART event from ISR
  xTaskCreate(uart_event_task, "uart_event_task", 1024 * 2, NULL, 12, NULL);
}

static jerry_value_t manual_handle_print(
  const jerry_call_info_t* function_object,
  const jerry_value_t arguments[],
  const jerry_length_t arguments_count
) {
  if (arguments_count > 0) {
    jerry_value_t string_value = jerry_value_to_string(arguments[0]);
    jerry_char_t buffer[256];
    jerry_size_t copied_bytes = jerry_string_to_utf8_char_buffer(string_value, buffer, sizeof(buffer) - 1);
    buffer[copied_bytes] = '\0';

    printf("%s\n", (const char*)buffer);
  }

  return jerry_create_undefined();
}

// switch the blue light
static jerry_value_t switch_light(
  const jerry_call_info_t* call_info,
  const jerry_value_t arguments[],
  const jerry_length_t arguments_count
) {
  jerry_value_t string_value = jerry_value_to_string(arguments[0]);
  jerry_char_t buffer[16];
  jerry_size_t copied_bytes = jerry_string_to_utf8_char_buffer(string_value, buffer, sizeof(buffer) - 1);
  buffer[copied_bytes] = '\0';

  printf("Turning %s the LED\n", buffer);

  gpio_pad_select_gpio(BLINK_GPIO);

  /* set the gpio as a push/pull output */
  gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

  if(strcmp((const char*)buffer, (const char*)"On")) {
    gpio_set_level(BLINK_GPIO, 0);
  } else {
    gpio_set_level(BLINK_GPIO, 1);
  }

  return jerry_create_undefined();
}

static void handler_register()
{
  /* Register 'print' function from the extensions */
  // jerryx_handler_register_global ((const jerry_char_t *) "print", jerryx_handler_print);

  jerryx_handler_register_global ((const jerry_char_t *) "switch_light", switch_light);

  {
    jerry_value_t global_object = jerry_get_global_object();
    jerry_value_t property_name = jerry_create_string((const jerry_char_t *)"print");
    jerry_value_t property_value = jerry_create_external_function(manual_handle_print);
    jerry_value_t result = jerry_set_property(global_object, property_name, property_value);

    if (jerry_value_is_error(result)) {
      printf("Failed to add 'print' property\n");
    }

    // Release all jerry_value_t
    jerry_release_value(result);
    jerry_release_value(property_value);
    jerry_release_value(property_name);
    jerry_release_value(global_object);
  }
}

void app_main()
{
  /* Initialize engine */
  jerry_init(JERRY_INIT_EMPTY);

  handler_register();


  handle_uart_input();

  while (true)
  {
    // alive check here. but nothing to do now!
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  /* Cleanup engine */
  jerry_cleanup();
}