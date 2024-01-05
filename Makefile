CC := gcc

RELEASE_DIR := release
BITNAME     := $(shell getconf LONG_BIT)
LIBNAME     := $(RELEASE_DIR)/lib/liblinux$(BITNAME)modbus
APP         := $(RELEASE_DIR)/bin/evoc_mb_demo
V           := 0

CFLAGS 	:= -g
CFLAGS 	+= -I $(RELEASE_DIR)/include
CFLAGS 	+= -Wall -Werror
CFLAGS 	+= -lpthread 

static=no
ifeq ($(static),yes)
	LIB_LINK = $(LIBNAME).a
else
	LIB_LINK = -llinux$(BITNAME)modbus -L $(RELEASE_DIR)/lib
endif

ifneq ($(V),99)
	HIDE=@
endif

ALL : clean
	@echo "EVOC ModBus compiling"
	$(HIDE) make --no-print-directory -f source/Makefile MBAPIDIR=source
	$(HIDE) mkdir -p $(RELEASE_DIR)
	$(HIDE) mkdir -p $(RELEASE_DIR)/bin
	$(HIDE) cp source/lib $(RELEASE_DIR) -rf
	$(HIDE) cp source/include $(RELEASE_DIR) -rf
	$(HIDE) cp README.md $(RELEASE_DIR) -rf
	$(HIDE) $(CC) $(CFLAGS) main.c $(SRCS) $(LIB_LINK) -o $(APP)
	@echo "EVOC ModBus compile SUCCESS"

clean :
	$(HIDE) make --no-print-directory -f source/Makefile MBAPIDIR=source clean
	$(HIDE) rm $(RELEASE_DIR) -rf

install :
	$(HIDE) cp $(LIBNAME).* /usr/lib
	$(HIDE) cp $(LIBNAME).* /usr/lib$(BITNAME)
	$(HIDE) cp $(APP) /usr/bin
	$(HIDE) cp $(APP) /usr/sbin
