#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define UNIT_CYLINDERS	1024

void* flashUpdate(void* args);
void* flashRead(void* args);

#ifdef __cplusplus
}
#endif

#endif
