CONTIKI_PROJECT = simple-coap-client
all: $(CONTIKI_PROJECT)

# Exclude Sky because of its code size limitation
PLATFORMS_EXCLUDE = sky z1

CONTIKI=../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

include $(CONTIKI)/Makefile.include
