#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "jerryscript-port.h"

static void start_jerryscript()
{
  /* Initialize engine */
  jerry_init(JERRY_INIT_EMPTY);
}

void app_main()
{
  // init jerryscript
  start_jerryscript();
  while (true)
  {
    // alive check here. but nothing to do now!
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  /* Cleanup engine */
  jerry_cleanup();
}