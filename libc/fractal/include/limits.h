#define _POSIX_AIO_LISTIO_MAX   2
#define _POSIX_AIO_MAX          1
#define _POSIX_ARG_MAX          4096
#define _POSIX_CHILD_MAX        6
#define _POSIX_DELAYTIMER_MAX   32
#define _POSIX_HOST_NAME_MAX    255
#define _POSIX_LINK_MAX         8
#define _POSIX_MAX_CANON        255
#define _POSIX_MAX_INPUT        255
#define _POSIX_MQ_OPEN_MAX      8
#define _POSIX_MQ_PRIO_MAX      32
#define _POSIX_NAME_MAX         255
#define _POSIX_NGROUPS_MAX      0
#define _POSIX_OPEN_MAX         16
#define _POSIX_PATH_MAX         255
#define _POSIX_PIPE_BUF         512
/* The maximum number of repeated occurrences of a regular expression
 *  *    permitted when using the interval notation `\{M,N\}'.  */
#define _POSIX2_RE_DUP_MAX              255
#define _POSIX_RTSIG_MAX        8
#define _POSIX_SEM_NSEMS_MAX    256
#define _POSIX_SEM_VALUE_MAX    32767
#define _POSIX_SIGQUEUE_MAX     32
#define _POSIX_SSIZE_MAX        32767
#define _POSIX_STREAM_MAX       8
#define _POSIX_TIMER_MAX        32
#define _POSIX_TZNAME_MAX       3

/*
 *  Definitions of the following may be omitted if the value is >= stated
 *  minimum but is indeterminate.
 */

#define AIO_LISTIO_MAX          2
#define AIO_MAX                 1
#define AIO_PRIO_DELTA_MAX      0
#define DELAYTIMER_MAX          32
#define MQ_OPEN_MAX             8
#define MQ_PRIO_MAX             32
#define PAGESIZE                (1<<12)
#define RTSIG_MAX               8
#define SEM_NSEMS_MAX           256
#define SEM_VALUE_MAX           INT_MAX
#define SIGQUEUE_MAX            32
#define STREAM_MAX              8
#define TIMER_MAX               32
#define TZNAME_MAX              3

/*
 *  Invariant values
 */

#ifdef __SIZE_MAX__
#define SSIZE_MAX    (__SIZE_MAX__ >> 1)
#elif defined(__SIZEOF_SIZE_T__) && defined(__CHAR_BIT__)
#define SSIZE_MAX               ((1UL << (__SIZEOF_SIZE_T__ * __CHAR_BIT__ - 1)) - 1)
#else /* historic fallback, wrong in most cases */
#define SSIZE_MAX               32767
#endif

/*
 *  Maximum Values
 */

#define _POSIX_CLOCKRES_MIN      0   /* in nanoseconds */

/****************************************************************************
 ****************************************************************************
 *                                                                          *
 *         P1003.1c/D10 defines the constants below this comment.           *
 *
 *  XXX: doc seems to have printing problems in this table :(
 *                                                                          *
 **************************************************************************** 
 ****************************************************************************/

#define _POSIX_LOGIN_NAME_MAX                9
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS  4
#define _POSIX_THREAD_KEYS_MAX               28
#define _POSIX_THREAD_THREADS_MAX            64
#define _POSIX_TTY_NAME_MAX                  9

/*
 *  Definitions of the following may be omitted if the value is >= stated
 *  minimum but is indeterminate.
 *
 *  NOTE:  LOGIN_NAME_MAX is named LOGNAME_MAX under Solaris 2.x.  Perhaps
 *         the draft specification will be changing.  jrs 05/24/96
 */

#define LOGIN_NAME_MAX                      _POSIX_LOGIN_NAME_MAX
#define TTY_NAME_MAX                        _POSIX_TTY_NAME_MAX
#define PTHREAD_DESTRUCTOR_ITERATIONS       _POSIX_THREAD_DESTRUCTOR_ITERATIONS

/*
 *  RTEMS is smart enough to give us the minimum stack size if we ask
 *  for too little.  Because the real RTEMS limit for this is cpu dependent
 *  AND rtems header files are not installed yet, we cannot use the cpu
 *  dependent constant CPU_STACK_MINIMUM_SIZE.  Moreover, we do not want
 *  to duplicate that information here so we will just let RTEMS magically
 *  give us its minimum stack size.
 *
 *  NOTE:  The other alternative is to have this be a macro for a 
 *         routine in RTEMS which returns the constant.
 */

#define PTHREAD_STACK_MIN                   0

/*
 *  The maximum number of keys (PTHREAD_KEYS_MAX) and threads
 *  (PTHREAD_THREADS_MAX) are configurable and may exceed the minimum.
 *
#define PTHREAD_KEYS_MAX                    _POSIX_THREAD_KEYS_MAX
#define PTHREAD_THREADS_MAX                 _POSIX_THREAD_THREADS_MAX
*/


/****************************************************************************
 ****************************************************************************
 *                                                                          *
 *         P1003.4b/D8 defines the constants below this comment.            *
 *                                                                          *
 **************************************************************************** 
 ****************************************************************************/

#define _POSIX_INTERRUPT_OVERRUN_MAX        32

/*
 *  Definitions of the following may be omitted if the value is >= stated
 *  minimum but is indeterminate.
 */

#define INTERRUPT_OVERRUN_MAX               32

/*
 *  Pathname Variables
 */

#define MIN_ALLOC_SIZE      
#define REC_MIN_XFER_SIZE   
#define REC_MAX_XFER_SIZE   
#define REC_INCR_XFER_SIZE  
#define REC_XFER_ALIGN      
#define MAX_ATOMIC_SIZE     



#ifndef _LIBC_LIMITS_H_
# define _LIBC_LIMITS_H_	1

#include <newlib.h>
#include <sys/cdefs.h>
#include <sys/syslimits.h>

# ifdef _MB_LEN_MAX
#  define MB_LEN_MAX	_MB_LEN_MAX
# else
#  define MB_LEN_MAX    1
# endif

/* Maximum number of positional arguments, if _WANT_IO_POS_ARGS.  */
# ifndef NL_ARGMAX
#  define NL_ARGMAX 32
# endif

/* if do not have #include_next support, then we
   have to define the limits here. */
# if !defined __GNUC__ || __GNUC__ < 2

#  ifndef _LIMITS_H
#   define _LIMITS_H	1

#   include <sys/config.h>

/* Number of bits in a `char'.  */
#   undef CHAR_BIT
#   define CHAR_BIT 8

/* Minimum and maximum values a `signed char' can hold.  */
#   undef SCHAR_MIN
#   define SCHAR_MIN (-128)
#   undef SCHAR_MAX
#   define SCHAR_MAX 127

/* Maximum value an `unsigned char' can hold.  (Minimum is 0).  */
#   undef UCHAR_MAX
#   define UCHAR_MAX 255

/* Minimum and maximum values a `char' can hold.  */
#   ifdef __CHAR_UNSIGNED__
#    undef CHAR_MIN
#    define CHAR_MIN 0
#    undef CHAR_MAX
#    define CHAR_MAX 255
#   else
#    undef CHAR_MIN
#    define CHAR_MIN (-128)
#    undef CHAR_MAX
#    define CHAR_MAX 127
#   endif

/* Minimum and maximum values a `signed short int' can hold.  */
#   undef SHRT_MIN
/* For the sake of 16 bit hosts, we may not use -32768 */
#   define SHRT_MIN (-32767-1)
#   undef SHRT_MAX
#   define SHRT_MAX 32767

/* Maximum value an `unsigned short int' can hold.  (Minimum is 0).  */
#   undef USHRT_MAX
#   define USHRT_MAX 65535

/* Minimum and maximum values a `signed int' can hold.  */
#   ifndef __INT_MAX__
#    define __INT_MAX__ 2147483647
#   endif
#   undef INT_MIN
#   define INT_MIN (-INT_MAX-1)
#   undef INT_MAX
#   define INT_MAX __INT_MAX__

/* Maximum value an `unsigned int' can hold.  (Minimum is 0).  */
#   undef UINT_MAX
#   define UINT_MAX (INT_MAX * 2U + 1)

/* Minimum and maximum values a `signed long int' can hold.
   (Same as `int').  */
#   ifndef __LONG_MAX__
#    if defined (__alpha__) || (defined (__sparc__) && defined(__arch64__)) || defined (__sparcv9)
#     define __LONG_MAX__ 9223372036854775807L
#    else
#     define __LONG_MAX__ 2147483647L
#    endif /* __alpha__ || sparc64 */
#   endif
#   undef LONG_MIN
#   define LONG_MIN (-LONG_MAX-1)
#   undef LONG_MAX
#   define LONG_MAX __LONG_MAX__

/* Maximum value an `unsigned long int' can hold.  (Minimum is 0).  */
#   undef ULONG_MAX
#   define ULONG_MAX (LONG_MAX * 2UL + 1)

#   ifndef __LONG_LONG_MAX__
#    define __LONG_LONG_MAX__ 9223372036854775807LL
#   endif

#   if __ISO_C_VISIBLE >= 1999
/* Minimum and maximum values a `signed long long int' can hold.  */
#    undef LLONG_MIN
#    define LLONG_MIN (-LLONG_MAX-1)
#    undef LLONG_MAX
#    define LLONG_MAX __LONG_LONG_MAX__

/* Maximum value an `unsigned long long int' can hold.  (Minimum is 0).  */
#    undef ULLONG_MAX
#    define ULLONG_MAX (LLONG_MAX * 2ULL + 1)
#   endif

#  if __GNU_VISIBLE
/* Minimum and maximum values a `signed long long int' can hold.  */
#    undef LONG_LONG_MIN
#    define LONG_LONG_MIN (-LONG_LONG_MAX-1)
#    undef LONG_LONG_MAX
#    define LONG_LONG_MAX __LONG_LONG_MAX__

/* Maximum value an `unsigned long long int' can hold.  (Minimum is 0).  */
#    undef ULONG_LONG_MAX
#    define ULONG_LONG_MAX (LONG_LONG_MAX * 2ULL + 1)
#   endif

#  endif /* _LIMITS_H  */
# endif	 /* GCC 2.  */

#endif	 /* !_LIBC_LIMITS_H_ */

#if defined __GNUC__ && !defined _GCC_LIMITS_H_
/* `_GCC_LIMITS_H_' is what GCC's file defines.  */
# include_next <limits.h>
#endif /* __GNUC__ && !_GCC_LIMITS_H_ */

#ifndef _POSIX2_RE_DUP_MAX
/* The maximum number of repeated occurrences of a regular expression
 *    permitted when using the interval notation `\{M,N\}'.  */
#define _POSIX2_RE_DUP_MAX              255
#endif /* _POSIX2_RE_DUP_MAX  */

#ifndef ARG_MAX
#define ARG_MAX		4096
#endif

#ifndef PATH_MAX
#define PATH_MAX	4096
#endif