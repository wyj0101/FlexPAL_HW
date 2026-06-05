# FlexPAL Hardware

编译方法：
1、将compile.sh文件放在zephyr的同级目录，即PlexPAL_HW的前一级目录
2、执行 ./compile.sh all 即可
3、烧录连接stlink之后，执行 ./compile.sh flash 即可


# 加入usb并运行docker
sudo docker run -it --name zephyr-main-docker   -v $HOME/docker-zephyr/zephyr-main/zephyr-main:/workdir   --device=/dev/ttyUSB0   ghcr.io/zephyrproject-rtos/zephyr-build:main bash
sudo docker ps -a
sudo docker start -ai zephyr-main-docker

west  build -p always -b esp32c3_devkitm ./zephyr/samples/drivers/led/led_strip/

west  build -p always -b esp32c3_devkitm ./FlexPAL_HW/

west flash --esp-device /dev/ttyUSB0 && picocom -b 115200 /dev/ttyUSB0


west blobs fetch hal_espressif