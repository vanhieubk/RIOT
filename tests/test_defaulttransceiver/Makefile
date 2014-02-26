export PROJECT = test_defaulttransceiver
export BOARD ?= native
export RIOTBASE ?= $(CURDIR)/../..
QUIET ?= 1

BOARD_BLACKLIST ?= # allow same-style adding to the list below
BOARD_BLACKLIST += msb-430 # https://github.com/RIOT-OS/RIOT/issues/658
BOARD_BLACKLIST += pttu # https://github.com/RIOT-OS/RIOT/issues/659
BOARD_BLACKLIST += mbed_lpc1768 # https://github.com/RIOT-OS/RIOT/issues/675
BOARD_BLACKLIST += redbee-econotag # https://github.com/RIOT-OS/RIOT/issues/676

USEMODULE += auto_init
USEMODULE += defaulttransceiver

include $(RIOTBASE)/Makefile.include
