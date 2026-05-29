/* sys/ioctl.h

This file is part of Cygwin.

This software is a copyrighted work licensed under the terms of the
Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
details. */

/* sys/ioctl.h */

#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#include <sys/cdefs.h>
#include <sys/termios.h>

__BEGIN_DECLS

/* Some standard linux defines */

#define _IOC_NRBITS	8
#define _IOC_TYPEBITS	8
#define _IOC_SIZEBITS	14
#define _IOC_DIRBITS	2

#define _IOC_NRMASK	((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK	((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK	((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK	((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT	0
#define _IOC_TYPESHIFT	(_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT	(_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT	(_IOC_SIZESHIFT+_IOC_SIZEBITS)

#define _IOC_NONE	0U
#define _IOC_WRITE	1U
#define _IOC_READ	2U

#define _IOC(dir,type,nr,size) \
  (((dir)	<< _IOC_DIRSHIFT) | \
    +	((type) << _IOC_TYPESHIFT) | \
    +	((nr)	<< _IOC_NRSHIFT) | \
    +	((size) << _IOC_SIZESHIFT))

#define _LINUX_IO(type,nr)		_IOC(_IOC_NONE,(type),(nr),0)
#define _LINUX_IOR(type,nr,size)	_IOC(_IOC_READ,(type),(nr),sizeof(size))
#define _LINUX_IOW(type,nr,size)	_IOC(_IOC_WRITE,(type),(nr),sizeof(size))
#define _LINUX_IOWR(type,nr,size)	_IOC(_IOC_READ|_IOC_WRITE,(type),(nr),sizeof(size))

#define _IO	_LINUX_IO
#define _IOR	_LINUX_IOR
#define _IOW	_LINUX_IOW
#define _IOWR	_LINUX_IOWR

int ioctl (int __fd, int __cmd, ...);

__END_DECLS
#endif
