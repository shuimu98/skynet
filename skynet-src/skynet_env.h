#ifndef SKYNET_ENV_H
#define SKYNET_ENV_H

#include <stdint.h>

const char * skynet_getenv(const char *key);
void skynet_setenv(const char *key, const char *value);
int64_t skynet_snowflake(int machine);

void skynet_env_init();

#endif
