#ifndef __FPGA_H__
#define __FPGA_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	SPI_DEV1 = 0,
	SPI_DEV2 = 1,
	SPI_DEV3 = 2,
	SPI_DEV4 = 3,
	SPI_DEV5 = 4,
};

enum {
	FPGA_1KHZ_SAMPLE = 0,
	FPGA_2KHZ_SAMPLE,
	FPGA_4KHZ_SAMPLE,
 	FPGA_5KHZ_SAMPLE,
	FPGA_8KHZ_SAMPLE,
 	FPGA_10KHZ_SAMPLE,
 	FPGA_16KHZ_SAMPLE,
	FPGA_20KHZ_SAMPLE,
	FPGA_25KHZ_SAMPLE,
	FPGA_50KHZ_SAMPLE,
	FPGA_HIGH_RESOLUTION = 0x10,
};

enum {
	FPGA_CMD_ADC0 = 0x0,
 	FPGA_CMD_ADC1 = 0x1,
 	FPGA_CMD_ADC2 = 0x2,
	FPGA_CMD_ADC3 = 0x3,
 	FPGA_CMD_ADC4 = 0x4,
	FPGA_CMD_ADC0_BYPASS = 0x5,
	FPGA_CMD_ADC1_BYPASS = 0x6,
	FPGA_CMD_ADC2_BYPASS = 0x7,
	FPGA_CMD_ADC3_BYPASS = 0x8,
	FPGA_CMD_ADC4_BYPASS = 0x9,
	FPGA_CMD_ADC0_SAMPLE = 0xa,
	FPGA_CMD_ADC1_SAMPLE = 0xb,
	FPGA_CMD_ADC2_SAMPLE = 0xc,
	FPGA_CMD_ADC3_SAMPLE = 0xd,
	FPGA_CMD_ADC4_SAMPLE = 0xe,
	FPGA_CMD_ADC_START = 0xf,
};

#define DIR_INPUT	0
#define DIR_OUTPUT	1
#define SINGLE_ADC	0
#define DIFF_ADC	1

#define GPIO_NR(bank, nr)		(((bank) - 1) * 32 + (nr))

#define AD_CMD0				GPIO_NR(5, 14)		//DISP0_DAT20
#define AD_CMD1				GPIO_NR(5, 15)		//DISP0_DAT21
#define AD_CMD2				GPIO_NR(5, 16)		//DISP0_DAT22
#define AD_CMD3				GPIO_NR(5, 17)		//DISP0_DAT23

#define FPGA_INTB			GPIO_NR(4, 28)		//DISP0_DAT7

#define FPGA_DONE			GPIO_NR(4, 26)		//DISP0_DAT5
#define PROG_B				GPIO_NR(4, 25)		//DISP0_DAT4

#define ADC_MAX_RANGE		0x7
#define ADC_START			0x80
#define ADC_DIFF_MODE		0x1
#define ADC_SINGLE_MODE		0x0

#define FPGASPI_READ				_IO('F', 1)
#define FPGASPI_WRITE				_IO('F', 2)
#define FPGASPI_REGREAD				_IO('F', 3)
#define FPGASPI_REGWRITE			_IO('F', 4)
#define FPGASPI_GET_DATA			_IO('F', 5)
#define FPGASPI_SET_FREQ			_IO('F', 6)

int fpga_GpioSetDirection(int gpio, int dir);
int fpga_GpioGetValue(int gpio);
int fpga_GpioSetValue(int gpio, int value);
int fpga_Setcmd(unsigned char cmd);
int fpga_SingleInit(int freq, int high_resolution);
int fpga_taskStart(int freq);
int fpga_taskStop(void);
int fpga_open(int freq);
void fpga_abort(void);
int fpga_datas(void);


#ifdef __cplusplus
}
#endif

#endif
