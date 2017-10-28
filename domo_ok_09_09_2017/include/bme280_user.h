s8 BME280_SPI_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt);

s8 BME280_SPI_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt);
s8 SPI_routine(void);

void BME280_delay_msek(BME280_MDELAY_DATA_TYPE msek);

void ICACHE_FLASH_ATTR bme280_init_user(void);