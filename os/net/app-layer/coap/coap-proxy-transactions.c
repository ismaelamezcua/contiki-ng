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
 *      CoAP proxy transactions implementation.
 * \author
 *      Ismael Amezcua Valdovinos <ismaelamezcua@ucol.mx>
 *      Patricia Elizabeth Figueroa Millan <patricia.figueroa@colima.tecnm.mx>
 */

/**
 * \addtogroup coap-proxy
 * @{
 */

#include "coap-engine.h"
#include "coap-transactions.h"
#include "coap-proxy-transactions.h"
#include "lib/memb.h"
#include "lib/list.h"
#include <string.h>

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap-proxy"
#define LOG_LEVEL  LOG_LEVEL_COAP

/*---------------------------------------------------------------------------*/
MEMB(tp_memb, coap_transaction_pair_t, COAP_MAX_OPEN_TRANSACTIONS);
LIST(tp_list);
/*---------------------------------------------------------------------------*/
void
coap_proxy_new_transaction_pair(uint16_t mid,
                                char cache_uri[],
                                coap_transaction_t *source,
                                coap_transaction_t *target)
{
  coap_transaction_pair_t *tp = memb_alloc(&tp_memb);

  if(tp) {
    tp->mid = mid;
    strcpy(tp->cache_uri, cache_uri);
    tp->source = source;
    tp->target = target;

    list_add(tp_list, tp);
    LOG_DBG("Created a new transaction pair: MID: %u, URI: %s\n", tp->mid, tp->cache_uri);
  }
}
/*---------------------------------------------------------------------------*/
void
coap_proxy_clear_transaction_pair(coap_transaction_pair_t *tp)
{
  if(tp) {
    LOG_DBG("Freeing transaction pair %u: s -> %p, t -> %p", tp->mid, tp->source, tp->target);
    list_remove(tp_list, tp);
    memb_free(&tp_memb, tp);
  }
}
/*---------------------------------------------------------------------------*/
coap_transaction_pair_t *
coap_proxy_get_transaction_pair_by_mid(uint16_t mid)
{
  coap_transaction_pair_t *tp = NULL;

  for(tp = (coap_transaction_pair_t *)list_head(tp_list); tp; tp = tp->next) {
    if(tp->mid == mid) {
      LOG_DBG("Found transaction pair for MID %u, URI: %s: s -> %p, t -> %p\n", tp->mid, tp->cache_uri, tp->source, tp->target);
      return tp;
    }
  }

  return NULL;
}
/** @}*/
