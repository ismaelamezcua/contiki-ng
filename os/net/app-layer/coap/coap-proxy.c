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
#include "lib/memb.h"
#include "lib/list.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap-proxy"
#define LOG_LEVEL  LOG_LEVEL_COAP

/*---------------------------------------------------------------------------*/
MEMB(tp_memb, coap_transaction_pair_t, COAP_MAX_OPEN_TRANSACTIONS);
LIST(tp_list);
/*---------------------------------------------------------------------------*/
void
coap_new_transaction_pair(uint16_t mid, coap_transaction_t *s, coap_transaction_t *t)
{
  coap_transaction_pair_t *tp = memb_alloc(&tp_memb);

  if(tp) {
    tp->mid = mid;
    tp->source = s;
    tp->target = t;

    list_add(tp_list, tp);
    LOG_DBG("Created a coap_transaction_pair_t LIST\n");
  }
}
/*---------------------------------------------------------------------------*/
void
coap_clear_transaction_pair(coap_transaction_pair_t *tp)
{
  if(tp) {
    LOG_DBG("Freeing transaction pair %u: s -> %p, t -> %p", tp->mid, tp->source, tp->target);
    list_remove(tp_list, tp);
    memb_free(&tp_memb, tp);
  }
}
/*---------------------------------------------------------------------------*/
coap_transaction_pair_t *
coap_get_transaction_pair_by_mid(uint16_t mid)
{
  coap_transaction_pair_t *tp = NULL;

  for(tp = (coap_transaction_pair_t *)list_head(tp_list); tp; tp = tp->next) {
    if(tp->mid == mid) {
      LOG_DBG("Found transaction pair for MID %u: s -> %p, t -> %p\n", tp->mid, tp->source, tp->target);
      return tp;
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
/*- Internal Proxy Engine ---------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void
handle_proxy_request(coap_message_t message[], const coap_endpoint_t *endpoint)
{
  static coap_message_t request[1];
  coap_endpoint_t target_endpoint;
  coap_transaction_t *source_transaction;
  coap_transaction_t *target_transaction;
  char source_address[128];

  LOG_DBG("  Handling a request with mid %u.\n", message->mid);

  source_transaction = coap_new_transaction(message->mid, endpoint);

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
    if((target_transaction = coap_new_transaction(new_mid, &target_endpoint))) {
      /* Create a transaction_pair LIST */
      coap_new_transaction_pair(new_mid, source_transaction, target_transaction);

      coap_init_message(request, message->type, COAP_GET, new_mid);
      coap_set_header_uri_path(request, request_path);
      if((target_transaction->message_len = coap_serialize_message(request, target_transaction->message)) == 0) {
        coap_status_code = PACKET_SERIALIZATION_ERROR;

        return;
      }

      coap_send_transaction(target_transaction);
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
  coap_transaction_pair_t *transaction_pair;
  coap_transaction_t *source_transaction;
  coap_transaction_t *target_transaction;

  LOG_DBG("  Handling a response with mid %u.\n", message->mid);

  /* Get the transaction pair from the message MID */
  transaction_pair = coap_get_transaction_pair_by_mid(message->mid);
  source_transaction = transaction_pair->source;
  target_transaction = transaction_pair->target;

  /* Send the response back to the source node */
  coap_init_message(source_response, message->type, CONTENT_2_05, source_transaction->mid);

  if(message->token_len) {
    coap_set_token(source_response, message->token, message->token_len);
  }

  coap_set_header_content_format(source_response, APPLICATION_JSON);
  coap_set_payload(source_response, message->payload, message->payload_len);

  if((source_transaction->message_len = coap_serialize_message(source_response, source_transaction->message)) == 0) {
    coap_status_code = PACKET_SERIALIZATION_ERROR;
  }

  coap_send_transaction(source_transaction);

  LOG_INFO("Received response from ");
  LOG_INFO_COAP_EP(endpoint);
  LOG_INFO_(" with payload: ");
  LOG_INFO_COAP_STRING((char *)message->payload, message->payload_len);
  LOG_INFO_("\n");

  LOG_INFO("Sending data back to ");
  LOG_INFO_COAP_EP(&source_transaction->endpoint);
  LOG_INFO_("\n");

  /* Remove the transaction_pair from the LIST */
  coap_clear_transaction_pair(transaction_pair);

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
  coap_resource_response_handler_t callback = target_transaction->callback;
  void *callback_data = target_transaction->callback_data;

  coap_clear_transaction(target_transaction);

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