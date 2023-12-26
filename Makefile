## define
CC 		:= gcc
APP 	:= evoc_mb_demo
SRCS 	:= evoc_mb.c mb_common.c mb_tcp.c mb_rtu.c
CFLAGS 	:= -g
CFLAGS 	+= -I .
CFLAGS 	+= -lpthread
CFLAGS 	+= -Wall -Werror

debug   := no
stay    := no
V		:= 0

ifeq ($(debug),yes)
	CFLAGS 	+= -DMB_DEBUG
endif

ifeq ($(stay),yes)
	CFLAGS 	+= -DMB_STAY
endif

ALL :
	@echo "EVOC ModBus compiling"
ifeq ($(V),99)
## target
	$(CC) $(CFLAGS) main.c $(SRCS) -o $(APP)
else
	@$(CC) $(CFLAGS) main.c $(SRCS) -o $(APP)
endif
	@echo "EVOC ModBus compile SUCCESS"

clean :
	rm $(APP) -rf
