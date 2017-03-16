#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <pthread.h>
#include <jni.h>

#include "sensor.h"
#include "flash.h"
#include "fpga.h"

int flash_ddwrite(const char *inputfile, const char *outfile, long size)
{
	int ret;
	FILE *fin = NULL, *fout = NULL;
	long write_size = size;
	unsigned char *pBuf = NULL;

	fin = fopen(inputfile, "rb");
	if (!fin) {
		LOGE("can't open input file:%s : %s",
					inputfile, strerror(errno));
		return -1;
	}

	fout = fopen(outfile, "wb");
	if (!fout) {
		LOGE("can't open output file:%s : %s",
					outfile, strerror(errno));
		goto error;
	}

	pBuf = (unsigned char *)malloc(UNIT_CYLINDERS);
	if (!pBuf)	{
		printf("Buff malloc error!\n");
		goto error;
	}

	LOGD("==============================================================================\n");
	LOGD("dd if=%s of=%s bs=1 size=%d\n", inputfile, outfile, (unsigned int)size);

	while (write_size > UNIT_CYLINDERS) {

		memset(pBuf, 0, UNIT_CYLINDERS);

		ret = fread(pBuf, 1, UNIT_CYLINDERS, fin);
		if (ret < UNIT_CYLINDERS) {
			LOGE("fread error:%s :%s ",
						inputfile, strerror(errno));
			goto error;
		}

		ret = fwrite(pBuf, 1, UNIT_CYLINDERS, fout);
		if (ret < UNIT_CYLINDERS) {
			LOGE("fwrite error:%s : %s", outfile, strerror(errno));
			goto error;
		}
		sync();
		
		write_size -= UNIT_CYLINDERS;
		LOGD("\rProgress : %d / %d", (unsigned int)(size-write_size), (unsigned int)size);
		sync();
	}
	
	if (write_size > 0 ) {
		memset(pBuf, 0, UNIT_CYLINDERS);
		
		ret = fread(pBuf, 1, write_size, fin);
		if (ret < write_size) {
			LOGE("fread error:%s :%s ",
						inputfile, strerror(errno));
			goto error;
		}

		ret = fwrite(pBuf, 1, write_size, fout);
		if (ret < write_size) {
			printf("fwrite error:%s : %s", outfile, strerror(errno));
			goto error;
		}
		sync();
		
		LOGD("\rProgress : %d / %d", (unsigned int)(size), (unsigned int)size);
	}

	LOGD("\nfpga_ddwrite complete\n");

	LOGD("\n");
	free(pBuf);
	if (fout)
		fclose(fout);
	fclose(fin);
	sync();

	return 0;

error:	
	if (pBuf)
		free(pBuf);
	if (fout)
		fclose(fout);
	if (fin)
		fclose(fin);

	return -1;

}

int fpga_UpgradeFirmware(char *pFile)
{
	int ret;
	FILE *fp = NULL;
	unsigned int image_size = 0;

	fp = fopen(pFile, "rb");
	if (!fp) {
		LOGE("fpga file open fail = %s\n", pFile);
		return -1;
	}
	
	fseek(fp, 0, SEEK_END);
	image_size = ftell(fp);
	fclose(fp);

	ret = flash_ddwrite(pFile, "/dev/mtd/mtd0", image_size);
	if (ret < 0) {
		LOGE("fpga write fail\n");
		return -1;
	}

	return 0;
}

void* flashUpdate(void* args)
{
	int ret;
	mtd_info_t mtd_info;           // the MTD structure
	erase_info_t ei;               // the erase block structure
	int i;
	char *filename = (char *)args;

	LOGD("flashUpdate %s", filename);

	ret = fpga_Setcmd(FPGA_CMD_ADC0_BYPASS);
	if (ret < 0) {
		LOGE("fpga_setcmd fail\n");
		return NULL;
	}

	ret = fpga_GpioSetValue(PROG_B, 0);
	if (ret < 0) {
		LOGE("fpga_GpioValue fail\n");
		return NULL;
	}

	int fd = open("/dev/mtd/mtd0", O_RDWR); // open the mtd device for reading and 
	if (fd < 0) {
		LOGE("/dev/mtd/mtd0 open fail %d\n", fd);
		goto exit;
	}

	ioctl(fd, MEMGETINFO, &mtd_info);

	LOGD("MTD Type: %x\nMTD total size: %x bytes\nMTD erase size: %x bytes\n",
				mtd_info.type, mtd_info.size, mtd_info.erasesize);

	ei.length = mtd_info.erasesize;
	for(ei.start = 0; ei.start < mtd_info.size; ei.start += ei.length) {
		ioctl(fd, MEMUNLOCK, &ei);
		LOGD("Eraseing Block %#x\n", ei.start);
		ioctl(fd, MEMERASE, &ei);
	}    

	close(fd);

	ret = fpga_UpgradeFirmware(filename);
	if (ret < 0) {
		LOGE("fpga_UpgradeFirmware fail\n");
		return NULL;
	}

	usleep(2000000);

	ret = fpga_GpioSetValue(PROG_B, 1);
	if (ret < 0) {
		LOGE("fpga_GpioSetValue fail\n");
		return NULL;
	}

	LOGI("flashUpdate Success");

exit:
	pthread_exit(NULL);
	return NULL;
}

void* flashRead(void* args)
{
	int ret;
	mtd_info_t mtd_info;           // the MTD structure
	erase_info_t ei;               // the erase block structure
	int i;
	char *filename = (char *)args;
	unsigned char read_buf[1024] = {0};                // empty array for reading
	FILE *fout = NULL;
	unsigned int write_size = 0;

	fout = fopen(filename, "wb");
	if (!fout) {
		LOGE("can't open output file:%s",
					filename);
		return NULL;
	}

	LOGD("flashRead %s", filename);

	ret = fpga_Setcmd(FPGA_CMD_ADC0_BYPASS);
	if (ret < 0) {
		LOGE("fpga_setcmd fail\n");
		return NULL;
	}

	ret = fpga_GpioSetValue(PROG_B, 0);
	if (ret < 0) {
		LOGE("fpga_GpioValue fail\n");
		return NULL;
	}

	int fd = open("/dev/mtd/mtd0", O_RDWR); // open the mtd device for reading and 
	if (fd < 0) {
		LOGE("/dev/mtd/mtd0 open fail %d\n", fd);
		return NULL;
	}

	ioctl(fd, MEMGETINFO, &mtd_info);

	LOGD("MTD Type: %x\nMTD total size: %x bytes\nMTD erase size: %x bytes\n",
				mtd_info.type, mtd_info.size, mtd_info.erasesize);

	
    fseek(fout, 0, SEEK_SET);               // go to the first block

	for (;;) {
	    ret = read(fd, read_buf, sizeof(read_buf));
		LOGD("read = %d size %d\n", ret, sizeof(read_buf));

		if (mtd_info.size >= write_size + ret) {
			ret = fwrite(read_buf, 1, sizeof(read_buf), fout);
			write_size += ret;
			LOGW("fwrite = %d size %d\n", ret, sizeof(read_buf));
		} else {
			ret = fwrite(read_buf, 1, mtd_info.size - write_size, fout);
			break;
		}

		if (mtd_info.size == write_size)
			break;
	}
	LOGD("write_size = %d \n", write_size);

	close(fd);
	fclose(fout);

	sync();

	ret = fpga_GpioSetValue(PROG_B, 1);
	if (ret < 0) {
		LOGE("fpga_GpioSetValue fail\n");
		return NULL;
	}

	LOGI("flashRead Success");

exit:
	pthread_exit(NULL);
	return NULL;
}

