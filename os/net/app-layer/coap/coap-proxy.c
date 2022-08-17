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

char source_address[128];
uint16_t source_mid = 0;

/*---------------------------------------------------------------------------*/
int
coap_proxy_receive(const coap_endpoint_t *src,
                   uint8_t *payload, uint16_t payload_length)
{
  /* static declaration reduces stack peaks and program code size */
  static coap_message_t message[1]; /* this way the message can be treated as pointer as usual */
  static coap_message_t response[1];
  coap_transaction_t *transaction = NULL;

  coap_status_code = coap_parse_message(message, payload, payload_length);
  coap_set_src_endpoint(message, src);

  if(coap_status_code == NO_ERROR) {

    LOG_DBG("  CoAP Proxy Parsed: v %u, t %u, tkl %u, c %u, mid %u\n", message->version,
            message->type, message->token_len, message->code, message->mid);
    LOG_DBG("  URL:");
    LOG_DBG_COAP_STRING(message->uri_path, message->uri_path_len);
    LOG_DBG_("\n");
    LOG_DBG("  Payload: ");
    LOG_DBG_COAP_STRING((const char *)message->payload, message->payload_len);
    LOG_DBG_("\n");
    LOG_DBG_("  Client wants to reach: ");
    LOG_DBG_COAP_STRING(message->proxy_uri, message->proxy_uri_len);
    LOG_DBG_("\n");

    /* Handle requests */
    if(message->code >= COAP_GET && message->code <= COAP_DELETE) {
      static coap_endpoint_t target_server;
      static coap_message_t target_request[1];
      coap_transaction_t *target_transaction = NULL;

      if((coap_endpoint_parse(message->proxy_uri, message->proxy_uri_len, &target_server))) {
        /*
         * If the proxy-uri node is not reachable, send 5.03 Service Unavailable.
         * Unfortunately, the function to checkif reachable does not work
         * since it will return true even if the endpoint does not exist.
         * The routing will be commented and scoped in case is needed in the future.
         */
        /* { */
        /*   if((coap_endpoint_is_connected(&target_server)) == 0) { */
        /*     LOG_DBG("  CoAP Proxy: The target endpoint is not reachable.\n"); */
        /*     if((transaction = coap_new_transaction(message->mid, src))) { */
        /*       if(message->type == COAP_TYPE_CON) { */
        /*         coap_init_message(response, COAP_TYPE_ACK, SERVICE_UNAVAILABLE_5_03, message->mid); */
        /*       } else { */
        /*         coap_init_message(response, COAP_TYPE_NON, SERVICE_UNAVAILABLE_5_03, coap_get_mid()); */
        /*       } */
        /*     } */

        /*     / * Mirror token * / */
        /*     if(message->token_len) { */
        /*       coap_set_token(response, message->token, message->token_len); */
        /*     } */

        /*     coap_status_code = SERVICE_UNAVAILABLE_5_03; */
        /*     coap_send_transaction(transaction); */

        /*     return coap_status_code; */
        /*   } */
        /* } */

        /*
         * We need to parse the ipaddr from the CoAP endpoint to store it as string
         * only when the Proxy-Uri option is present.
         * Avoid the use of coap_endpoint_copy() since it throws segmentation faults.
         * Also, store message MID for replying and freeing the transaction.
         */
        if(message->proxy_uri_len > 0) {
          uiplib_ipaddr_snprint(source_address, sizeof(source_address), &src->ipaddr);
          source_mid = message->mid;
          LOG_DBG("  Saving MID: %u\n", message->mid);
          LOG_DBG("  Saved MID: %u\n", source_mid);
        }
        
        /* Extract Uri-Path from the Proxy-Uri option */
        char *locate_bracket = strchr(message->proxy_uri, ']');
        char *request_path = locate_bracket + 1;

        uint16_t new_mid = coap_get_mid();
        target_transaction = coap_new_transaction(new_mid, &target_server);
        coap_set_src_endpoint(target_request, &target_server);
        coap_init_message(target_request, COAP_TYPE_CON, COAP_GET, new_mid);
        coap_set_token(target_request, message->token, message->token_len);
        coap_set_header_uri_path(target_request, request_path);
        target_transaction->message_len = coap_serialize_message(target_request, target_transaction->message);
        coap_send_transaction(target_transaction);
      }
    } else {
      /* Handle responses */

      /*
       * This SHOULD be a response from the target server with the
       * requested payload. We MUST create one transaction to send
       * an ACK to the target server and another transaction to
       * send the payload to the source client.
       */

      /* Transaction to send the payload to the source client */
      coap_transaction_t *source_transaction = NULL;
      // // coap_transaction_t *target_transaction = NULL;
      coap_endpoint_t source_endpoint;
      static coap_message_t source_message[1];

      coap_endpoint_parse(source_address, strlen(source_address), &source_endpoint);
      coap_endpoint_print(&source_endpoint);

      LOG_DBG("  Recovering a CoAP Transaction from source endpoint with MID: %d\n", source_mid);
      if((source_transaction = coap_get_transaction_by_mid(source_mid))) {
      // if((source_transaction = coap_new_transaction(coap_get_mid(), &source_endpoint))) {
      LOG_DBG("  Recovered CoAP Transaction with MID: %d\n", source_mid);
       // if((source_transaction = coap_new_transaction(coap_get_mid(), source_endpoint))) {
        coap_set_src_endpoint(source_message, &source_endpoint);
        coap_init_message(source_message, COAP_TYPE_CON, CONTENT_2_05, 0);
        // if(coap_get_header_content_format(message->content_format, content_format)) {
        //     coap_set_header_content_format(source_message, content_format);
        // }
        // coap_set_token(source_message, source_token, COAP_TOKEN_LEN);
        coap_set_payload(source_message, payload, payload_length);
        if((source_transaction->message_len = coap_serialize_message(response, transaction->message)) == 0) {
          coap_status_code = PACKET_SERIALIZATION_ERROR;
        }

        if(coap_status_code == NO_ERROR) {
          if(source_transaction) {
            coap_send_transaction(source_transaction);
            coap_clear_transaction(source_transaction);
          }
        }
      }
      LOG_DBG("  Done with sending message to the source node\n");

      // if((transaction = coap_new_transaction(message->mid, src))) {
      //   /*
      //    * Reliable CON request are answered with an ACK
      //    * Unreliable NON requesta are answered with a NON as well
      //    */
      //   if(message->type == COAP_TYPE_CON) {
      //     coap_init_message(response, COAP_TYPE_ACK, CONTENT_2_05, message->mid);
      //   } else {
      //     coap_init_message(response, COAP_TYPE_NON, CONTENT_2_05, coap_get_mid());
      //   }

      //   /* Mirror token */
      //   if(message->token_len) {
      //     coap_set_token(response, message->token, message->token_len);
      //   }

      //   /* We don't want to support blockwise transfers just yet */
      //   /* Simulate a CoAP Resource to send it to the client */
      //   /* char *buffer = "{\"hello\":\"world\"}"; */
      //   coap_set_header_content_format(response, APPLICATION_JSON);
      //   /* coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer)); */
      //   coap_set_payload(response, payload, payload_length);
      //   if((transaction->message_len = coap_serialize_message(response, transaction->message)) == 0) {
      //     coap_status_code = PACKET_SERIALIZATION_ERROR;
      //   }

      //   if(coap_status_code == NO_ERROR) {
      //     if(transaction) {
      //       coap_send_transaction(transaction);
      //     }
      //   }

      if(message->type == COAP_TYPE_CON && message->code == 0) {
        LOG_INFO("Received Ping\n");
        coap_status_code = PING_RESPONSE;
      } else if(message->type == COAP_TYPE_ACK) {
        /* transactions are closed through lookup below */
        LOG_DBG("Received ACK\n");
      } else if(message->type == COAP_TYPE_RST) {
        LOG_INFO("Received RST\n");
        /* cancel possible subscriptions */
        coap_remove_observer_by_mid(src, message->mid);
      }

      LOG_DBG("  Freeing transaction from target node\n");
      if((transaction = coap_get_transaction_by_mid(message->mid))) {
        /* free transaction memory before callback, as it may create a new transaction */
        coap_resource_response_handler_t callback = transaction->callback;
        void *callback_data = transaction->callback_data;

        coap_clear_transaction(transaction);

        /* check if someone registered for the response */
        if(callback) {
          callback(callback_data, message);
        }
      }
      /* if(ACKed transaction) */
      transaction = NULL;

#if COAP_OBSERVE_CLIENT
        /* if observe notification */
        if((message->type == COAP_TYPE_CON || message->type == COAP_TYPE_NON)
           && coap_is_option(message, COAP_OPTION_OBSERVE)) {
          LOG_DBG("Observe [%" PRId32 "]\n", message->observe);
          coap_handle_notification(src, message);
        }
#endif /* COAP_OBSERVE_CLIENT */

    }

    return NO_ERROR;
  }

  return coap_status_code;
}