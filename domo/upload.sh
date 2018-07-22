export PATH=/opt/Espressif/crosstool-NG//builds/xtensa-lx106-elf/bin:$PATH

killall gtkterm
cd .output/eagle/debug/image
esptool -eo eagle.app.v6.out -bo eagle.app.v6.flash.bin -bs .text -bs .data -bs .rodata -bc -ec
xtensa-lx106-elf-objcopy --only-section .irom0.text -O binary eagle.app.v6.out eagle.app.v6.irom0text.bin
#cp eagle.app.v6.flash.bin ../../../../../bin/
#cp eagle.app.v6.irom0text.bin ../../../../../bin/ 

#cd ../../../../../bin/
#/opt/Espressif/esptool-py/esptool.py --port /dev/ttyUSB0 erase_flash
/opt/Espressif/esptool-py/esptool.py --port /dev/ttyUSB0 write_flash 0x40000 eagle.app.v6.irom0text.bin 0x00000 ../../../../../bin/eagle.flash.bin 0x7C000 ../../../../../bin/esp_init_data_default_v08.bin 0x7E000 ../../../../../bin/blank.bin
