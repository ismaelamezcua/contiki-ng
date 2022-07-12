#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#define TOGGLE_INTERVAL 10

PROCESS(coap_client, "CoAP Client");
AUTOSTART_PROCESSES(&coap_client);

static struct etimer et;

char *service_urls[1] = { ".well-known/core" };

/*
 * This function will be passed to COAP_BLOCKING_REQUEST()
 * to handle responses
 */
void
client_chunk_handler(coap_message_t *response)
{
  const uint8_t *chunk;

  if (response == NULL) {
    puts("Request timed out.");
    return;
  }

  int len = coap_get_payload(response, &chunk);

  printf("|%.*s", len, (char *)chunk);
}

PROCESS_THREAD(coap_client, ev, data)
{
  static coap_endpoint_t server_ep;

  PROCESS_BEGIN();

  // This way the packet can be treated as a pointer
  static coap_message_t request[1];

  coap_endpoint_parse(COAP_SERVER_ADDR, strlen(COAP_SERVER_ADDR), &server_ep);

  etimer_set(&et, TOGGLE_INTERVAL * CLOCK_SECOND);

  while (1) {
    PROCESS_YIELD();
  
    if (etimer_expired(&et)) {
      printf("-- Toggle timer --\n");

      // prepare the requet, TID is set by COAP_BLOCKING_REQUEST()
      coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(request, service_urls[1]);

      const char msg[] = "Toggle!";

      coap_set_payload(request, (uint8_t *)msg, sizeof(msg) -1);

      LOG_INFO_COAP_EP(&server_ep);
      LOG_INFO_("\n");

      COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

      printf("\n -- Done --\n");

      etimer_reset(&et);
    }
  
  }

  PROCESS_END();
}
