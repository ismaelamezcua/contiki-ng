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
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      CoAP proxy implementation.
 * \author
 *      Ismael Amezcua Valdovinos <ismaelamezcua@ucol.mx>
 *      Patricia Elizabeth Figueroa Millan <patricia.figueroa@colima.tecnm.mx>
 */

/**
 * \addtogroup coap
 * @{
 */

#include "coap-proxy.h"
#include "coap-engine.h"
#include "coap-transactions.h"
#include "uiplib.h"
#include "lib/random.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap-proxy"
#define LOG_LEVEL  LOG_LEVEL_COAP

transaction_pair_t transactions;
/*---------------------------------------------------------------------------*/
/*- Internal Proxy Engine ---------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void
handle_proxy_request(coap_message_t message[], const coap_endpoint_t *endpoint)
{
  static coap_message_t request[1];
  coap_endpoint_t target_endpoint;
  char source_address[128];

  LOG_DBG("  Handling a request with mid %u.\n", message->mid);

  transactions.source = coap_new_transaction(message->mid, endpoint);
  transactions.mid = message->mid;

  /* Sending a new request to the target before responding to source */
  if(message->proxy_uri_len) {
    /* Extract the IP address from Proxy-Uri */
    uiplib_ipaddr_snprint(source_address, sizeof(source_address), &endpoint->ipaddr);

    /* Extract Uri-Path from the Proxy-Uri option */
    char *locate_bracket = strchr(message->proxy_uri, ']');
    char *request_path = locate_bracket + 1;

    /* Creates a new endpoint for the proxy and the target */
    if((coap_endpoint_parse(message->proxy_uri, message->proxy_uri_len, &target_endpoint)) == 0) {
      LOG_DBG("  Error: Could not create endpoint for ");
      LOG_DBG_COAP_STRING(message->proxy_uri, message->proxy_uri_len);
      LOG_DBG_("\n");
      coap_status_code = ERROR_RESPONSE_CODE;

      return;
    }

    /* Sends a COAP GET request to the target server */
    uint16_t new_mid = coap_get_mid();
    if((transactions.target = coap_new_transaction(new_mid, &target_endpoint))) {
      coap_init_message(request, message->type, COAP_GET, new_mid);
      coap_set_header_uri_path(request, request_path);
      if((transactions.target->message_len = coap_serialize_message(request, transactions.target->message)) == 0) {
        coap_status_code = PACKET_SERIALIZATION_ERROR;

        return;
      }

      coap_send_transaction(transactions.target);
      LOG_INFO("Handling proxy request with path %s to ", request_path);
      LOG_INFO_COAP_EP(&target_endpoint);
      LOG_INFO_("\n");
    }
  }

  return;
}
void
handle_proxy_response(coap_message_t message[], const coap_endpoint_t *endpoint)
{
  coap_message_t source_response[1];

  LOG_DBG("  Handling a response with mid %u.\n", message->mid);

  /* Send the response back to the source node */
  coap_init_message(source_response, message->type, CONTENT_2_05, transactions.source->mid);

  if(message->token_len) {
    coap_set_token(source_response, message->token, message->token_len);
  }

  coap_set_header_content_format(source_response, APPLICATION_JSON);
  coap_set_payload(source_response, message->payload, message->payload_len);

  if((transactions.source->message_len = coap_serialize_message(source_response, transactions.source->message)) == 0) {
    coap_status_code = PACKET_SERIALIZATION_ERROR;
  }

  coap_send_transaction(transactions.source);

  LOG_INFO("Received response from ");
  LOG_INFO_COAP_EP(endpoint);
  LOG_INFO_(" with payload: ");
  LOG_INFO_COAP_STRING((char *)message->payload, message->payload_len);
  LOG_INFO_("\n");

  LOG_INFO("Sending data back to ");
  LOG_INFO_COAP_EP(&transactions.source->endpoint);
  LOG_INFO_("\n");

  if(message->type == COAP_TYPE_CON && message->code == 0) {
    LOG_INFO("Received a Ping.\n");
    coap_status_code = PING_RESPONSE;
  } else if(message->type == COAP_TYPE_ACK) {
    /* Transactions are closed through lookup below */
    LOG_DBG("  Received ACK.\n");
  } else if(message->type == COAP_TYPE_RST) {
    LOG_INFO("Received RST.\n");
    /* Cancel possible subscription */
    coap_remove_observer_by_mid(endpoint, message->mid);
  }

  /* Free transaction memory before Callback, as it may create a new Transaction */
  coap_resource_response_handler_t callback = transactions.target->callback;
  void *callback_data = transactions.target->callback_data;

  coap_clear_transaction(transactions.target);

  /* Check if a Callback is registered */
  if(callback) {
    callback(callback_data, message);
  }

  coap_status_code = NO_ERROR;

  return;
}
/*---------------------------------------------------------------------------*/
int
coap_proxy_receive(const coap_endpoint_t *src,
                   uint8_t *payload, uint16_t payload_length)
{
  coap_message_t message[1];

  coap_status_code = coap_parse_message(message, payload, payload_length);

  LOG_DBG("  Handle request: version %u, type %u, token %s, code %u, mid %u\n", message->version,
          message->type, (char *)message->token, message->code, message->mid);
  LOG_DBG("  URL: ");
  LOG_DBG_COAP_STRING(message->uri_path, message->uri_path_len);
  LOG_DBG_("\n");
  LOG_DBG("  Payload: ");
  LOG_DBG_COAP_STRING((const char *)message->payload, message->payload_len);
  LOG_DBG_("\n");
  if(message->proxy_uri_len) {
    LOG_DBG("  Proxy-Uri: ");
    LOG_DBG_COAP_STRING(message->proxy_uri, message->proxy_uri_len);
    LOG_DBG_("\n");
  }

  if(coap_status_code == NO_ERROR) {
    if(message->code >= COAP_GET && message->code <= COAP_DELETE) {
      /* Handle requests */
      handle_proxy_request(message, src);
    } else {
      /* Handle responses */
      handle_proxy_response(message, src);
    }
  }
  return coap_status_code;
}