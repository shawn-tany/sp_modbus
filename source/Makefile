CC := gcc

RELEASE_DIR := ../release
LIB_DIR     := lib
INC_DIR     := include
BIN_DIR     := bin
CONF_FILE   := mb_io.conf

BITNAME     := $(shell getconf LONG_BIT)
LIBNAME     := $(LIB_DIR)/liblinux$(BITNAME)modbus
APP         := $(BIN_DIR)/sp_mb_demo
PACKAGE     := $(RELEASE_DIR)/sp_modbus.tar.gz
V           := 0

CFLAGS 	:= -g
CFLAGS 	+= -I $(INC_DIR)
CFLAGS 	+= -Wall -Werror
CFLAGS 	+= -lpthread 

static=no
ifeq ($(static),yes)
	LIB_LINK = $(LIBNAME).a
else
	LIB_LINK = -llinux$(BITNAME)modbus -L $(LIB_DIR)
	NOTE     = (echo "\nnote : you should execute make install")
endif

ifneq ($(V),99)
	HIDE=@
endif

.PHONY: install release

ALL : clean
	@echo "SP ModBus compiling"
	@## build libraries
	$(HIDE) make --no-print-directory -f src/Makefile MBAPIDIR=src
	$(HIDE) cp src/lib . -rf
	$(HIDE) cp src/include . -rf
	$(HIDE) cp rule/*.h ./include -rf
	@## build app
	$(HIDE) mkdir -p $(BIN_DIR)
	$(HIDE) $(CC) $(CFLAGS) main.c rule/mask_rule.c $(SRCS) $(LIB_LINK) -o $(APP)
	@echo "SP ModBus compile SUCCESS"
	$(HIDE) $(NOTE)

clean :
	$(HIDE) make --no-print-directory -f src/Makefile MBAPIDIR=src clean
	$(HIDE) rm $(BIN_DIR) -rf
	$(HIDE) rm $(LIB_DIR) -rf
	$(HIDE) rm $(INC_DIR) -rf

install :
	$(HIDE) cp $(LIBNAME).* /usr/lib
	$(HIDE) cp $(LIBNAME).* /usr/lib$(BITNAME)
	$(HIDE) cp $(APP) /usr/bin
	$(HIDE) cp $(APP) /usr/sbin
	$(HIDE) cp $(CONF_FILE) /usr/local/etc

release :
	$(HIDE) mkdir -p $(RELEASE_DIR)
	$(HIDE) cp README.md  $(RELEASE_DIR) -rf
	$(HIDE) cp install  $(RELEASE_DIR) -rf
	$(HIDE) cp $(CONF_FILE)  $(RELEASE_DIR) -rf
	$(HIDE) cp $(BIN_DIR) $(RELEASE_DIR) -rf
	$(HIDE) cp $(LIB_DIR) $(RELEASE_DIR) -rf
	$(HIDE) cp $(INC_DIR) $(RELEASE_DIR) -rf
	$(HIDE) tar -zcPf $(PACKAGE) README.md install $(BIN_DIR) $(LIB_DIR) $(INC_DIR)
