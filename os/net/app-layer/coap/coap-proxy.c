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
#include "lib/random.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap-eng"
#define LOG_LEVEL  LOG_LEVEL_COAP

/* Volatile cache storage */
coap_proxy_enpoints_t proxy_endpoints;
/*---------------------------------------------------------------------------*/
int
coap_proxy_receive(const coap_endpoint_t *src,
                   uint8_t *payload, uint16_t payload_length)
{
  /* static declaration reduces stack peaks and program code size */
  static coap_message_t message[1]; /* this way the message can be treated as pointer as usual */
  static coap_message_t response[1];
  static coap_message_t request[1];
  coap_transaction_t *transaction = NULL;

  coap_status_code = coap_parse_message(message, payload, payload_length);
  coap_set_src_endpoint(message, src);

  if(coap_status_code == NO_ERROR) {

    LOG_DBG("  COAP-PROXY Parsed: v %u, t %u, tkl %u, c %u, mid %u\n", message->version,
            message->type, message->token_len, message->code, message->mid);
    LOG_DBG("  URL:");
    LOG_DBG_COAP_STRING(message->uri_path, message->uri_path_len);
    LOG_DBG_("\n");
    LOG_DBG("  Payload: ");
    LOG_DBG_COAP_STRING((const char *)message->payload, message->payload_len);
    LOG_DBG_("\n");

    /*
     * Check if the packet is "normal" or proxied.
     * When the proxy_uri_len is present, it means that we need to 
     * create a new request to the proxy_uri. If not, we MUST assume
     * the packet is a response from the target proxied node, so we
     * should gather its payload and create a response to the client.
     */
    if(message->proxy_uri_len > 0) {
      LOG_DBG("We must process the packet.");
    } else {
      LOG_DBG("We must process differently this packet.");
    }

    /*
      * According to FIGURE 20 in RFC7252, we can issue a response with
      * code 0 if the request is confirmable until the Proxy-Uri node
      * sends its response. However, we MUST ensure the request is BLOCKING
      * to wait for the response from the target.
      */
    uint16_t mid = random_rand();
    coap_init_message(request, COAP_TYPE_CON, COAP_GET, mid);
    coap_set_header_uri_path(request, message->proxy_uri);

    /* Handle requests */
    if(message->code >= COAP_GET && message->code <= COAP_DELETE) {
      if((transaction = coap_new_transaction(message->mid, src))) {
        /* send response only if message type is CON */
        if(message->type == COAP_TYPE_CON) {
          coap_init_message(response, COAP_TYPE_ACK, 0, message->mid);
        }
        if(message->token_len) {
          coap_set_token(response, message->token, message->token_len);
        }

        /* do not support BLOCK transfers yet */
        if((transaction->message_len = coap_serialize_message(response, transaction->message)) == 0) {
          coap_status_code = PACKET_SERIALIZATION_ERROR;
        }
        if(transaction) {
          coap_send_transaction(transaction);
        }
      }
    }


    // TODO: Maybe necessary to create a transaction to send an ACK?
    // if(message->type == COAP_TYPE_CON) {
    //   coap_init_message(response, COAP_TYPE_ACK, 0, message->mid);
    // }
    // coap_error_message = "No Error";
    // coap_set_payload(message, coap_error_message, strlen(coap_error_message));
    // coap_sendto(src, payload, coap_serialize_message(message, payload));

    // LOG_DBG("This was made by the proxy. We need to make another request and match to the original MID");

    return NO_ERROR;
  }

  return coap_status_code;
}