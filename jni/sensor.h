#ifndef __SENSOR_H__
#define __SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <android/log.h>

#define  LOG_TAG    "sensor"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)

#define IOCTL_SPI_READ				_IO('S', 1)
#define IOCTL_SPI_WRITE				_IO('S', 2)
#define IOCTL_SPI_GET_DATA			_IO('S', 3)
#define IOCTL_SPI_MANUAL_CAPTURE	_IO('S', 4)
#define IOCTL_SPI_REALTIME			_IO('S', 5)
#define IOCTL_SPI_BYTE_WRITE		_IO('S', 6)
#define IOCTL_SPI_SENSOR_SCAN		_IO('S', 7)

struct spi_info{
	unsigned int 	channel;
	unsigned char *	inbuff;
	unsigned char *	outbuff;
	unsigned int 	size;
};

#ifdef __cplusplus
}
#endif

#endif
