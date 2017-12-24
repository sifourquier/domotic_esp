export PATH=/opt/Espressif/crosstool-NG//builds/xtensa-lx106-elf/bin:$PATH
make clean all
make ARG=PWM_TRIACK_0V_DETECT
