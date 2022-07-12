CONTIKI_PROJECT = coap-client
all: $(CONTIKI_PROJECT)

# Sky has a code size limitation
PLATFORMS_EXCLUDE = sky z1

CONTIKI=../../..

MAKE_COAP_SERVER_ADDR ?= 0 # force server address from command line

# include the CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

include $(CONTIKI)/Makefile.include