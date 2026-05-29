// Half of this is the default newlib signal.h,
// and some of the defines are pulled from cygwin's
#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "_ansi.h"
#include <sys/cdefs.h>
#include <sys/signal.h>

_BEGIN_STD_C

typedef int	sig_atomic_t;		/* Atomic entity type (ANSI) */
#if __BSD_VISIBLE
typedef _sig_func_ptr sig_t;		/* BSD naming */
#endif
#if __GNU_VISIBLE
typedef _sig_func_ptr sighandler_t;	/* glibc naming */
#endif

#define SIG_DFL ((_sig_func_ptr)0)	/* Default action */
#define SIG_IGN ((_sig_func_ptr)1)	/* Ignore action */
#define SIG_ERR ((_sig_func_ptr)-1)	/* Error return */

struct _reent;

_sig_func_ptr _signal_r (struct _reent *, int, _sig_func_ptr);
int	_raise_r (struct _reent *, int);

#ifndef _REENT_ONLY
_sig_func_ptr signal (int, _sig_func_ptr);
int	raise (int);
void	psignal (int, const char *);
#endif


#define SA_NOCLDSTOP 1			/* Do not generate SIGCHLD when children
					   stop */
#define SA_SIGINFO   2			/* Invoke the signal catching function
					   with three arguments instead of one
					 */
#define SA_RESTART   0x10000000		/* Restart syscall on signal return */
#define SA_ONSTACK   0x20000000		/* Call signal handler on alternate
					   signal stack provided by
					   sigaltstack(2). */
#define SA_NODEFER   0x40000000		/* Don't automatically block the signal
					   when its handler is being executed  */
#define SA_RESETHAND 0x80000000		/* Reset to SIG_DFL on entry to handler */
#define SA_ONESHOT   SA_RESETHAND	/* Historical linux name */
#define SA_NOMASK    SA_NODEFER		/* Historical linux name */

/* Used internally by cygwin.  Included here to group everything in one place.
   Do not use.  */
#define _SA_INTERNAL_MASK 0xf000	/* bits in this range are internal */

_END_STD_C

#endif /* _SIGNAL_H_ */
