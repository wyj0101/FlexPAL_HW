#!/bin/bash

cp ./001-add-hal-gpio.patch ../../../zephyr/
cp ./001-modify-mpu6050-driver.patch ../../../zephyr/
cd ../../../zephyr/
git apply 001-add-hal-gpio.patch
git apply 001-modify-mpu6050-driver.patch