## compile parameters
#
# delete intermediate files and compile
* make clean;make
#
# compile using static libraries,default to using shared libraries
* make static=yes
# 
# installing software packages
# !! this operation must be performed before running a program compiled from a shared libraries
* make install
#
# packaging software and development libraries
* make realease
#
# show compilation details,default to V=0
* make V=99
#
# dispaly debugging information,such as cached data,default to no
* make debug=yes
#
##

## execution parameters
#
#   --type,            Select ModBus protocol type [tcp]
#   --max_data_size,   Limit ModBus transform data cache size [1400]
#   --ip,              ModBus TCP server ip [192.168.1.12]
#   --port,            ModBus TCP server port [502]
#   --ethdev,          ModBus TCP ethernet device for transform [eth0]
#   --serial,          Select ModBus RTU serial [/dev/ttyUSB0]
#   --buadrate,        Set ModBus RTU baudrate [9600]
#   --databit,         Set ModBus RTU data bit [8]
#   --stopbit,         Set ModBus RTU stop bit [1]
#   --flowctl,         Set ModBus RTU flow control [0]
#   --parity,          Set ModBus RTU parity [0]
#   --slaver,          Set ModBus RTU slaver address [1]
#   --help,            Show SP ModBus demo options
#
## modbus tcp :
#
* sudo ./sp_mb_demo --type tcp --ip 192.168.1.12 --port 502 --ethdev enp1s0 --max_data_size 1400
#
## modbus rtu :
#
* sudo ./sp_mb_demo --type rtu --serial /dev/ttyUSB0 --baudrate 9600 --databit 8 --parity 0 --stopbit 1 --flowctl 0 --slaver 1 --max_data_size 1400
#
## 

##
#
# MENU : the menu interface will be displayed after the ModBus connection is successful
#
# please enter according to the prompt information
#
##
#
#   ***********************************************************
#   *  [ WORK MODE : UNSTAY ]                                 *
#   ***********************************************************
#   *                          MENU                           *
#   ***********************************************************
#   *  [     1]. Function code : 0x01 Read output IO coils    *
#   *  [     2]. FunCtion code : 0x02 Read output IO coils    *
#   *  [     3]. FunCtion code : 0x03 Read hold register      *
#   *  [     4]. FunCtion code : 0x04 Read input register     *
#   *  [     5]. FunCtion code : 0x05 Write a coil            *
#   *  [     6]. FunCtion code : 0x06 Write a register        *
#   *  [    15]. FunCtion code : 0x0F Write multiple coils    *
#   *  [    16]. FunCtion code : 0x10 Write multiple register *
#   *  [  stay]. Stay connect                                 *
#   *  [unstay]. Unstay connect                               *
#   *  [  exit]. Exit                                         *
#   ***********************************************************
##
