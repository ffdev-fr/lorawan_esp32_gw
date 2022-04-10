deps_config := \
	/home/FF/esp-idf/components/app_trace/Kconfig \
	/home/FF/esp-idf/components/aws_iot/Kconfig \
	/home/FF/esp-idf/components/bt/Kconfig \
	/home/FF/esp-idf/components/esp32/Kconfig \
	/home/FF/esp-idf/components/ethernet/Kconfig \
	/home/FF/esp-idf/components/fatfs/Kconfig \
	/home/FF/esp-idf/components/freertos/Kconfig \
	/home/FF/esp-idf/components/heap/Kconfig \
	/home/FF/esp-idf/components/libsodium/Kconfig \
	/home/FF/esp-idf/components/log/Kconfig \
	/home/FF/esp-idf/components/lwip/Kconfig \
	/home/FF/esp-idf/components/mbedtls/Kconfig \
	/home/FF/esp-idf/components/openssl/Kconfig \
	/home/FF/esp-idf/components/pthread/Kconfig \
	/home/FF/esp-idf/components/spi_flash/Kconfig \
	/home/FF/esp-idf/components/spiffs/Kconfig \
	/home/FF/esp-idf/components/tcpip_adapter/Kconfig \
	/home/FF/esp-idf/components/wear_levelling/Kconfig \
	/home/FF/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/FF/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/FF/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/FF/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
