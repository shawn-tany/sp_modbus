CC := gcc

MBAPIDIR := .

BITNAME := $(shell getconf LONG_BIT)

LIBNAME := liblinux$(BITNAME)modbus

SRCS := $(MBAPIDIR)/evoc_mb.c
SRCS += $(MBAPIDIR)/ModBus/mb_common.c
SRCS += $(MBAPIDIR)/ModBus/mb_tcp.c
SRCS += $(MBAPIDIR)/ModBus/mb_rtu.c

INCS := $(MBAPIDIR)/evoc_mb.h
INCS += $(MBAPIDIR)/ModBus/mb_common.h
INCS += $(MBAPIDIR)/ModBus/mb_tcp.h
INCS += $(MBAPIDIR)/ModBus/mb_rtu.h

OBJS := $(patsubst %.c,%.o,$(SRCS))

CFLAGS 	:= -g
CFLAGS 	+= -I .
CFLAGS 	+= -I $(MBAPIDIR)/ModBus
CFLAGS 	+= -lpthread
CFLAGS 	+= -Wall -Werror

LIB_DIR := $(MBAPIDIR)/lib
INC_DIR := $(MBAPIDIR)/include

debug   := no

ifeq ($(debug),yes)
	CFLAGS 	+= -DMB_DEBUG
endif

ifneq ($(V),99)
	HIDE=@
endif

ALL : $(OBJS)
	$(HIDE) mkdir -p $(LIB_DIR)
	$(HIDE) mkdir -p $(INC_DIR)
	$(HIDE) $(CC) -fPIC -shared $(OBJS) -o $(LIB_DIR)/$(LIBNAME).so 
	$(HIDE) ar rcs $(LIB_DIR)/$(LIBNAME).a $(OBJS)
	$(HIDE) cp $(INCS) $(INC_DIR)

%.o : %.c 
	$(HIDE) $(CC) $(CFLAGS) -c -fPIC -o $@ $<

clean :
	$(HIDE) rm -rf $(OBJS)
	$(HIDE) rm -rf $(LIB_DIR)
	$(HIDE) rm -rf $(INC_DIR)

