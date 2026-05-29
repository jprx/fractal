#ifndef _SYS_UTIME_H
#define _SYS_UTIME_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct utimbuf 
{
  time_t actime;
  time_t modtime; 
};

int utime(const char *, const struct utimbuf *);

#ifdef __cplusplus
};
#endif

#endif /* _SYS_UTIME_H */
