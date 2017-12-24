mosquitto_sub -h 192.168.0.120 -t /sensors/switch/# | hexdump -v -e '/1 "%02X"' -e '" _%_u\_\n"'
 
