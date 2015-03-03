#ifndef STAK_H
#define STAK_H

#define LOG_ERRORS 0
#define LOG_INFO 1
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void stak_activate_mode();

const char *stak_assets_path();

#ifdef __cplusplus
}
#endif

#endif
