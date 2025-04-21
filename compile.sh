#!/bin/bash

if [ "$1" = "flash" ]; then
    st-flash --connect-under-reset write build/zephyr/zephyr.bin 0x8000000
elif [ "$1" = "all" ]; then
    west build -p always -b stm32_min_dev_blue  ./FlexPAL_HW
else
    west build -p auto -b stm32_min_dev_blue  ./FlexPAL_HW
fi

#source ../.venv/bin/activate

#west build -d zet6/ -t menuconfig
#west build -t menuconfig

