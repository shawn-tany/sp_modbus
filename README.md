## compile parameters
#
# V=99 : show compilation details,default to V=0
* make V=99
#
# debug : dispaly debugging information,such as cached data,default to no
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
#   --help,            Show EVOC ModBus demo options
#
## modbus tcp :
#
* sudo ./evoc_mb_demo --type tcp --ip 192.168.1.12 --port 502 --ethdev enp1s0 --max_data_size 1400
#
## modbus rtu :
#
* sudo ./evoc_mb_demo --type rtu --serial /dev/ttyUSB0 --baudrate 9600 --databit 8 --parity 0 --stopbit 1 --flowctl 0 --slaver 1 --max_data_size 1400
#
## 


