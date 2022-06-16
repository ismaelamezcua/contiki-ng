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
 *         An observable humidity CoAP resource.
 *
 * \author
 *         Ismael Amezcua Valdovinos <ismaelamezcua@ucol.mx>
 *         Patricia Elizabeth Figueroa Millan <patricia.figueroa@colima.tecnm.mx>
 *
 */

#include <stdio.h>
#include <string.h>
#include "virtual-sensor.h"
#include "coap-engine.h"

static void res_get_handler(coap_message_t *request,
                            coap_message_t *response,
                            uint8_t *buffer,
                            uint16_t preferred_size,
                            int32_t *offset);

static void
res_periodic_handler(void);

PERIODIC_RESOURCE(
  res_humidity,
  "title=\"Humidity Sensor\";rt=\"humidity\";obs",
  res_get_handler,
  NULL,
  NULL,
  NULL,
  5000,
  res_periodic_handler
  );

static void
res_get_handler(coap_message_t *request,
                coap_message_t *response,
                uint8_t *buffer,
                uint16_t preferred_size,
                int32_t *offset)
{
  float humidity = read_humidity();
  unsigned int accept = -1;

  if(accept == -1) {
    coap_set_header_content_format(response, APPLICATION_JSON);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "{\"humidity\": \"%.2f\"}", humidity);
    coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else {
    coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
    const char *message = "Only application/json as Content-Type is supported.";
    coap_set_payload(response, message, strlen(message));
  }
}
static void
res_periodic_handler()
{
  coap_notify_observers(&res_humidity);
}
