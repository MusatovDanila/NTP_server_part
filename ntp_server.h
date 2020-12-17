#pragma once
#ifndef  MY_NTP_SERV
#define MY_NTP_SERV

#include"C:\Users\Danila\Documents\queue_unix\queue.h"

#define MAXIMUM(a, b)   ((a) > (b) ? (a) : (b))

#define NTPD_USER       "_ntp"
#define CONFFILE        "/etc/ntpd.conf"
#define DRIFTFILE       "/var/db/ntpd.drift"
#define CTLSOCKET       "/var/run/ntpd.sock"

#define INTERVAL_QUERY_NORMAL           30      /* sync to peers every n secs */
#define INTERVAL_QUERY_PATHETIC         60
#define INTERVAL_QUERY_AGGRESSIVE       5
#define INTERVAL_QUERY_ULTRA_VIOLENCE   1       /* used at startup for auto */

#define TRUSTLEVEL_BADPEER              6
#define TRUSTLEVEL_PATHETIC             2
#define TRUSTLEVEL_AGGRESSIVE           8
#define TRUSTLEVEL_MAX                  10

#define MAX_SERVERS_DNS                 8

#define QSCALE_OFF_MIN                  0.001
#define QSCALE_OFF_MAX                  0.050

#define QUERYTIME_MAX           15      /* single query might take n secs max */
#define OFFSET_ARRAY_SIZE       8
#define SENSOR_OFFSETS          6
#define SETTIME_TIMEOUT         15      /* max seconds to wait with -s */
#define LOG_NEGLIGIBLE_ADJTIME  32      /* negligible drift to not log (ms) */
#define LOG_NEGLIGIBLE_ADJFREQ  0.05    /* negligible rate to not log (ppm) */
#define FREQUENCY_SAMPLES       8       /* samples for est. of permanent drift */
#define MAX_FREQUENCY_ADJUST    128e-5  /* max correction per iteration */
#define MAX_SEND_ERRORS         3       /* max send errors before reconnect */
#define MAX_DISPLAY_WIDTH       80      /* max chars in ctl_show report line */

#define FILTER_ADJFREQ          0x01    /* set after doing adjfreq */
#define AUTO_REPLIES            4       /* # of ntp replies we want for auto */
#define AUTO_THRESHOLD          60      /* dont bother auto setting < this */
#define INTERVAL_AUIO_DNSFAIL   1       /* DNS tmpfail interval for auto */
#define TRIES_AUTO_DNSFAIL      4       /* DNS tmpfail quick retries */


#define SENSOR_DATA_MAXAGE              (15*60)
#define SENSOR_QUERY_INTERVAL           15
#define SENSOR_QUERY_INTERVAL_SETTIME   (SETTIME_TIMEOUT/3)
#define SENSOR_SCAN_INTERVAL            (1*60)
#define SENSOR_DEFAULT_REFID            "HARD"

#define CONSTRAINT_ERROR_MARGIN         (4)
#define CONSTRAINT_RETRY_INTERVAL       (15)
#define CONSTRAINT_SCAN_INTERVAL        (15*60)
#define CONSTRAINT_SCAN_TIMEOUT         (10)
#define CONSTRAINT_MARGIN               (2.0*60)
#define CONSTRAINT_PORT                 "443"   /* HTTPS port */
#define CONSTRAINT_MAXHEADERLENGTH      8192
#define CONSTRAINT_PASSFD               (STDERR_FILENO + 1)

#define PARENT_SOCK_FILENO              CONSTRAINT_PASSFD

#define NTP_PROC_NAME                   "ntp_main"
#define NTPDNS_PROC_NAME                "ntp_dns"
#define CONSTRAINT_PROC_NAME            "constraint"

#define LI_NOWARNING    (0 << 6)        /* no warning */
#define LI_PLUSSEC      (1 << 6)        /* add a second (61 seconds) */
#define LI_MINUSSEC     (2 << 6)        /* minus a second (59 seconds) */
#define LI_ALARM        (3 << 6)        /* alarm condition */

#define MODEMASK        (7 << 0)
#define VERSIONMASK     (7 << 3)
#define LIMASK          (3 << 6)

#define MODE_RES0       0       /* reserved */
#define MODE_SYM_ACT    1       /* symmetric active */
#define MODE_SYM_PAS    2       /* symmetric passive */
#define MODE_CLIENT     3       /* client */
#define MODE_SERVER     4       /* server */
#define MODE_BROADCAST  5       /* broadcast */
#define MODE_RES1       6       /* reserved for NTP control message */
#define MODE_RES2       7       /* reserved for private use */

#define NTP_DIGESTSIZE          16
#define NTP_MSGSIZE_NOAUTH      48
#define NTP_MSGSIZE             (NTP_MSGSIZE_NOAUTH + 4 + NTP_DIGESTSIZE)

#define NTP_ERA         0

#define SECS_IN_ERA     (UINT32_MAX + 1ULL)
#define JAN_1970        2208988800UL    /* 1970 - 1900 in seconds */
#define IPTOS_LOWDELAY 16;

typedef LONG_PTR ssize_t;

struct s_fixedpt{
	uint16_t int_parts;
	uint16_t fractions;
};
struct l_fixedpt {
	uint32_t int_partl; 
	uint32_t fractionl;
};

struct ntp_msg{
	uint8_t status;
	uint8_t stratum;
	uint8_t ppoll;
	int8_t precision;
	struct s_fixedpt rootdelay;
	struct s_fixedpt dispersion;
	uint32_t refid;
	struct l_fixedpt reftime;
	struct l_fixedpt orgtime;
	struct l_fixedpt rectime;
	struct l_fixedpt xmttime;
};

struct ntp_query {
	int                     fd;
	struct ntp_msg          msg;
	double                  xmttime;
};
 
struct listen_addr {
	TAILQ_ENTRY(listen_addr)         entry;
	struct sockaddr_storage          sa;
	int                              fd;
	int                              rtable;
};

const char*			log_sockaddr(struct sockaddr*);
int					ntp_getmsg(struct sockaddr* , char *, ssize_t, struct ntp_msg*);
int					ntp_sendmsg(int , struct sockaddr *, struct ntp_msg*);
struct s_fixedpt	d_to_sfp(double);
struct l_fixedpt	d_to_lfp(double);
struct l_fixedpt 	d_to_lfp(SYSTEMTIME);
int setup_listeners(unsigned int *);
int server_dispatch(struct listen_addr*);

struct listen_addr;
TAILQ_HEAD(listen_addrs, listen_addr);
extern struct listen_addrs listen_addrs;
#endif // ! MY_NTP_SERV