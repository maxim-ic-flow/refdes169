#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#define ARRAY_COUNT(x)  (sizeof(x)/sizeof(*x))
#define MAX(a,b)	((a)>(b)?(a):(b))