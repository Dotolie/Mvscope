#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <jni.h>

#include "sensor.h"
#include "fpga.h"
#include "flash.h"

#define FILE_SAVE

static int gFpgaDev = -1;
static int fpgathread_Exit = 0;
int gFreq = 0;
static int gMode = SINGLE_ADC;//DIFF_ADC;//SINGLE_ADC;	//0 = single, 1 = differential

#ifdef FILE_SAVE
FILE *g_fp = NULL;
#endif

int fpga_GetFreqValue(int freq)
{
	if (freq == 1)
		return FPGA_1KHZ_SAMPLE;
	else if (freq == 2)
		return FPGA_2KHZ_SAMPLE;
	else if (freq == 4)
		return FPGA_4KHZ_SAMPLE;
	else if (freq == 5)
		return FPGA_5KHZ_SAMPLE;
#if 1//Not Support
	else if (freq == 8)
		return FPGA_8KHZ_SAMPLE;
	else if (freq == 10)
		return FPGA_10KHZ_SAMPLE;
	else if (freq == 16)
		return FPGA_16KHZ_SAMPLE;
	else if (freq == 20)
		return FPGA_20KHZ_SAMPLE;
	else if (freq == 25)
		return FPGA_25KHZ_SAMPLE;
	else if (freq == 50)
		return FPGA_50KHZ_SAMPLE;
#endif
	else
		return -1;
}

int fpga_GetSampleCnt(int freq)
{
	if (freq == 1)
		return 1;
	else if (freq == 2)
		return 2;
	else if (freq == 4)
		return 4;
	else if (freq == 5)
		return 5;
#if 1//Not Support
	else if (freq == 8)
		return 8;
	else if (freq == 10)
		return 10;
	else if (freq == 16)
		return 16;
	else if (freq == 20)
		return 20;
	else if (freq == 25)
		return 25;
	else if (freq == 50)
		return 50;
#endif
	else
		return -1;
}

int fpga_GpioSetDirection(int gpio, int dir)
{
	int fd;
	char buf[255];

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
	
	fd = open(buf, O_WRONLY);
	if (fd < 0)
		return -1;

	if (dir == DIR_OUTPUT)
		write(fd, "out", 3);
	else if (dir == DIR_INPUT)
		write(fd, "in", 2); 
	
	close(fd);

	return 0;
}

int fpga_GpioGetValue(int gpio)
{
	int fd, fd2, val;
	char value;
	char buf[255] = {0}; 
	
	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
	
	fd2 = open(buf, O_WRONLY);
	if (fd2 < 0)
		return -1;

	write(fd2, "in", 2); 
	
	close(fd2);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	
	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return -1;

	read(fd, &value, 1);
	
	if(value == '0') { 
		val = 0;
	} else if(value == '1'){
		val = 1;
	} else
		return -1;
	
	close(fd);

	return val;
}

int fpga_GpioSetValue(int gpio, int value)
{
	int fd, fd2, fd3;
	char buf[255]; 

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd < 0) {
		LOGE("fail export %d", gpio);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", gpio); 

	write(fd, buf, strlen(buf));

	close(fd);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);

	fd2 = open(buf, O_WRONLY);
	if (fd2 < 0) {
		LOGE("fail direction %d", gpio);
		return -1;
	}

	write(fd2, "out", 3);

	close(fd2);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);

	fd3 = open(buf, O_WRONLY);
	if (fd3 < 0) {
		LOGE("fail value %d", gpio);
		return -1;
	}

	if (value)
		write(fd3, "1", 1);
	else
		write(fd3, "0", 1); 

	close(fd3);

	return 0;
}

int fpga_Setcmd(unsigned char cmd)
{
	int ret;
	ret = fpga_GpioSetValue(AD_CMD0, cmd & 0x01);
	if (ret)
		return -1;
	ret = fpga_GpioSetValue(AD_CMD1, cmd & 0x02);
	if (ret)
		return -1;
	ret = fpga_GpioSetValue(AD_CMD2, cmd & 0x04);
	if (ret)
		return -1;
	ret = fpga_GpioSetValue(AD_CMD3, cmd & 0x08);
	if (ret)
		return -1;
	return 0;
}

int fpga_hardwareEnable (int on)
{
	int ret;

	//Enable : 1, Disable : 0
	ret = fpga_GpioSetValue(PROG_B, on);
	if (ret < 0) {
		LOGE("fpga_GpioSetValue fail\n");
		return -1;
	}

	sleep(2);
	return 0;
}

int fpga_dev_open (void)
{
	int err;
	
	if (gFpgaDev < 0)
	{
		LOGI("--fpga_dev_open\n");
		if ((gFpgaDev = open("/dev/fpga_spi", O_RDWR)) < 0) {
			LOGE("gFpgaDev_open fail\n");
			return -1;
		}
	}
	return 0;
}

void fpga_dev_close (void)
{
//	if (gFpgaDev >= 0)
	{
		LOGI("--fpga_dev_close\n");
		close (gFpgaDev);
		gFpgaDev = -1;
	}
}

int fpga_SingleInit(int freq, int high_resolution)
{
	int i, j, ret;
	struct spi_info sit = {0};
	unsigned char wData[8] = {0};
	unsigned char rData[8] = {0};

	LOGD("fpga_SingleInit\n");

	ret = fpga_dev_open();
	if (ret < 0) {
		LOGE("fpga dev open fail");
		return -1;
	}

	//fpga stop
	ret = fpga_Setcmd(FPGA_CMD_ADC0_BYPASS);
	if (ret < 0)
		return -1;

	//max1300 bypass init
	sit.channel = 2;
	sit.inbuff = wData;
	sit.outbuff = rData;
	sit.size = 1;

	for (i = 0; i < 1; i++) {
		fpga_Setcmd(FPGA_CMD_ADC0_BYPASS + i);

		for(j = 0; j < 8; j++) {
			//mode & channel configuration
			wData[0] = (j << 4) | ADC_MAX_RANGE | ADC_START | (ADC_SINGLE_MODE << 3);
			
			if (gFpgaDev >= 0) {
				ret = ioctl(gFpgaDev, FPGASPI_REGWRITE, &sit);
				if (ret) {
					fpga_dev_close();
					LOGE("FPGASPI_REGWRITE error\n");
					return -1;
				}
			}
		}
	}


	//fpga conversion 8 bytes
	for (i = 0; i < 8; i++)
		wData[i] = 0x80 | (i << 4);
	
	sit.size = 8;
	fpga_Setcmd(FPGA_CMD_ADC0);
	if (gFpgaDev >= 0) {
		ret = ioctl(gFpgaDev, FPGASPI_REGWRITE, &sit);
		if (ret) {
			LOGE("FPGASPI_REGWRITE 111 error\n");
			return -1;
		}
	}

	//fpga sample 2byte
	wData[0] = 0;
	wData[1] = fpga_GetFreqValue(freq);
	if (high_resolution)
		wData[1] |= FPGA_HIGH_RESOLUTION;
	sit.size = 2;
	
	for (i = 0; i < 2; i++) {
		fpga_Setcmd(FPGA_CMD_ADC0_SAMPLE + i);
		if (gFpgaDev >= 0) {
			ret = ioctl(gFpgaDev, FPGASPI_REGWRITE, &sit);
			if (ret) {
				LOGE("FPGASPI_REGWRITE 222 error\n");
				return -1;
			}
		}
	}

	ret = ioctl(gFpgaDev, FPGASPI_SET_FREQ, &freq);
	if (ret) {
		LOGE("FPGASPI_SET_FREQ error\n");
		return -1;
	}

	fpga_Setcmd(FPGA_CMD_ADC_START);
	usleep(1000000);
	LOGD("fpga_SingleInit SUCCESS\n");
	return 0;
}

int fpga_DifferentialInit(int freq, int high_resolution)
{
	int i, j, ret;
	struct spi_info sit = {0};
	unsigned char wData[8] = {0};
	unsigned char rData[8] = {0};

	LOGD("fpga_diffinit\n");
	ret = fpga_dev_open();
	if (ret < 0)
		return -1;

	//fpga stop
	fpga_Setcmd(FPGA_CMD_ADC0_BYPASS);
	
	//max1300 bypass init
	sit.channel = 2;
	sit.inbuff = wData;
	sit.outbuff = rData;
	sit.size = 1;

	for (i = 0; i < 1; i++) {
		fpga_Setcmd(FPGA_CMD_ADC0_BYPASS + i);

		for (j = 0; j < 4; j++) {
			//mode & channel configuration
			wData[0] = ((j * 2) << 4) | ADC_MAX_RANGE | ADC_START | (ADC_DIFF_MODE << 3);
			
			if (gFpgaDev >= 0) {
				ret = ioctl(gFpgaDev, FPGASPI_REGWRITE, &sit);
				if (ret) {
					LOGE("SPI_WRITE error\n");
					return -1;
				}
			}
		}
	}

	//fpga conversion 8byte
	for (i = 0; i < 4; i++)
		wData[i] = ADC_START | ((i * 2) << 4);
	sit.size = 4;
	fpga_Setcmd(FPGA_CMD_ADC0);
	if (gFpgaDev >= 0) {
		ret = ioctl(gFpgaDev, FPGASPI_REGWRITE, &sit);
		if (ret) {
			LOGE("SPI_WRITE error\n");
			return -1;
		}
	}

	//fpga sample 2byte
	wData[0] = 0;
	wData[1] = fpga_GetFreqValue(freq);
	if (high_resolution)
		wData[1] |= FPGA_HIGH_RESOLUTION;
	sit.size = 2;

	for (i = 0; i < 2; i++) {
		fpga_Setcmd(FPGA_CMD_ADC0_SAMPLE + i);
		if (gFpgaDev >= 0) {
			ret = ioctl(gFpgaDev, FPGASPI_REGWRITE, &sit);
			if (ret) {
				LOGE("SPI_WRITE error\n");
				return -1;
			}
		}
	}

	ret = ioctl(gFpgaDev, FPGASPI_SET_FREQ, &freq);
	if (ret) {
		printf("FPGASPI_SET_FREQ error\n");
		return -1;
	}

	fpga_Setcmd(FPGA_CMD_ADC_START);
	usleep(1000000);
	LOGD("fpga_diffinit SUCCESS\n");

	return 0;
}

unsigned char fpga_data[20000 * 5] = {0};

int fpga_GetData(void)
{
	struct spi_info sit;
	int i = 0, j, ret, cnt = 0;
	static int first = 1;
	int buf_offset = 20000;
	int buf_idx = 0;
	int a, b, c, d;
	sit.channel = 2;
	sit.inbuff =  0;
	sit.outbuff = fpga_data;
	
//	memset(fpga_data, 0, 20000);

	//20ms get data
	if (gFpgaDev > 0) {
		if ((ret = ioctl(gFpgaDev, FPGASPI_GET_DATA, &sit)) < 0) {
			LOGE("spi_get_data fail %d\n", ret);
			return -1;
		}
	}

	if (first) {
		first = 0;
		return 0;
	}
#ifdef _FILE_SAVE
	if (g_fp) {
		cnt = gFreq * 20;
		
		for (i = 0; i < cnt; i++) {
			fprintf(g_fp, "%d max1300 =  ", i);
			if (gMode == SINGLE_ADC) {
				//max1300 16bit 8channel single adc data
				if (gFreq == 1 || gFreq == 2 || gFreq == 4 || gFreq == 5 || gFreq == 8 || gFreq == 10) {
					//max1300 16bit single 8 channel
					fprintf(g_fp, "	%d	%d	%d	%d	%d	%d	%d	%d		", 
						fpga_data[buf_idx + 0] << 8 | fpga_data[buf_idx + 1], fpga_data[buf_idx + 2] << 8 | fpga_data[buf_idx + 3],
						fpga_data[buf_idx + 4] << 8 | fpga_data[buf_idx + 5], fpga_data[buf_idx + 6] << 8 | fpga_data[buf_idx + 7],
						fpga_data[buf_idx + 8] << 8 | fpga_data[buf_idx + 9], fpga_data[buf_idx + 10] << 8 | fpga_data[buf_idx + 11],
						fpga_data[buf_idx + 12] << 8 | fpga_data[buf_idx + 13], fpga_data[buf_idx + 14] << 8 | fpga_data[buf_idx + 15]);
					buf_idx += 16;
				} else if (gFreq == 16 || gFreq == 20) {
					//max1300 16bit single 4 channel
					fprintf(g_fp, "	%d	%d	%d	%d		", 
						fpga_data[buf_idx + 0] << 8 | fpga_data[buf_idx + 1], fpga_data[buf_idx + 2] << 8 | fpga_data[buf_idx + 3],
						fpga_data[buf_idx + 4] << 8 | fpga_data[buf_idx + 5], fpga_data[buf_idx + 6] << 8 | fpga_data[buf_idx + 7]);
					buf_idx += 8;
				} else if (gFreq == 25) {
					//max1300 16bit single 2 channel
					fprintf(g_fp, "	%d	%d		", 
						fpga_data[buf_idx + 0] << 8 | fpga_data[buf_idx + 1], fpga_data[buf_idx + 2] << 8 | fpga_data[buf_idx + 3]);
					buf_idx += 4;
				} else if (gFreq == 50) {
					//max1300 16bit single 1 channel
					fprintf(g_fp, "	%d		", fpga_data[buf_idx + 0] << 8 | fpga_data[buf_idx + 1]);
					buf_idx += 2;
				}
			} else if (gMode == DIFF_ADC) {
				//max1300 16bit differential adc data
				if (gFreq == 1 || gFreq == 2 || gFreq == 4 || gFreq == 5 || gFreq == 8 || gFreq == 10) {
					//max1300 16bit differential 8 channel
					fprintf(g_fp, "	%d	%d	%d	%d		", 
						fpga_data[buf_idx + 0] << 8 | fpga_data[buf_idx + 1], fpga_data[buf_idx + 2] << 8 | fpga_data[buf_idx + 3],
						fpga_data[buf_idx + 4] << 8 | fpga_data[buf_idx + 5], fpga_data[buf_idx + 6] << 8 | fpga_data[buf_idx + 7]);
					buf_idx += 16;
				} else if (gFreq == 16 || gFreq == 20) {
					//max1300 16bit differential 4 channel
					fprintf(g_fp, "	%d	%d	%d	%d		", 
						fpga_data[buf_idx + 0] << 8 | fpga_data[buf_idx + 1], fpga_data[buf_idx + 2] << 8 | fpga_data[buf_idx + 3],
						fpga_data[buf_idx + 4] << 8 | fpga_data[buf_idx + 5], fpga_data[buf_idx + 6] << 8 | fpga_data[buf_idx + 7]);
					buf_idx += 8;
				} else if (gFreq == 25) {
					//max1300 16bit differential 2 channel
					fprintf(g_fp, "	%d	%d		", 
						fpga_data[buf_idx + 0] << 8 | fpga_data[buf_idx + 1], fpga_data[buf_idx + 2] << 8 | fpga_data[buf_idx + 3]);
					buf_idx += 4;
				} else if (gFreq == 50) {
					//max1300 16bit differential 1 channel
					fprintf(g_fp, "	%d		", fpga_data[buf_idx + 0] << 8 | fpga_data[buf_idx + 1]);
					buf_idx += 2;
				}
			}

			//ads1271 strain gage 32bit data
			a = ((fpga_data[20000 + (i * 4) + 1] << 16) | (fpga_data[20000 + (i * 4) + 2] << 8) | fpga_data[20000 + (i * 4) + 3]);
			if (a > 0x7FFFFF)
				a = (0xFFFFFF - a) * (-1); 
			b = ((fpga_data[40000 + (i * 4) + 1] << 16) | (fpga_data[40000 + (i * 4) + 2] << 8) | fpga_data[40000 + (i * 4) + 3]);
			if (b > 0x7FFFFF)
				b = (0xFFFFFF - b) * (-1); 
			c = ((fpga_data[60000 + (i * 4) + 1] << 16) | (fpga_data[60000 + (i * 4) + 2] << 8) | fpga_data[60000 + (i * 4) + 3]);
			if (c > 0x7FFFFF)
				c = (0xFFFFFF - c) * (-1); 
			d = ((fpga_data[80000 + (i * 4) + 1] << 16) | (fpga_data[80000 + (i * 4) + 2] << 8) | fpga_data[80000 + (i * 4) + 3]);
			if (d > 0x7FFFFF)
				d = (0xFFFFFF - d) * (-1);
			ret = fprintf(g_fp, "ads1271 =	%d	%d	%d	%d\r\n", a, b, c, d);
		}
		fprintf(g_fp, "=====================================================================================\r\n"); 
	}
#endif
	return 0;
}


extern JNIEnv*		g_env;
extern JavaVM*		jvm;
extern jobject 		g_jobject;
extern jclass		g_clazz;
extern jobject		g_jobjBuffer;
extern jmethodID	g_mid;

void* fpga_thread(void* arg)
{
	int ret;
	LOGD("fpga_thread start freq %d", gFreq);
	
#ifdef FILE_SAVE
	g_fp = fopen("/mnt/sdcard/sem.txt", "wt");
	if (!g_fp) {
		LOGE("fpga_thread file open fail");
		return NULL;
	}
#endif

	if (gMode == SINGLE_ADC)
		ret = fpga_SingleInit(gFreq, 0);
	else if (gMode == DIFF_ADC)
		ret = fpga_DifferentialInit(gFreq, 0);
	if (ret < 0) {
		LOGE("fpga adc init fail");
		return NULL;
	}
	
	usleep(10000);
	
	while (!fpgathread_Exit) {		
		fpga_GetData();

		usleep(10);
	}

	//fpga stop
	fpga_Setcmd(FPGA_CMD_ADC0_BYPASS);

	fpga_dev_close();

#ifdef FILE_SAVE
	if (g_fp)
		fclose(g_fp);
	g_fp = NULL;
#endif
	LOGD("fpga_thread exit");
	fpga_dev_close();
	pthread_exit(NULL);
	return NULL;
}

static pthread_t fpga_pthread = 0;

int fpga_open(int freq)
{
	int ret;


#ifdef _FILE_SAVE
	g_fp = fopen("/mnt/sdcard/sem.txt", "wt");
	if (!g_fp) {
		LOGE("fpga_thread file open fail");
		return;
	}
#endif

	gFreq = freq;

	if (gMode == SINGLE_ADC)
		ret = fpga_SingleInit(gFreq, 0);
	else if (gMode == DIFF_ADC)
		ret = fpga_DifferentialInit(gFreq, 0);
	if (ret < 0) {
		LOGE("fpga adc init fail");
		return -1;
	}

	LOGE("---fstart,ret=%d, gFreq=%d, gMode=%d", ret, gFreq, gMode);


	usleep(1000);

	return 0;
}

void fpga_abort()
{
	//fpga stop
	fpga_Setcmd(FPGA_CMD_ADC0_BYPASS);

	fpga_dev_close();

#ifdef _FILE_SAVE
	if (g_fp)
		fclose(g_fp);
	g_fp = NULL;
#endif

	LOGE("---fstop");
}

int fpga_datas(void)
{
	return fpga_GetData();
}


int fpga_taskStart(int freq)
{
	int ret;

	if (fpgathread_Exit)
		return -1;
	
	gFreq = freq;
	fpgathread_Exit = 0;
	ret = pthread_create(&fpga_pthread, NULL, fpga_thread, NULL);
	if (ret) {
		LOGE("pthread create failed");
		return -1;
	}

	return 0;
}

int fpga_taskStop(void)
{
	int ret;
	
	LOGD("fpga_taskStop");
	fpgathread_Exit = 1;
	if (fpga_pthread)
		pthread_join(fpga_pthread, NULL);

	fpga_pthread = 0;
	fpga_dev_close();
	fpgathread_Exit = 0;

	return 0;
}

