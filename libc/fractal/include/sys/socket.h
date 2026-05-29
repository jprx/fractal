#ifndef FRACTAL_LIBC_SOCKET_H
#define FRACTAL_LIBC_SOCKET_H

// Parts copied from rtems newlib headers
#include <sys/_types.h>

// For __htons and family
#include <machine/endian.h>

typedef	__socklen_t	socklen_t;
typedef __sa_family_t sa_family_t;

/*
 * Types
 */
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */
#if __BSD_VISIBLE
#define	SOCK_RDM	4		/* reliably-delivered message */
#endif
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	0x00000001	/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x00000002	/* socket has had listen() */
#define	SO_REUSEADDR	0x00000004	/* allow local address reuse */
#define	SO_KEEPALIVE	0x00000008	/* keep connections alive */
#define	SO_DONTROUTE	0x00000010	/* just use interface addresses */
#define	SO_BROADCAST	0x00000020	/* permit sending of broadcast msgs */
#if __BSD_VISIBLE
#define	SO_USELOOPBACK	0x00000040	/* bypass hardware when possible */
#endif
#define	SO_LINGER	0x00000080	/* linger on close if data present */
#define	SO_OOBINLINE	0x00000100	/* leave received OOB data in line */
#if __BSD_VISIBLE
#define	SO_REUSEPORT	0x00000200	/* allow local address & port reuse */
#define	SO_TIMESTAMP	0x00000400	/* timestamp received dgram traffic */
#define	SO_NOSIGPIPE	0x00000800	/* no SIGPIPE from EPIPE */
#define	SO_ACCEPTFILTER	0x00001000	/* there is an accept filter */
#define	SO_BINTIME	0x00002000	/* timestamp received dgram traffic */
#endif
#define	SO_NO_OFFLOAD	0x00004000	/* socket cannot be offloaded */
#define	SO_NO_DDP	0x00008000	/* disable direct data placement */
#define	SO_REUSEPORT_LB	0x00010000	/* reuse with load balancing */
#define	SO_RERROR	0x00020000	/* keep track of receive errors */


/*
 * Additional options, not kept in so_options.
 */
#define	SO_SNDBUF	0x1001		/* send buffer size */
#define	SO_RCVBUF	0x1002		/* receive buffer size */
#define	SO_SNDLOWAT	0x1003		/* send low-water mark */
#define	SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define	SO_SNDTIMEO	0x1005		/* send timeout */
#define	SO_RCVTIMEO	0x1006		/* receive timeout */
#define	SO_ERROR	0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */
#if __BSD_VISIBLE
#define	SO_LABEL	0x1009		/* socket's MAC label */
#define	SO_PEERLABEL	0x1010		/* socket's peer's MAC label */
#define	SO_LISTENQLIMIT	0x1011		/* socket's backlog limit */
#define	SO_LISTENQLEN	0x1012		/* socket's complete queue length */
#define	SO_LISTENINCQLEN	0x1013	/* socket's incomplete queue length */
#define	SO_SETFIB	0x1014		/* use this FIB to route */
#define	SO_USER_COOKIE	0x1015		/* user cookie (dummynet etc.) */
#define	SO_PROTOCOL	0x1016		/* get socket protocol (Linux name) */
#define	SO_PROTOTYPE	SO_PROTOCOL	/* alias for SO_PROTOCOL (SunOS name) */
#define	SO_TS_CLOCK	0x1017		/* clock type used for SO_TIMESTAMP */
#define	SO_MAX_PACING_RATE	0x1018	/* socket's max TX pacing rate (Linux name) */
#define	SO_DOMAIN	0x1019		/* get socket domain */
#endif

#if __BSD_VISIBLE
#define	SO_TS_REALTIME_MICRO	0	/* microsecond resolution, realtime */
#define	SO_TS_BINTIME		1	/* sub-nanosecond resolution, realtime */
#define	SO_TS_REALTIME		2	/* nanosecond resolution, realtime */
#define	SO_TS_MONOTONIC		3	/* nanosecond resolution, monotonic */
#define	SO_TS_DEFAULT		SO_TS_REALTIME_MICRO
#define	SO_TS_CLOCK_MAX		SO_TS_MONOTONIC
#endif

/*
 * Space reserved for new socket options added by third-party vendors.
 * This range applies to all socket option levels.  New socket options
 * in FreeBSD should always use an option value less than SO_VENDOR.
 */
#if __BSD_VISIBLE
#define	SO_VENDOR	0x80000000
#endif

/*
 * Structure used for manipulating linger option.
 */
struct linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};

#if __BSD_VISIBLE
struct accept_filter_arg {
	char	af_name[16];
	char	af_arg[256-16];
};
#endif

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */

/*
 * Address families.
 */
#define	AF_UNSPEC	0		/* unspecified */
#if __BSD_VISIBLE
#define	AF_LOCAL	AF_UNIX		/* local to host (pipes, portals) */
#endif
#define	AF_UNIX		1		/* standardized name for AF_LOCAL */
#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */
#if __BSD_VISIBLE
#define	AF_IMPLINK	3		/* arpanet imp addresses */
#define	AF_PUP		4		/* pup protocols: e.g. BSP */
#define	AF_CHAOS	5		/* mit CHAOS protocols */
#define	AF_NETBIOS	6		/* SMB protocols */
#define	AF_ISO		7		/* ISO protocols */
#define	AF_OSI		AF_ISO
#define	AF_ECMA		8		/* European computer manufacturers */
#define	AF_DATAKIT	9		/* datakit protocols */
#define	AF_CCITT	10		/* CCITT protocols, X.25 etc */
#define	AF_SNA		11		/* IBM SNA */
#define AF_DECnet	12		/* DECnet */
#define AF_DLI		13		/* DEC Direct data link interface */
#define AF_LAT		14		/* LAT */
#define	AF_HYLINK	15		/* NSC Hyperchannel */
#define	AF_APPLETALK	16		/* Apple Talk */
#define	AF_ROUTE	17		/* Internal Routing Protocol */
#define	AF_LINK		18		/* Link layer interface */
#define	pseudo_AF_XTP	19		/* eXpress Transfer Protocol (no AF) */
#define	AF_COIP		20		/* connection-oriented IP, aka ST II */
#define	AF_CNT		21		/* Computer Network Technology */
#define pseudo_AF_RTIP	22		/* Help Identify RTIP packets */
#define	AF_IPX		23		/* Novell Internet Protocol */
#define	AF_SIP		24		/* Simple Internet Protocol */
#define	pseudo_AF_PIP	25		/* Help Identify PIP packets */
#define	AF_ISDN		26		/* Integrated Services Digital Network*/
#define	AF_E164		AF_ISDN		/* CCITT E.164 recommendation */
#define	pseudo_AF_KEY	27		/* Internal key-management function */
#endif
#define	AF_INET6	28		/* IPv6 */
#if __BSD_VISIBLE
#define	AF_NATM		29		/* native ATM access */
#define	AF_ATM		30		/* ATM */
#define pseudo_AF_HDRCMPLT 31		/* Used by BPF to not rewrite headers
					 * in interface output routine
					 */
#define	AF_NETGRAPH	32		/* Netgraph sockets */
#define	AF_SLOW		33		/* 802.3ad slow protocol */
#define	AF_SCLUSTER	34		/* Sitara cluster protocol */
#define	AF_ARP		35
#define	AF_BLUETOOTH	36		/* Bluetooth sockets */
#define	AF_IEEE80211	37		/* IEEE 802.11 protocol */
#define	AF_INET_SDP	40		/* OFED Socket Direct Protocol ipv4 */
#define	AF_INET6_SDP	42		/* OFED Socket Direct Protocol ipv6 */
#define	AF_HYPERV	43		/* HyperV sockets */
#define	AF_MAX		43
/*
 * When allocating a new AF_ constant, please only allocate
 * even numbered constants for FreeBSD until 134 as odd numbered AF_
 * constants 39-133 are now reserved for vendors.
 */
#define AF_VENDOR00 39
#define AF_VENDOR01 41
#define AF_VENDOR03 45
#define AF_VENDOR04 47
#define AF_VENDOR05 49
#define AF_VENDOR06 51
#define AF_VENDOR07 53
#define AF_VENDOR08 55
#define AF_VENDOR09 57
#define AF_VENDOR10 59
#define AF_VENDOR11 61
#define AF_VENDOR12 63
#define AF_VENDOR13 65
#define AF_VENDOR14 67
#define AF_VENDOR15 69
#define AF_VENDOR16 71
#define AF_VENDOR17 73
#define AF_VENDOR18 75
#define AF_VENDOR19 77
#define AF_VENDOR20 79
#define AF_VENDOR21 81
#define AF_VENDOR22 83
#define AF_VENDOR23 85
#define AF_VENDOR24 87
#define AF_VENDOR25 89
#define AF_VENDOR26 91
#define AF_VENDOR27 93
#define AF_VENDOR28 95
#define AF_VENDOR29 97
#define AF_VENDOR30 99
#define AF_VENDOR31 101
#define AF_VENDOR32 103
#define AF_VENDOR33 105
#define AF_VENDOR34 107
#define AF_VENDOR35 109
#define AF_VENDOR36 111
#define AF_VENDOR37 113
#define AF_VENDOR38 115
#define AF_VENDOR39 117
#define AF_VENDOR40 119
#define AF_VENDOR41 121
#define AF_VENDOR42 123
#define AF_VENDOR43 125
#define AF_VENDOR44 127
#define AF_VENDOR45 129
#define AF_VENDOR46 131
#define AF_VENDOR47 133
#endif

typedef	uint32_t	in_addr_t;

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
	unsigned char	sa_len;		/* total length */
	sa_family_t	sa_family;	/* address family */
	char		sa_data[14];	/* actually longer; address value */
};
#if __BSD_VISIBLE
#define	SOCK_MAXADDRLEN	255		/* longest possible addresses */

struct in_addr {
	in_addr_t s_addr;
};

/* Socket address, internet style. */
struct sockaddr_in {
	uint8_t	sin_len;
	sa_family_t	sin_family;
	in_port_t	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	unsigned short	sp_family;		/* address family */
	unsigned short	sp_protocol;		/* protocol */
};
#endif

#ifndef _BYTEORDER_PROTOTYPED
#define	_BYTEORDER_PROTOTYPED
__BEGIN_DECLS
uint32_t	htonl(uint32_t);
uint16_t	htons(uint16_t);
uint32_t	ntohl(uint32_t);
uint16_t	ntohs(uint16_t);
__END_DECLS
#endif

#ifndef _BYTEORDER_FUNC_DEFINED
#define	_BYTEORDER_FUNC_DEFINED
#define	htonl(x)	__htonl(x)
#define	htons(x)	__htons(x)
#define	ntohl(x)	__ntohl(x)
#define	ntohs(x)	__ntohs(x)
#endif

#if __BSD_VISIBLE
/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC	AF_UNSPEC
#define	PF_LOCAL	AF_LOCAL
#define	PF_UNIX		PF_LOCAL	/* backward compatibility */
#define	PF_INET		AF_INET
#define	PF_IMPLINK	AF_IMPLINK
#define	PF_PUP		AF_PUP
#define	PF_CHAOS	AF_CHAOS
#define	PF_NETBIOS	AF_NETBIOS
#define	PF_ISO		AF_ISO
#define	PF_OSI		AF_ISO
#define	PF_ECMA		AF_ECMA
#define	PF_DATAKIT	AF_DATAKIT
#define	PF_CCITT	AF_CCITT
#define	PF_SNA		AF_SNA
#define PF_DECnet	AF_DECnet
#define PF_DLI		AF_DLI
#define PF_LAT		AF_LAT
#define	PF_HYLINK	AF_HYLINK
#define	PF_APPLETALK	AF_APPLETALK
#define	PF_ROUTE	AF_ROUTE
#define	PF_LINK		AF_LINK
#define	PF_XTP		pseudo_AF_XTP	/* really just proto family, no AF */
#define	PF_COIP		AF_COIP
#define	PF_CNT		AF_CNT
#define	PF_SIP		AF_SIP
#define	PF_IPX		AF_IPX
#define PF_RTIP		pseudo_AF_RTIP	/* same format as AF_INET */
#define PF_PIP		pseudo_AF_PIP
#define	PF_ISDN		AF_ISDN
#define	PF_KEY		pseudo_AF_KEY
#define	PF_INET6	AF_INET6
#define	PF_NATM		AF_NATM
#define	PF_ATM		AF_ATM
#define	PF_NETGRAPH	AF_NETGRAPH
#define	PF_SLOW		AF_SLOW
#define PF_SCLUSTER	AF_SCLUSTER
#define	PF_ARP		AF_ARP
#define	PF_BLUETOOTH	AF_BLUETOOTH
#define	PF_IEEE80211	AF_IEEE80211
#define	PF_INET_SDP	AF_INET_SDP
#define	PF_INET6_SDP	AF_INET6_SDP

#define	PF_MAX		AF_MAX

/*
 * Definitions for network related sysctl, CTL_NET.
 *
 * Second level is protocol family.
 * Third level is protocol number.
 *
 * Further levels are defined by the individual families.
 */

/*
 * PF_ROUTE - Routing table
 *
 * Three additional levels are defined:
 *	Fourth: address family, 0 is wildcard
 *	Fifth: type of info, defined below
 *	Sixth: flag(s) to mask with for NET_RT_FLAGS
 */
#define NET_RT_DUMP	1		/* dump; may limit to a.f. */
#define NET_RT_FLAGS	2		/* by flags, e.g. RESOLVING */
#define NET_RT_IFLIST	3		/* survey interface list */
#define	NET_RT_IFMALIST	4		/* return multicast address list */
#define	NET_RT_IFLISTL	5		/* Survey interface list, using 'l'en
					 * versions of msghdr structs. */
#define NET_RT_NHOP	6		/* dump routing nexthops */
#define NET_RT_NHGRP	7		/* dump routing nexthop groups */
#endif /* __BSD_VISIBLE */

/*
 * Maximum queue length specifiable by listen.
 */
#define	SOMAXCONN	128

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 */
struct msghdr {
	void		*msg_name;		/* optional address */
	socklen_t	 msg_namelen;		/* size of address */
	struct iovec	*msg_iov;		/* scatter/gather array */
	int		 msg_iovlen;		/* # elements in msg_iov */
	void		*msg_control;		/* ancillary data, see below */
	socklen_t	 msg_controllen;	/* ancillary data buffer len */
	int		 msg_flags;		/* flags on received message */
};

#define	MSG_OOB		 0x00000001	/* process out-of-band data */
#define	MSG_PEEK	 0x00000002	/* peek at incoming message */
#define	MSG_DONTROUTE	 0x00000004	/* send without using routing tables */
#define	MSG_EOR		 0x00000008	/* data completes record */
#define	MSG_TRUNC	 0x00000010	/* data discarded before delivery */
#define	MSG_CTRUNC	 0x00000020	/* control data lost before delivery */
#define	MSG_WAITALL	 0x00000040	/* wait for full request or error */
#if __BSD_VISIBLE
#define	MSG_DONTWAIT	 0x00000080	/* this message should be nonblocking */
#define	MSG_EOF		 0x00000100	/* data completes connection */
/*			 0x00000200	   unused */
/*			 0x00000400	   unused */
/*			 0x00000800	   unused */
/*			 0x00001000	   unused */
#define	MSG_NOTIFICATION 0x00002000	/* SCTP notification */
#define	MSG_NBIO	 0x00004000	/* FIONBIO mode, used by fifofs */
#define	MSG_COMPAT       0x00008000		/* used in sendit() */
#endif
#if __POSIX_VISIBLE >= 200809
#define	MSG_NOSIGNAL	 0x00020000	/* do not generate SIGPIPE on EOF */
#endif
#if __BSD_VISIBLE
#define	MSG_CMSG_CLOEXEC 0x00040000	/* make received fds close-on-exec */
#define	MSG_WAITFORONE	 0x00080000	/* for recvmmsg() */
#endif

/*
 * Header for ancillary data objects in msg_control buffer.
 * Used for additional information with/about a datagram
 * not expressible by flags.  The format is a sequence
 * of message elements headed by cmsghdr structures.
 */
struct cmsghdr {
	socklen_t	cmsg_len;		/* data byte count, including hdr */
	int		cmsg_level;		/* originating protocol */
	int		cmsg_type;		/* protocol-specific type */
/* followed by	u_char  cmsg_data[]; */
};

#if __BSD_VISIBLE
/*
 * While we may have more groups than this, the cmsgcred struct must
 * be able to fit in an mbuf and we have historically supported a
 * maximum of 16 groups.
*/
#define CMGROUP_MAX 16

/*
 * Credentials structure, used to verify the identity of a peer
 * process that has sent us a message. This is allocated by the
 * peer process but filled in by the kernel. This prevents the
 * peer from lying about its identity. (Note that cmcred_groups[0]
 * is the effective GID.)
 */
struct cmsgcred {
	pid_t	cmcred_pid;		/* PID of sending process */
	uid_t	cmcred_uid;		/* real UID of sending process */
	uid_t	cmcred_euid;		/* effective UID of sending process */
	gid_t	cmcred_gid;		/* real GID of sending process */
	short	cmcred_ngroups;		/* number or groups */
	gid_t	cmcred_groups[CMGROUP_MAX];	/* groups */
};

/*
 * Socket credentials (LOCAL_CREDS).
 */
struct sockcred {
	uid_t	sc_uid;			/* real user id */
	uid_t	sc_euid;		/* effective user id */
	gid_t	sc_gid;			/* real group id */
	gid_t	sc_egid;		/* effective group id */
	int	sc_ngroups;		/* number of supplemental groups */
	gid_t	sc_groups[1];		/* variable length */
};

/*
 * Compute size of a sockcred structure with groups.
 */
#define	SOCKCREDSIZE(ngrps) \
	(sizeof(struct sockcred) + (sizeof(gid_t) * ((ngrps) - 1)))

#include <sys/cdefs.h>

__BEGIN_DECLS
int	accept(int, struct sockaddr * __restrict, socklen_t * __restrict);
int	bind(int, const struct sockaddr *, socklen_t);
int	connect(int, const struct sockaddr *, socklen_t);
#if __BSD_VISIBLE
int	accept4(int, struct sockaddr * __restrict, socklen_t * __restrict, int);
int	bindat(int, int, const struct sockaddr *, socklen_t);
int	connectat(int, int, const struct sockaddr *, socklen_t);
#endif
int	getpeername(int, struct sockaddr * __restrict, socklen_t * __restrict);
int	getsockname(int, struct sockaddr * __restrict, socklen_t * __restrict);
int	getsockopt(int, int, int, void * __restrict, socklen_t * __restrict);
int	listen(int, int);
ssize_t	recv(int, void *, size_t, int);
ssize_t	recvfrom(int, void *, size_t, int, struct sockaddr * __restrict, socklen_t * __restrict);
ssize_t	recvmsg(int, struct msghdr *, int);
#if __BSD_VISIBLE
struct timespec;
ssize_t	recvmmsg(int, struct mmsghdr * __restrict, size_t, int,
    const struct timespec * __restrict);
#endif
ssize_t	send(int, const void *, size_t, int);
ssize_t	sendto(int, const void *,
	    size_t, int, const struct sockaddr *, socklen_t);
ssize_t	sendmsg(int, const struct msghdr *, int);
#if __BSD_VISIBLE
int	sendfile(int, int, off_t, size_t, struct sf_hdtr *, off_t *, int);
ssize_t	sendmmsg(int, struct mmsghdr * __restrict, size_t, int);
int	setfib(int);
#endif
int	setsockopt(int, int, int, const void *, socklen_t);
int	shutdown(int, int);
int	sockatmark(int);
int	socket(int, int, int);
int	socketpair(int, int, int, int *);
__END_DECLS

#endif /* !_KERNEL */

// From rtems netdb.h

/*
 * Structures returned by network data base library.  All addresses are
 * supplied in host order, and returned in network order (suitable for
 * use in system calls).
 */
struct hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatibility */
};

struct netent {
	char		*n_name;	/* official name of net */
	char		**n_aliases;	/* alias list */
	int		n_addrtype;	/* net address type */
	uint32_t	n_net;		/* network # */
};

struct servent {
	char	*s_name;	/* official service name */
	char	**s_aliases;	/* alias list */
	int	s_port;		/* port # */
	char	*s_proto;	/* protocol to use */
};

struct protoent {
	char	*p_name;	/* official protocol name */
	char	**p_aliases;	/* alias list */
	int	p_proto;	/* protocol # */
};

struct hostent	*gethostbyaddr(const void *, socklen_t, int);
struct hostent	*gethostbyname(const char *);
struct hostent	*gethostent(void);
struct netent	*getnetbyaddr(uint32_t, int);
struct netent	*getnetbyname(const char *);
struct netent	*getnetent(void);
struct protoent	*getprotobyname(const char *);
struct protoent	*getprotobynumber(int);
struct protoent	*getprotoent(void);
struct servent	*getservbyname(const char *, const char *);
struct servent	*getservbyport(int, const char *);
struct servent	*getservent(void);

#endif // FRACTAL_LIBC_SOCKET_H
