export PROJECT = test_net_if

BOARD_WHITELIST = native msba2 telosb

include ../Makefile.tests_common

ifeq ($(BOARD),stm32f4discovery)
	include Makefile.$(BOARD)
endif

USEMODULE += auto_init
USEMODULE += net_if
USEMODULE += defaulttransceiver

export INCLUDES += -I$(RIOTBASE)/sys/net/include

ifeq ($(BOARD),msba2)
	export INCLUDES += -I$(RIOTBASE)/drivers/cc110x_ng/include \
					   -I$(RIOTBASE)/boards/msba2-common/include
endif

include $(RIOTBASE)/Makefile.include
