/*
 * Copyright (c) 2021 Marc Reilly, Creative Product Design
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/spi.h>


#define SPIBB_NODE	DT_NODELABEL(spibb0)


/* CMD define */
#define SPI_READ  0x80


/* register define */
#define WHO_AM_I			0X0F
#define CTRL3_C				0X12
#define CTRL1_XL			0X10
#define CTRL8_XL			0x17
#define CTRL10_C			0x19 
#define INTERNAL_FREQ_FINE 	0x63
#define STATUS_REG			0x1E
#define FIFO_CTRL3 			0x09
#define FIFO_CTRL4			0x0A 
#define FIFO_DATA_OUT_TAG 	0X78
#define OUTX_L_A			0x28
#define OUTX_H_A			0x29
#define OUTY_L_A			0x2A
#define OUTY_H_A			0x2B
#define OUTZ_L_A			0x2C
#define OUTZ_H_A			0x2D
#define FIFO_STATUS2		0X3B
#define FIFO_DATA_OUT_X_L	0X79
#define FIFO_DATA_OUT_X_H	0X7A
#define FIFO_DATA_OUT_Y_L	0X7B
#define FIFO_DATA_OUT_Y_H	0X7C
#define FIFO_DATA_OUT_Z_L	0X7D
#define FIFO_DATA_OUT_Z_H	0X7E


/* static int16_t data_raw_acceleration[3];
static float acceleration_mg[3]; */

#define POINT_NUMBER	512

uint8_t raw_data[3072];

int CURRENT_POINT;

/* spi config */
struct spi_cs_control cs_ctrl = (struct spi_cs_control){
	.gpio = GPIO_DT_SPEC_GET(SPIBB_NODE, cs_gpios),
	.delay = 0u,
};

struct spi_config config = {
	.frequency = 125000,
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_MODE_CPOL | SPI_MODE_CPHA,
	.slave = 0,
	.cs = &cs_ctrl,
};

const struct device *dev = DEVICE_DT_GET(SPIBB_NODE);

/*spi read*/
uint8_t test_read(const struct device *dev,uint8_t addr)
{

	enum { datacount = 2 };
	uint8_t buff[datacount] = { SPI_READ|addr, 0x00};
	uint8_t rxdata[datacount];
	struct spi_buf tx_buf[1] = {
		{.buf = buff, .len = 2},
	};
	struct spi_buf rx_buf[1] = {
		{.buf = rxdata, .len = 2},
	};

	struct spi_buf_set tx_set = { .buffers = tx_buf, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = rx_buf, .count = 1 };

	int ret = spi_transceive(dev, &config, &tx_set, &rx_set);
	if (ret < 0) {
		printk("SPI transceive error: %d\n", ret);
	}
/* 	printf(" tx (i)  : %02x \n",
	       buff[0]);
	printf(" rx (i)  : %02x \n",
	       rxdata[1]); */

	return rxdata[1];
	
}


/* 6_byte SPI READ */
void test_read_6(const struct device *dev,uint8_t addr)
{
	enum { datacount = 7 };
	uint8_t buff[datacount] = { SPI_READ|addr, 0x00,0x00,0x00,0x00,0x00,0x00};
	uint8_t rxdata[datacount];
	struct spi_buf tx_buf[1] = {
		{.buf = buff, .len = 7},
	};
	struct spi_buf rx_buf[1] = {
		{.buf = rxdata, .len = 7},
	};

	struct spi_buf_set tx_set = { .buffers = tx_buf, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = rx_buf, .count = 1 };

	int ret = spi_transceive(dev, &config, &tx_set, &rx_set);
	if (ret < 0) {
		printk("SPI transceive error: %d\n", ret);
	}
	/* printf(" tx (i)  : %02x \n",
	       buff[0]);
	printf(" rx (i)  : %02x \n",
	       rxdata[1]);
 */
	for (int i = 1; i < 7; i++)
	{
		raw_data[CURRENT_POINT*6+i-1] = rxdata[i];
		//printf("%02x ", rxdata[i]);
	} 
	//printf("\n");
}


/* spi write */
void test_write(const struct device *dev,uint8_t addr,uint8_t data )
{

	enum { datacount = 2 };
	uint8_t buff[datacount] = { addr, data};
	uint8_t rxdata[datacount];

	struct spi_buf tx_buf[1] = {
		{.buf = buff, .len = 2},
	};
	struct spi_buf rx_buf[1] = {
		{.buf = rxdata, .len = 2},
	};

	struct spi_buf_set tx_set = { .buffers = tx_buf, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = rx_buf, .count = 1 };

	int ret = spi_transceive(dev, &config, &tx_set, &rx_set);
	if (ret < 0) {
		printk("SPI transceive error: %d", ret);
	}

}

void sensor_config()
{
	/* get id */
	
	printf("the sennsor ID is :%02x\n",test_read(dev,WHO_AM_I));

	printf("the controller status is : %2x\n",test_read(dev,CTRL3_C));

/* software reset set  */

	test_write(dev,CTRL3_C,0X01);

/* Enable Block Data Update  */
	test_write(dev,CTRL3_C,0X44);

/* Enable accelerater  */
/* Set Output Data Rate */
/* Set full scale  16g*/
	test_write(dev,CTRL1_XL,0XA6);

/* Accelerometer low pass filter path */
	test_write(dev,CTRL8_XL,0X80);

/* TIMESTAMP_EN */
	test_write(dev,CTRL10_C,0X20);

	k_sleep(K_MSEC(50));

}

void data_read_useFIFO(){

	/* BDR 26667HZ */
	test_write(dev,FIFO_CTRL3,0X0A);
	/* FIFO mode FIFO 满了后停止收集 */
	//test_write(dev,FIFO_CTRL4,0X01);
	/* 复位fifo */
	test_write(dev,FIFO_CTRL4,0X00);
	test_write(dev,FIFO_CTRL4,0X01);
	k_sleep(K_MSEC(10));
 	uint8_t FIFO_status;
	FIFO_status = test_read(dev,FIFO_STATUS2);
	if((FIFO_status & 0X20)== 0X20){
		printf("FIFO-FULL\n");
		for (CURRENT_POINT = 0; CURRENT_POINT < POINT_NUMBER; CURRENT_POINT++){
			test_read_6(dev,FIFO_DATA_OUT_X_L);
		}
	}else{
		printf("FIFO NOT FULL%2X\n",FIFO_status);
	} 

/* 	for (int i = 0; i < 512; i++){
		test_read_6(dev,FIFO_DATA_OUT_X_L);
	}	
 */

}

void status_read()
{
	uint8_t status = test_read(dev,STATUS_REG);
	if((status & 0x01)==0x01){
		printf("sensor ready!\n");
	}else{
	printf("sensor  NOT  ready!\n ");
	}
}

/* convert raw-data into engineering units */
float iis3dwb_from_fs16g_to_mg(int16_t lsb)
{
  return ((float)lsb * 0.488f);
}


void spi_data_get(void)
{

	if (!device_is_ready(dev)) {
		printk("%s: device not ready.\n", dev->name);
		return;
	}

	sensor_config();
	status_read();
/* 	while(1){
		data_read_useFIFO();
		//k_sleep(K_MSEC(3000));
	} */

/* 	while(1){
		data_read_useFIFO();
		printf("the raw data is\n");
		for (int i = 0; i < 3072; i++){

			printf("%02x ",raw_data[i]);
		}
		printf("\n");
		//k_sleep(K_MSEC(3000));
	} */

	data_read_useFIFO();
	/* printf("the raw data is\n");
	for (int i = 0; i < 3072; i++){

		printf("%02x ",raw_data[i]);
	}
	printf("\n"); */
	
/* 	while (1) {
		test_read_6(dev,OUTX_L_A);
		//k_sleep(K_MSEC(100));
	} */
} 
