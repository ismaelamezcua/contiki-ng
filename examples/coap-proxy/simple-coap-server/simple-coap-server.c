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
 *         This is a simple CoAP server that simulates a humidity and
 *         temperature sensors.
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
#include "coap-engine.h"
#include "node-id.h"

#include "sys/log.h"
#define LOG_MODULE "CoAP Server"
#define LOG_LEVEL LOG_LEVEL_INFO

extern coap_resource_t res_humidity, res_temperature;

PROCESS(simple_coap_server, "Simple CoAP Server");
AUTOSTART_PROCESSES(&coap_server);

PROCESS_THREAD(simple_coap_server, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();

  LOG_INFO("Simple CoAP server for Node %d has started.\n", node_id);

  /*
   * Bind the resources to their Uri-Path.
   */
  coap_activate_resource(&res_humidity, "sensors/humidity");
  coap_activate_resource(&res_temperature, "sensors/temperature");

  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
