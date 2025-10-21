
# zephyr_library()

zephyr_library_sources_ifdef (CONFIG_PMIC_SGM41511
   sgm41511_i2c.c
   sgm41511.c
)
zephyr_include_directories(./)