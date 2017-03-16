//mvtech 2015. 02. 06
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <termio.h>

#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <jni.h>

#include "sensor.h"
#include "flash.h"
#include "fpga.h"
#include "socket.h"

static int gDev = -1;
static int thread_Exit = 0;
static unsigned int irq_cnt = 0;
static pthread_t sensor_pthread;
unsigned char adc_data[1280  * 2] = {0};
unsigned char capture_buffer[512  * 6] = {0};

JNIEnv*		g_env = NULL;
JavaVM*		jvm = NULL;
jclass		g_jNativesCls = NULL;  

jmethodID	g_jpost_eventId = NULL;

int spi_dev_open (void)
{
	int err;
	
	if (gDev < 0) {
		if ((gDev = open("/dev/spi", O_RDWR)) < 0) {
			LOGE("spi_open fail\n");
			return -1;
		}

		//if ((err=ioctl(gDev, IOCTL_SPI_INFORM_SIG, NULL)) < 0) {
		//	LOGE("SPI_INFORM_SIG error\n");
		//	return -1;
		//}
	}
	return 0;
}

void spi_dev_close (void)
{
	if (gDev >= 0) {
		close (gDev);
		gDev = -1;
	}
}

unsigned short spi_regread(int channel, unsigned char reg)
{
	struct spi_info sit;
	unsigned char rReg[2];
	unsigned char rData[2];
	int ret;
	memset(&sit, 0, sizeof(sit));
	memset(rReg, 0, sizeof(rReg));
	memset(rData, 0, sizeof(rData));
	
	rReg[0] = reg;

	sit.channel = channel;
	sit.inbuff = rReg;
	sit.outbuff = rData;
	sit.size = 2;

	if (gDev >= 0) {
		ret = ioctl(gDev, IOCTL_SPI_READ, &sit);
		if (ret) {
			LOGE("SPI_REGREAD error\n");
			return -1;
		}
	}

	//LOGD("reg(0x%02x) 0 0x%02x\n", reg, rData[0]);
	//LOGD("reg(0x%02x) 1 0x%02x\n", reg, rData[1]);

	return (rData[0]<<8) | rData[1];
}

int spi_regwrite(int channel, unsigned char reg, unsigned short data)
{
	int ret;
	struct spi_info sit;
	unsigned char wReg[4];

	memset(&sit, 0, sizeof(sit));
	memset(wReg, 0, sizeof(wReg));
	
	wReg[0] = reg;
	wReg[1] = (data>>8)&0xff;
	wReg[2] = data&0xff;

	sit.channel = channel;
	sit.inbuff = wReg;
	sit.outbuff = NULL;
	sit.size = 3;

	if (gDev >= 0) {
		ret = ioctl(gDev, IOCTL_SPI_WRITE, &sit);
		if (ret) {
			LOGE("SPI_WRITE error\n");
			return -1;
		}
	} else
		LOGE("SPI_WRITE error\n");

	return 0;
}

int spi_regbytewrite(int channel, unsigned char reg, unsigned char data)
{
	int ret;
	struct spi_info sit;
	unsigned char wReg[4];

	memset(&sit, 0, sizeof(sit));
	memset(wReg, 0, sizeof(wReg));
	
	wReg[0] = reg;
	wReg[1] = data&0xff;

	sit.channel = channel;
	sit.inbuff = wReg;
	sit.outbuff = NULL;
	sit.size = 2;

	if (gDev >= 0) {
		ret = ioctl(gDev, IOCTL_SPI_BYTE_WRITE, &sit);
		if (ret) {
			LOGE("SPI_WRITE error\n");
			return -1;
		}
	} else
		LOGE("SPI_WRITE error\n");

	return 0;
}

int spi_get_data(void)
{
	struct spi_info sit;
	int i, ret, cnt = 0;

	sit.channel = 0;
	sit.inbuff =  0;
	sit.outbuff = adc_data;

	if (gDev >= 0) {
		if ((ret = ioctl(gDev, IOCTL_SPI_GET_DATA, &sit)) < 0) {
			LOGE("spi_get_data fail %d\n", ret);
			return -1;
		}
	}
#if 0
	LOGD("spi size = %d", sit.size);
	for (i = 0; i < 10; i++) {
		
		cnt *= 5;
		LOGD("[0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]", 
				adc_data[cnt], adc_data[cnt + 1], adc_data[cnt + 2], adc_data[cnt + 3], adc_data[cnt + 4]);
	}
#endif
	return 0;
}

int spi_manual_time_capture(void)
{
	struct spi_info sit;
	int i, ret, cnt = 0;

	sit.channel = 0;
	sit.inbuff =  0;
	sit.outbuff = capture_buffer;

	if (gDev >= 0) {
		if ((ret = ioctl(gDev, IOCTL_SPI_MANUAL_CAPTURE, &sit)) < 0) {
			LOGE("IOCTL_SPI_MANUAL_CAPTURE fail %d\n", ret);
			return -1;
		}
	}
#if 1
	LOGD("spi size = %d", sit.size);
	cnt = 0;
	LOGD("%d [0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]", cnt,
			capture_buffer[cnt], capture_buffer[cnt + 1], 
			capture_buffer[cnt + 2], capture_buffer[cnt + 3], 
			capture_buffer[cnt + 4], capture_buffer[cnt + 5],
			capture_buffer[cnt + 6], capture_buffer[cnt + 7], 
			capture_buffer[cnt + 8], capture_buffer[cnt + 9], 
			capture_buffer[cnt + 10], capture_buffer[cnt + 11]);
	cnt = 1024;
	LOGD("%d [0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]",  cnt,
		capture_buffer[cnt], capture_buffer[cnt + 1], 
		capture_buffer[cnt + 2], capture_buffer[cnt + 3], 
		capture_buffer[cnt + 4], capture_buffer[cnt + 5],
		capture_buffer[cnt + 6], capture_buffer[cnt + 7], 
		capture_buffer[cnt + 8], capture_buffer[cnt + 9], 
		capture_buffer[cnt + 10], capture_buffer[cnt + 11]);
	cnt = 2048;
	LOGD("%d [0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]",  cnt,
		capture_buffer[cnt], capture_buffer[cnt + 1], 
		capture_buffer[cnt + 2], capture_buffer[cnt + 3], 
		capture_buffer[cnt + 4], capture_buffer[cnt + 5],
		capture_buffer[cnt + 6], capture_buffer[cnt + 7], 
		capture_buffer[cnt + 8], capture_buffer[cnt + 9], 
		capture_buffer[cnt + 10], capture_buffer[cnt + 11]);
#endif
	return 0;
}

void NotifyFromNative(unsigned char *buff)
{		
	jstring str;
	char strBuf[60] = {0};

	if (jvm) {
		JNIEnv *env = NULL;
		int status = jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
		int isAttached = 0;

		if (status == JNI_EDETACHED) {
			status = jvm->AttachCurrentThread(&env, NULL);
			if (status < 0)
				return;
			isAttached = 1;
		}
	
		if (g_jNativesCls == NULL && 
			NULL == (g_jNativesCls = env->FindClass("com/example/sensor/Sensor"))) {
			LOGE("Unable to find class: com/example/sensor/Sensor");
			return;
		}

		if (g_jpost_eventId == NULL &&
			NULL == (g_jpost_eventId = env->GetStaticMethodID(g_jNativesCls, "Callback", "(Ljava/lang/String;)V"))) {
			LOGE("Unable to find postEventFromNative");
			return;
		}

		//16bit 3 samples
		sprintf(strBuf, "[0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]", 
				buff[0],buff[1],buff[2],buff[3],buff[4],buff[5]);

		str = env->NewStringUTF(strBuf);
		env->CallStaticVoidMethod(g_jNativesCls, g_jpost_eventId, str);

		if(isAttached) {
			jvm->DetachCurrentThread();
		}
	}
}

void* sensor_thread(void* arg)
{
	LOGD("sensor_thread start");
	
	while (!thread_Exit) {		
		spi_get_data();
		NotifyFromNative(adc_data);

		usleep(10);
	}

	LOGD("sensor_thread exit");
	pthread_exit(NULL);
	return NULL;
}

JNIEXPORT int
Java_com_example_sensor_Sensor_connect( JNIEnv* env,
                                                  jobject thiz, jstring server_ip, jstring client_ip )
{
	char *strServerIp = (char *)env->GetStringUTFChars(server_ip, NULL);
	char *strClientIp = (char *)env->GetStringUTFChars(client_ip, NULL);

	LOGD("server %s", strServerIp);
	LOGD("client %s", strClientIp);
	return SocketClientOpen(strServerIp, strClientIp, 1111);
}

JNIEXPORT int
Java_com_example_sensor_Sensor_disconnect( JNIEnv* env, jobject thiz )
{
	return SocketClientClose();
}

JNIEXPORT int
Java_com_example_sensor_Sensor_send( JNIEnv* env,
													  jobject thiz, jstring msg )
{
	char *strMsg = (char *)env->GetStringUTFChars(msg, NULL);
	LOGD("client send msg = %s  %d", strMsg, strlen(strMsg));
	return SocketSend(strMsg, strlen(strMsg));
}

JNIEXPORT void
Java_com_example_sensor_Sensor_GpioSetDirection( JNIEnv* env,
                                                  jobject thiz, jstring port, jboolean dir )
{
	int gpio;
	const char *strPort = env->GetStringUTFChars(port, NULL);

	gpio = atoi(strPort);
	if (dir) {
		//LOGD("GpioSetDirection %d value %d", gpio, DIR_OUTPUT);
		fpga_GpioSetDirection(gpio, DIR_OUTPUT);
	} else {
		//LOGD("GpioSetDirection %d value %d", gpio, DIR_INPUT);
		fpga_GpioSetDirection(gpio, DIR_INPUT);
	}
}

JNIEXPORT void
Java_com_example_sensor_Sensor_GpioSetValue( JNIEnv* env,
                                                  jobject thiz, jstring port, jboolean value )
{
	int gpio;
	const char *strPort = env->GetStringUTFChars(port, NULL);

	gpio = atoi(strPort);
	if (value) {
		//LOGD("fpga_GpioSetValue %d value %d", gpio, 1);
		fpga_GpioSetValue(gpio, 1);
	} else {
		//LOGD("fpga_GpioSetValue %d value %d", gpio, 0);
		fpga_GpioSetValue(gpio, 0);
	}
}

JNIEXPORT jboolean
Java_com_example_sensor_Sensor_GpioGetValue( JNIEnv* env,
                                                  jobject thiz, jstring port )
{
	int gpio;
	const char *utf8 = env->GetStringUTFChars(port, NULL);

	gpio = atoi(utf8);

	//LOGD("fpga_GpioGetValue %d value %d", gpio, fpga_GpioGetValue(gpio));
	return fpga_GpioGetValue(gpio);
}

JNIEXPORT int
Java_com_example_sensor_Sensor_fpgaDaqTest( JNIEnv* env,
                                                  jobject thiz, jint freq )
{
	if (freq == 1 || freq == 2 || freq == 4 || freq == 5 || freq == 8 ||
		freq == 10 || freq == 16 || freq == 20 || freq == 25 || freq == 50)
		fpga_taskStart(freq);
	else {
		LOGE("SEM DAQ Freq Setting Fail %d", freq);
		return -1;
	}
	return 0;
}

JNIEXPORT int
Java_com_example_sensor_Sensor_fpgaStop( JNIEnv* env,
                                                  jobject thiz )
{
	return fpga_taskStop();
}

JNIEXPORT int
Java_com_example_sensor_Sensor_flashRead( JNIEnv* env,
                                                  jobject thiz, jstring fname )
{
	int ret;
	pthread_t pthread;
	const char *utf8 = env->GetStringUTFChars(fname, NULL);
	
	ret = pthread_create(&pthread, NULL, flashRead, (void *)utf8);
	if (ret) {
		LOGE("pthread create failed");
		return -1;
	}

	return 0;
}

JNIEXPORT int
Java_com_example_sensor_Sensor_flashUpdate( JNIEnv* env,
                                                  jobject thiz, jstring fname )
{
	int ret;
	pthread_t pthread;
	const char *utf8 = env->GetStringUTFChars(fname, NULL);
	
	ret = pthread_create(&pthread, NULL, flashUpdate, (void *)utf8);
	if (ret) {
		LOGE("pthread create failed");
		return -1;
	}

	return 0;
}

#define PAGE_ID		0x00
#define NETWORK_ID	0x02
#define GLOB_CMD	0x12
#define CMD_DATA	0x14

int page_write(int page, unsigned char reg, unsigned short value)
{
	int ret;

	ret = spi_regwrite(0, PAGE_ID, page);	//PAGE
	if (ret < 0) {
		LOGE("spi_regwrite fail %d", ret);
		return -1;
	}

	ret = spi_regwrite(0, CMD_DATA, page);	//CMD_DATA
	if (ret < 0) {
		LOGE("spi_regwrite fail");
		return -1;
	}
	usleep(1000);

	ret = spi_regwrite(0, reg, value);
	if (ret < 0) {
		LOGE("spi_regwrite fail");
		return -1;
	}

	ret = spi_regwrite(0, GLOB_CMD, 0x0002);	//CLOB_CMD_G
	if (ret < 0) {
		LOGE("spi_regwrite fail");
		return -1;
	}

	return 0;
}

int page_read(int page, unsigned char reg)
{
	int ret;
	
	ret = spi_regwrite(0, PAGE_ID, page);	//PAGE
	if (ret < 0) {
		LOGE("spi_regwrite fail %d", ret);
		return -1;
	}

	ret = spi_regwrite(0, CMD_DATA, page);	//CMD_DATA
	if (ret < 0) {
		LOGE("spi_regwrite fail %d", ret);
		return -1;
	}
	usleep(1000);

//	ret = spi_regwrite(0, GLOB_CMD, 0x0002);	//CLOB_CMD_G
//	if (ret < 0) {
//		LOGE("spi_regwrite fail %d", ret);
//		return -1;
//	}

	ret = spi_regwrite(0, GLOB_CMD, 0x2000);	//CLOB_CMD_G
	if (ret < 0) {
		LOGE("spi_regwrite fail %d", ret);
		return -1;
	}
	
	ret = spi_regread(0, reg);
	if (ret < 0) {
		LOGE("spi_regbytewrite fail %d", ret);
		return -1;
	}

	return (unsigned short)ret;
}

JNIEXPORT int
Java_com_example_sensor_Sensor_Wirelessvibtest( JNIEnv* env,
                                                  jobject thiz )
{
	int i, ret, value = 0x0;
	unsigned short reg;
	
	LOGD("Wireless vibtest");

	ret = spi_dev_open();
	if (ret < 0) {
		spi_dev_close();
		return -1;
	}

	LOGD("=== page0 ====");
	for (i = 0; i < 0x2e/2; i++) {
		reg = page_read(0, value);
		LOGD("0x%02x : 0x%04x\n", value, reg);
		value += 0x2;
	}

	LOGD("=== page1 ====");
	value = 0x0;
	for (i = 0; i < 0x6e / 2; i++) {
		reg = page_read(1, value);
		LOGD("0x%02x : 0x%04x\n", value, reg);
		value += 0x2;
	}

	LOGD("=== page2 ====");
	value = 0x0;
	for (i = 0; i < 0x6e / 2; i++) {
		reg = page_read(2, value);
		LOGD("0x%02x : 0x%04x\n", value, reg);
		value += 0x2;
	}

	LOGD("=== page3 ====");
	value = 0x0;
	for (i = 0; i < 0x6e / 2; i++) {
		reg = page_read(3, value);
		LOGD("0x%02x : 0x%04x\n", value, reg);
		value += 0x2;
	}

	LOGD("=== page4 ====");
	value = 0x0;
	for (i = 0; i < 0x6e / 2; i++) {
		reg = page_read(4, value);
		LOGD("0x%02x : 0x%04x\n", value, reg);
		value += 0x2;
	}

	LOGD("=== page5 ====");
	value = 0x0;
	for (i = 0; i < 0x6e / 2; i++) {
		reg = page_read(5, value);
		LOGD("0x%02x : 0x%04x\n", value, reg);
		value += 0x2;
	}

	LOGD("=== page6 ====");
	value = 0x0;
	for (i = 0; i < 0x6e / 2; i++) {
		reg = page_read(6, value);
		LOGD("0x%02x : 0x%04x\n", value, reg);
		value += 0x2;
	}

	return 0;
}

JNIEXPORT int
Java_com_example_sensor_Sensor_vibratortest( JNIEnv* env,
                                                  jobject thiz )
{
	int ret;
	ret = spi_dev_open();
	if (ret < 0) {
		spi_dev_close();
		return -1;
	}
	LOGD("spi_dev_open success");
#if 0
	//wireless vibrator
	int i, value = 0x0;
	unsigned short reg;

	//page0
	spi_regwrite(0, 0x0, 0x0);
	spi_regwrite(0, 0x12, 0x01);
	spi_regwrite(0, 0x14, 0x01);

	//gpo ctrl dio1 falling edge
	spi_regwrite(0, 0x2a, 0x38);
	spi_regwrite(0, 0x2b, 0x00);
	sleep(1);

	//page1
	spi_regbytewrite(0, 0x0, 0x1);
	spi_regbytewrite(0, 0x36, 0x40);
	spi_regbytewrite(0, 0x0, 0x0);
	spi_regbytewrite(0, 0x12, 0x02);
	
	spi_regbytewrite(0, 0x13, 0x38);
	//spi_regbytewrite(0, 0x13, 0x20);
	sleep(1);


	LOGD("=== page0 ====");
	value = 0x0;
	for (i = 0; i < 0x2e/2; i++) {
		reg = spi_regread(0, value);
		LOGD("0x%02x : 0x%04x\n", value, reg);
		value += 0x2;
	}

	LOGD("=== page1 ====");
	spi_regwrite(0, 0x0, 0x1);

	//page1 - rec_ctrl realtime
	spi_regbytewrite(0, 0x1a, 0x3);
	spi_regbytewrite(0, 0x1b, 0x0);
	
	spi_regbytewrite(0, 0x32, 0x04);
	spi_regbytewrite(0, 0x33, 0x0);

	value = 0x0;
	sleep(1);
	for (i = 0; i < 0x6e / 2; i++) {
		reg = spi_regread(0, value);
		LOGD("0x%02x : 0x%04x\n", value, reg);
		value += 0x2;
	}
#endif

	if (gDev >= 0) {
		ret = ioctl(gDev, IOCTL_SPI_SENSOR_SCAN, NULL);
		if (ret) {
			LOGE("SPI_WRITE error\n");
			return -1;
		}
	} else
		LOGE("SPI_WRITE error\n");

	spi_dev_close();

	return 0;
}

JNIEXPORT void
Java_com_example_sensor_Sensor_DeviceClose( JNIEnv* env,
                                                  jobject thiz )
{
	spi_dev_close();
	thread_Exit = 1;
	if (sensor_pthread)
		pthread_join(sensor_pthread, NULL);

	fpga_taskStop();
	SocketClientClose();
	LOGD("DeviceClose");
}

JNIEXPORT void JNICALL Java_com_example_sensor_Sensor_CallBackInit(JNIEnv *env, jobject thiz)
{		
	LOGD("CallBackInit");
	int status;

	if (!jvm)
		return;

	status = jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
	if (status < 0) {
		LOGE("CallBackInit - GetEnv status < 0");
	}

	g_jNativesCls = env->FindClass("com/example/sensor/Sensor");
	if (g_jNativesCls == NULL) {
		LOGE("native_init_Can't find  com/example/sensor/Sensor");
		return;
	}

	g_jpost_eventId = env->GetStaticMethodID(g_jNativesCls, "Callback", "(Ljava/lang/String;)V");
	if (g_jpost_eventId == NULL) {
		LOGE("Can't find Callback");
		return;
	}
}


jobject 	g_jobject = NULL;
jclass		g_clazz = NULL;
jobject		g_jobjBufferMax = NULL;
jobject		g_jobjBufferAds1 = NULL;
jobject		g_jobjBufferAds2 = NULL;
jobject		g_jobjBufferAds3 = NULL;
jobject		g_jobjBufferAds4 = NULL;

extern int gFreq;
extern unsigned char fpga_data[20000 * 5];


JNIEXPORT int
Java_com_example_sensor_Sensor_fpgaOpen( JNIEnv* env, jobject thiz, jint freq )
{
	jint nRet = 0;

	jclass cls = env->GetObjectClass( thiz);
	jclass clazz = (jclass)env->NewGlobalRef(cls);
	jmethodID mid = env->GetMethodID( clazz, "onDatas", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");


	jobject directBufferMax = env->NewDirectByteBuffer((void*)fpga_data, 2000);
	jobject directBufferAds1 = env->NewDirectByteBuffer((void*)(fpga_data+20000), 2000);
	jobject directBufferAds2 = env->NewDirectByteBuffer((void*)(fpga_data+40000), 2000);
	jobject directBufferAds3 = env->NewDirectByteBuffer((void*)(fpga_data+60000), 2000);
	jobject directBufferAds4 = env->NewDirectByteBuffer((void*)(fpga_data+80000), 2000);

	g_jobjBufferMax = env->NewGlobalRef(directBufferMax);
	g_jobjBufferAds1 = env->NewGlobalRef(directBufferAds1);
	g_jobjBufferAds2 = env->NewGlobalRef(directBufferAds2);
	g_jobjBufferAds3 = env->NewGlobalRef(directBufferAds3);
	g_jobjBufferAds4 = env->NewGlobalRef(directBufferAds4);

	env->CallVoidMethod(thiz, mid, g_jobjBufferMax, g_jobjBufferAds1, g_jobjBufferAds2, g_jobjBufferAds3, g_jobjBufferAds4);

	nRet = fpga_open(freq);

	return nRet;
}

JNIEXPORT int
Java_com_example_sensor_Sensor_fpgaGetData( JNIEnv* env, jobject thiz )
{
	return fpga_datas();
}

JNIEXPORT void
Java_com_example_sensor_Sensor_fpgaAbort( JNIEnv* env, jobject thiz )
{

	fpga_abort();

	env->DeleteGlobalRef(g_jobjBufferMax);
	env->DeleteGlobalRef(g_jobjBufferAds1);
	env->DeleteGlobalRef(g_jobjBufferAds2);
	env->DeleteGlobalRef(g_jobjBufferAds3);
	env->DeleteGlobalRef(g_jobjBufferAds4);
}




jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	LOGD("JNI_OnLoad start");

	JNIEnv	*env;
	jvm = vm;

	if (vm->GetEnv((void**)&g_env, JNI_VERSION_1_4) != JNI_OK) {
		LOGD("failed to get the environment using GetEnv()");
		return -1;
	}

	env = g_env;

	LOGD("JNI_OnLoad end");

	return JNI_VERSION_1_4;
}

void  JNI_OnUnload(JavaVM *jvm, void *reserved)
{
	LOGD("JNI_OnUnload start");

	JNIEnv *env;
	if (jvm->GetEnv((void **)&env, JNI_VERSION_1_4)) {
		return;
	}

	LOGD("JNI_OnUnload end");

	return;
}

#ifdef __cplusplus
}
#endif

