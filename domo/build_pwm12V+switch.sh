export PATH=/opt/Espressif/crosstool-NG//builds/xtensa-lx106-elf/bin:$PATH
make clean all
make ARG=PWM_12V_AND_SWITCH_PCF8885
