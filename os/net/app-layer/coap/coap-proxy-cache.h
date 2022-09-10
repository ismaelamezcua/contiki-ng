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
 *      CoAP proxy cache implementation header file.
 * \author
 *      Ismael Amezcua Valdovinos <ismaelamezcua@ucol.mx>
 *      Patricia Elizabeth Figueroa Millan <patricia.figueroa@colima.tecnm.mx>
 */

/**
 * \addtogroup coap-proxy
 * @{
 */

#ifndef COAP_PROXY_CACHE_H_
#define COAP_PROXY_CACHE_H_

#include "coap-engine.h"

typedef struct coap_proxy_cache_entry {
  struct coap_proxy_cache_entry *next;

  char proxy_uri[128];
  char payload[128];
} coap_proxy_cache_entry_t;

void
coap_proxy_new_cache_entry(char proxy_uri[], char payload[]);

void
coap_proxy_clear_cache_entry(coap_proxy_cache_entry_t *entry);

coap_proxy_cache_entry_t *
coap_proxy_get_cache_by_uri(char proxy_uri[]);

#endif /* COAP_PROXY_CACHE_H_ */
/** @} */
