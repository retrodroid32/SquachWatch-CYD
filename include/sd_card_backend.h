#pragma once
#include <SD.h>
#if defined(FREENOVE35_N)
#include <SD_MMC.h>
#define SQW_CARD SD_MMC
#else
#define SQW_CARD SD
#endif
