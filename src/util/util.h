#pragma once

#if CONFIG_LOG
#include <zephyr/logging/log.h>
#endif
#include "logging.h"

#define CHECK_NULL_ARG_AND_RETURN(arg_ptr, ret_val)                                                \
	do {                                                                                       \
		if (arg_ptr == NULL) {                                                             \
			LOG_ERR("Parameter \"" #arg_ptr "\" is NULL");                           \
			return -ret_val;                                                           \
		}                                                                                  \
	} while (0)

