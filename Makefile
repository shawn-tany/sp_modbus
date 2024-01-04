CC := gcc

RELEASE_DIR := release
BITNAME 	:= $(shell getconf LONG_BIT)
LIBNAME 	:= $(RELEASE_DIR)/lib/liblinux$(BITNAME)modbus
APP 		:= $(RELEASE_DIR)/bin/evoc_mb_demo
V			:= 0

CFLAGS 	:= -g
CFLAGS 	+= -I $(RELEASE_DIR)/include
CFLAGS 	+= -Wall -Werror

ALL :
	@echo "EVOC ModBus compiling"
	@make --no-print-directory -f source/Makefile MBAPIDIR=source
	@mkdir -p $(RELEASE_DIR)
	@mkdir -p $(RELEASE_DIR)/bin
	@cp source/lib $(RELEASE_DIR) -rf
	@cp source/include $(RELEASE_DIR) -rf
	@cp README.md $(RELEASE_DIR) -rf
	@$(CC) $(CFLAGS) main.c $(SRCS) $(LIBNAME).a -o $(APP)
	@echo "EVOC ModBus compile SUCCESS"

clean :
	@make --no-print-directory -f source/Makefile MBAPIDIR=source clean
	@rm $(RELEASE_DIR) -rf
