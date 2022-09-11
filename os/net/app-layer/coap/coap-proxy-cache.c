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
 *      CoAP proxy cache implementation.
 * \author
 *      Ismael Amezcua Valdovinos <ismaelamezcua@ucol.mx>
 *      Patricia Elizabeth Figueroa Millan <patricia.figueroa@colima.tecnm.mx>
 */

/**
 * \addtogroup coap-proxy
 * @{
 */

#include "coap-engine.h"
#include "coap-proxy-cache.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "ctimer.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap-proxy"
#define LOG_LEVEL  LOG_LEVEL_COAP

static struct ctimer timer;

/*---------------------------------------------------------------------------*/
MEMB(cache_memb, coap_proxy_cache_entry_t, COAP_MAX_OPEN_TRANSACTIONS);
LIST(cache_list);
/*---------------------------------------------------------------------------*/
static void
remove_timed_cache_entry(void *cache_uri_ptr)
{
  coap_proxy_cache_entry_t *entry = NULL;
  char cache_uri[128];

  strncpy(cache_uri, cache_uri_ptr, sizeof(cache_uri) - 1);
  cache_uri[sizeof(cache_uri) - 1] = '\n';

  LOG_DBG("Cache entry timer expired for %s. Removing from list.\n", cache_uri);

  entry = coap_proxy_get_cache_by_uri(cache_uri);
  if(entry) {
    /* Cache hit, remove from list */
    coap_proxy_clear_cache_entry(entry);
  }
}
/*---------------------------------------------------------------------------*/
void
coap_proxy_new_cache_entry(char proxy_uri[], char payload[], uint32_t max_age)
{
  coap_proxy_cache_entry_t *entry = memb_alloc(&cache_memb);
  if(max_age == 0) {
    max_age = 60;
  }

  if(entry) {
    memcpy(entry->proxy_uri, proxy_uri, strlen(proxy_uri));
    memcpy(entry->payload, payload, strlen(payload));
    ctimer_set(&timer, max_age * CLOCK_SECOND, remove_timed_cache_entry, proxy_uri);

    LOG_DBG("Created a cache entry for URI %s with PAYLOAD: %s: %p\n",
            entry->proxy_uri, entry->payload, entry);
    list_add(cache_list, entry);
  }
}
/*---------------------------------------------------------------------------*/
void
coap_proxy_clear_cache_entry(coap_proxy_cache_entry_t *entry)
{
  if(entry) {
    LOG_DBG("Freeing cache entry %s: %p\n", entry->proxy_uri, entry);
    list_remove(cache_list, entry);
    memb_free(&cache_memb, entry);
  }
}
/*---------------------------------------------------------------------------*/
coap_proxy_cache_entry_t *
coap_proxy_get_cache_by_uri(char proxy_uri[])
{
  coap_proxy_cache_entry_t *entry = NULL;

  for(entry = (coap_proxy_cache_entry_t *)list_head(cache_list); entry; entry = entry->next) {
    if((strcmp(entry->proxy_uri, proxy_uri)) == 0) {
      LOG_DBG("Found cache entry for %s: %p\n", proxy_uri, entry);
      return entry;
    }
  }

  return NULL;
}
/** @}*/
