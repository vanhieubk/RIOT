export PROJECT = test_anytransceiver
export BOARD ?= native
export RIOTBASE ?= $(CURDIR)/../..
QUIET ?= 1

USEMODULE += auto_init
USEMODULE += anytransceiver
USEMODULE += hwtimer

ifneq (,$(findstring msba2, $(BOARD)))
       export INCLUDES += -I$(RIOTBASE)/drivers/cc110x_ng/include
endif

include $(RIOTBASE)/Makefile.include
