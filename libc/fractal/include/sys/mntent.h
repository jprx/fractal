#ifndef _MNTENT_H
#define _MNTENT_H

// From musl include/mntent.h

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_FILE

#define MOUNTED "/etc/mtab"

#define MNTTYPE_IGNORE	"ignore"
#define MNTTYPE_NFS	"nfs"
#define MNTTYPE_SWAP	"swap"
#define MNTOPT_DEFAULTS	"defaults"
#define MNTOPT_RO	"ro"
#define MNTOPT_RW	"rw"
#define MNTOPT_SUID	"suid"
#define MNTOPT_NOSUID	"nosuid"
#define MNTOPT_NOAUTO	"noauto"

struct mntent {
	char *mnt_fsname;
	char *mnt_dir;
	char *mnt_type;
	char *mnt_opts;
	int mnt_freq;
	int mnt_passno;
};

#include <stdio.h>
FILE *setmntent(const char *, const char *);
int endmntent(FILE *);
struct mntent *getmntent(FILE *);
struct mntent *getmntent_r(FILE *, struct mntent *, char *, int);
int addmntent(FILE *, const struct mntent *);
char *hasmntopt(const struct mntent *, const char *);

#ifdef __cplusplus
}
#endif

#endif
