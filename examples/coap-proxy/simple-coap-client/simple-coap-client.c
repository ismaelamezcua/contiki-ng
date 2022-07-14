/*
 * Copyright (c) 2022, Universidad de Colima, Tecnologico Nacional de Mexico
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *         Simple CoAP client using Erbium (Er)
 *
 * \author
 *         Ismael Amezcua Valdovinos <ismaelamezcua@ucol.mx>
 *         Patricia Elizabeth Figueroa Millan <patricia.figueroa@colima.tecnm.mx>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "Simple CoAP Client"
#define LOG_LEVEL LOG_LEVEL_APP

#define SERVER_EP "coap://[fe80::201:1:1:1]"
#define TOGGLE_INTERVAL 10

PROCESS(simple_coap_client, "Simple CoAP Client");
AUTOSTART_PROCESSES(&simple_coap_client);

static struct etimer timer;

/* Example URIs that can be queried */
#define NUMBER_OF_URLS 3
char *service_urls[NUMBER_OF_URLS] =
{ ".well-known/core", "/sensors/temperature", "/sensors/humidity" };
static int uri_switch = 0;

/* This function will be passed to COAP_BLOCKING_REQUEST() to
   handle responses */
void
client_chunk_handler(coap_message_t *response)
{
  const uint8_t *chunk;

  if(response == NULL) {
    puts("Request timed out");
    return;
  }

  int length = coap_get_payload(response, &chunk);

  printf("|%.*s", length, (char *)chunk);
}
PROCESS_THREAD(simple_coap_client, ev, data)
{
  static coap_endpoint_t server_ep;
  PROCESS_BEGIN();

  static coap_message_t request[1];

  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

  etimer_set(&timer, TOGGLE_INTERVAL * CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&timer)) {
      printf("Timer triggered a request.\n");

      /* Send a request to each resource in a round-robin fashion */
      coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
      coap_set_header_uri_path(request, service_urls[uri_switch]);

      printf("Sending a request: %s\n", service_urls[uri_switch]);

      LOG_INFO_COAP_EP(&server_ep);
      LOG_INFO_("\n");

      COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

      printf("\n Done with the request.\n");

      uri_switch = (uri_switch + 1) % NUMBER_OF_URLS;
      etimer_reset(&timer);
    }
  }

  PROCESS_END();
}
