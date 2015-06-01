/*
 * Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 * This code is public domain.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if.h>

#include <netinet/in.h>

#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <getopt.h>
#include <unistd.h>
#include <signal.h>

#define EXC_OK 0
#define EXC_USAGE	1
#define EXC_SYS		2
#define EXC_RECV	3
#define EXC_NORECV	4

/* log facility */
#define LOG_ERROR	0x01
#define LOG_WARNING	0x02
#define LOG_INFO	0x03
#define LOG_DEBUG	0x04

#define LOGGER_NAME "arpsend: "
int debug_level = LOG_INFO;

#define logger(level, fmt, args...) \
	(debug_level < level ? (void)0 : fprintf(stderr, LOGGER_NAME fmt "\n", ##args))

#define IP_ADDR_LEN 4

#define REQUEST ARPOP_REQUEST
#define REPLY ARPOP_REPLY

struct arp_packet {
	u_char targ_hw_addr[ETH_ALEN];	/* ethernet destination MAC address */
	u_char src_hw_addr[ETH_ALEN];	/* ethernet source MAC address */
	u_short frame_type;			/* ethernet packet type */

	/* ARP packet */
	u_short hw_type;			/* hardware adress type (ethernet) */
	u_short prot_type;			/* resolve protocol address type (ip) */
	u_char hw_addr_size;
	u_char prot_addr_size;
	u_short op;				/* subtype */
	u_char sndr_hw_addr[ETH_ALEN];
	u_char sndr_ip_addr[IP_ADDR_LEN];
	u_char rcpt_hw_addr[ETH_ALEN];
	u_char rcpt_ip_addr[IP_ADDR_LEN];

	/* ethernet padding */
	u_char padding[18];
};

enum {
	AR_NOTHING = 0,
	AR_UPDATE,
	AR_DETECT,
	AR_REQUEST,
	AR_REPLY
} cmd = AR_NOTHING;

char* iface = NULL;

int timeout = 1;
int count = -1;

char *ip6_addr;

/* source MAC address in Ethernet header */
unsigned char src_hwaddr[ETH_ALEN];
int src_hwaddr_flag = 0;
/* target MAC address in Ethernet header */
unsigned char trg_hwaddr[ETH_ALEN];
int trg_hwaddr_flag = 0;
/* source MAC address in ARP header */
unsigned char src_arp_hwaddr[ETH_ALEN];
int src_arp_hwaddr_flag = 0;
/* target MAC address in ARP header */
unsigned char trg_arp_hwaddr[ETH_ALEN];
int trg_arp_hwaddr_flag = 0;

struct in_addr src_ipaddr;
int src_ipaddr_flag = 0;

int at_once = 0;
int force = 0;

#define MAX_IPADDR_NUMBER	1024
struct in_addr trg_ipaddr[MAX_IPADDR_NUMBER];
int trg_ipaddr_count = 0;
int trg_ipaddr_flag = 0;

const unsigned char broadcast[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
struct in_addr zero = {0x00000000};

struct sockaddr_ll iaddr;

char* print_hw_addr(const u_char* addr);
char* print_ip_addr(const u_char* addr);
char* print_arp_packet(struct arp_packet* pkt);
int read_hw_addr(u_char* buf, const char* str);
int read_ip_addr(struct in_addr* in_addr, const char* str);

char* programm_name = NULL;

void usage(void)
{
	fprintf(stderr, "Usage: %s <-U -i <src_ip_addr> | -D -e <trg_ip_addr> "
		"[-e <trg_ip_addr>] ...> [-c <count>] [-w <timeout>] "
		"interface_name\n", programm_name);
	exit(EXC_USAGE);
}

void parse_options (int argc, char **argv)
{
#define read_hw(addr)						\
	({							\
		if (!strcmp(optarg, "broadcast"))		\
			memcpy(addr, broadcast, ETH_ALEN);	\
		else if (read_hw_addr(addr, optarg) < 0)	\
			usage();				\
		addr##_flag = 1;				\
	})
#define read_ip(addr)					\
	({						\
		if (read_ip_addr(&addr, optarg) < 0)	\
			usage();			\
		addr##_flag = 1;			\
	})

	int c;
	static char short_options[] = "UDQPc:w:s:t:S:T:i:e:ovf";
	static struct option long_options[] =
	{
		{"update", 0, NULL, 'U'},
		{"detect", 0, NULL, 'D'},
		{"request", 0, NULL, 'Q'},
		{"reply", 0, NULL, 'P'},
		{"count", 1, NULL, 'c'},
		{"timeout", 1, NULL, 'w'},
		{"src-hw", 1, NULL, 's'},
		{"trg-hw", 1, NULL, 't'},
		{"src-arp", 1, NULL, 'S'},
		{"trg-arp", 1, NULL, 'T'},
		{"src-ip", 1, NULL, 'i'},
		{"trg-ip", 1, NULL, 'e'},
		{"at-once", 0, NULL, 'o'},
		{"force", 0, NULL, 'f'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
	{
		switch (c)
		{
		case 'U':
			if (cmd)
				usage();
			cmd = AR_UPDATE;
			break;
		case 'D':
			if (cmd)
				usage();
			cmd = AR_DETECT;
			break;
		case 'Q':
			if (cmd)
				usage();
			cmd = AR_REQUEST;
			break;
		case 'P':
			if (cmd)
				usage();
			cmd = AR_REPLY;
			break;
		case 'c':
			count = atoi(optarg);
			if (!count)
				usage();
			break;
		case 'w':
			timeout = atoi(optarg);
			break;
		case 's':
			read_hw(src_hwaddr);
			break;
		case 't':
			read_hw(trg_hwaddr);
			break;
		case 'S':
			read_hw(src_arp_hwaddr);
			break;
		case 'T':
			read_hw(trg_arp_hwaddr);
			break;
		case 'i':
			if (strchr(optarg, ':')) {
				ip6_addr = optarg;
				src_ipaddr_flag = 1;
				break;
			}
			read_ip(src_ipaddr);
			break;
		case 'e':
			if (strchr(optarg, ':')) {
				trg_ipaddr_flag = 1;
				ip6_addr = optarg;
				break;
			}
			if (trg_ipaddr_count >= MAX_IPADDR_NUMBER)
				usage();
			if (read_ip_addr(&trg_ipaddr[trg_ipaddr_count++], optarg) < 0)
				usage();
			trg_ipaddr_flag = 1;
			break;
		case 'o':
			at_once = 1;
			break;
		case 'v':
			debug_level ++ ;
			break;
		case 'f':
			force = 1;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (cmd == AR_NOTHING || argc != 1
	    || (cmd == AR_DETECT && !trg_ipaddr_flag)
	    || (cmd == AR_UPDATE && !src_ipaddr_flag))
		usage();

	iface = argv[0];

	/* user have to choose something */
	if (!cmd)
		usage();

	if (cmd == AR_DETECT
	    && trg_ipaddr_count <= 1) {
		/* for detection with no more then one target
		   we exit on first reply */
		at_once = 1;
	}
}

#define COMMON_ARP					\
	frame_type :		htons(ETH_P_ARP),	\
	hw_type :		htons(ARPHRD_ETHER),	\
	prot_type :		htons(ETH_P_IP),	\
	hw_addr_size :		ETH_ALEN,		\
	prot_addr_size :	IP_ADDR_LEN

u_char real_hwaddr[ETH_ALEN];
struct in_addr real_ipaddr;

int init_device_addresses(int sock, const char* device)
{
	struct ifreq ifr;
	int ifindex;
	short int ifflags;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, device, IFNAMSIZ-1);

	if (ioctl(sock, SIOCGIFINDEX, &ifr) != 0) {
		logger(LOG_ERROR, "Unknown network interface %s: %m", device);
		return -1;
	}
	ifindex = ifr.ifr_ifindex;

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
		logger(LOG_ERROR, "ioctl(SIOCGIFFLAGS) function "
				"returned error: %m");
		return -1;
	}
	ifflags = ifr.ifr_flags;

	if (!(ifflags & IFF_UP)) {
		logger(LOG_ERROR, "Network interface %s is down", device);
		return -1;
	}

	if (ifflags & (IFF_NOARP|IFF_LOOPBACK)) {
		logger(LOG_ERROR, "Network interface %s is not ARPable",
				device);
		return -1;
	}

	if (ifflags & IFF_SLAVE) {
		logger(LOG_ERROR, "Network interface %s is a slave device",
				device);
		return -1;
	}

	/* get interface HW address */
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) != 0) {
		logger(LOG_ERROR, "Cannot get the MAC address of "
				"network interface %s: %m", device);
		return -1;
	}
	memcpy(real_hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	if (ioctl(sock, SIOCGIFADDR, &ifr)) {
		if (!force) {
			logger(LOG_ERROR, "Cannot get the IP address of "
					"network interface %s: %m", device);
			return -1;
		}
	}
	memcpy(&real_ipaddr, ifr.ifr_addr.sa_data + 2, IP_ADDR_LEN);

	logger(LOG_DEBUG, "Retreived addresses: MAC address='%s', "
			"IP address='%s'",
		print_hw_addr(real_hwaddr),
		print_ip_addr((u_char*) &real_ipaddr));

	/* fill interface description struct */
	iaddr.sll_ifindex = ifindex;
	iaddr.sll_family = AF_PACKET;
	iaddr.sll_protocol = htons(ETH_P_ARP);
	memcpy(iaddr.sll_addr, real_hwaddr, iaddr.sll_halen = ETH_ALEN);

	return 0;
}

int sock;
/* sent packet */
struct arp_packet pkt;

void create_arp_packet(struct arp_packet* pkt)
{
#define set_ip(to, from) (memcpy(to, from, IP_ADDR_LEN))
#define set_hw(to, from) (memcpy(to, from, ETH_ALEN))
#define check(check, two) (check##_flag ? check : two)

	*pkt = ((struct arp_packet)
		{COMMON_ARP, op : htons(cmd == AR_REPLY ? REPLY : REQUEST)});

	set_ip(pkt->sndr_ip_addr,
		(src_ipaddr_flag ? &src_ipaddr : &real_ipaddr));

	set_hw(pkt->targ_hw_addr, check(trg_hwaddr, broadcast));
	set_hw(pkt->src_hw_addr, check(src_hwaddr, real_hwaddr));

	set_hw(pkt->rcpt_hw_addr, check(trg_arp_hwaddr, pkt->targ_hw_addr));
	set_hw(pkt->sndr_hw_addr, check(src_arp_hwaddr, pkt->src_hw_addr));

	/* special case 'cause we support multiple receipient ip addresses
	   we 'll setup 'pkt.rcpt_ip_addr' separately for each specified
	   (-e option) 'trg_ipaddr'
	 */
	if (trg_ipaddr_flag) {
		/* at least one trg_ipaddr is specified */
		set_ip(pkt->rcpt_ip_addr, &trg_ipaddr[0]);
	} else {
		set_ip(pkt->rcpt_ip_addr, &real_ipaddr);
		/* set 'trg_ipaddr' for checking incomming packets */
		set_ip(&trg_ipaddr[0], &real_ipaddr);
		trg_ipaddr_count = 1;
	}
#undef set_ip
}

void set_trg_ipaddr(struct arp_packet* pkt, const struct in_addr ipaddr)
{
#define set_ip(to, from) (memcpy(to, &from, IP_ADDR_LEN))
	set_ip(pkt->rcpt_ip_addr, ipaddr);
#undef set_ip
}

int recv_response = 0;

void finish(void)
{
	switch (cmd)
	{
	case AR_DETECT:
	case AR_UPDATE:
		exit((recv_response) ? EXC_RECV : EXC_OK);
	case AR_REQUEST:
		exit((recv_response) ? EXC_OK : EXC_NORECV);
	case AR_REPLY:
		exit(EXC_OK);
	default:
		exit(EXC_SYS);
	}
}

int recv_pack(unsigned char *buf, int len, struct sockaddr_ll *from)
{
	int rc = -1;
	int i;
	struct arp_packet * recv_pkt = (struct arp_packet*) buf;

	if (recv_pkt->frame_type != htons(ETH_P_ARP)) {
		logger(LOG_ERROR, "Unknown network interface type: %#x",
				recv_pkt->frame_type);
		goto out;
	}

	if (recv_pkt->op != htons(REQUEST)
	    && recv_pkt->op != htons(REPLY)) {
		logger(LOG_ERROR, "Unknown ARP packet type: %#x",
				recv_pkt->op);
		goto out;
	}

	for (i = 0; i < trg_ipaddr_count; i ++) {
		// check for all sent packets addresses
		if (memcmp(&trg_ipaddr[i], recv_pkt->sndr_ip_addr, IP_ADDR_LEN))
			continue;
		logger(LOG_DEBUG, "Received the packet: %s",
				print_arp_packet(recv_pkt));
		logger(LOG_INFO, "%s is detected on another computer: %s",
			print_ip_addr((u_char*) &trg_ipaddr[i]),
			print_hw_addr(recv_pkt->sndr_hw_addr));
		recv_response++;
		/* finish on first response */
		if (at_once)
			finish();
	}

	/* unknown packet */
	logger(LOG_DEBUG, "Received an unknown packet: %s",
			print_arp_packet(recv_pkt));
	rc = 0;
out:
	return rc;
}

void sender(void)
{
	int i;

	if (count-- == 0)
		finish();

	/* send multiple packets */
	for (i = 0; i < trg_ipaddr_count; i ++) {
		set_trg_ipaddr(&pkt, trg_ipaddr[i]);
		logger(LOG_DEBUG, "Send the packet: %s",
				print_arp_packet(&pkt));
		if(sendto(sock, &pkt,sizeof(pkt), 0,
			  (struct sockaddr*) &iaddr, sizeof(iaddr)) < 0) {
			logger(LOG_ERROR, "sendto function returned error: %m");
			exit(EXC_SYS);
		}
	}

	if (count == 0
	    && cmd != AR_DETECT
	    && cmd != AR_REQUEST)
		finish();

	alarm(timeout);
}

void set_signal(int signo, void (*handler)(void))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = (void (*)(int))handler;
	sa.sa_flags = SA_RESTART;
	sigaction(signo, &sa, NULL);
}

int main(int argc,char** argv)
{
	sigset_t block_alarm;
	programm_name = argv[0];
	parse_options (argc, argv);

	if (ip6_addr) {
		if (cmd == AR_UPDATE)
			execlp("/usr/sbin/ndsend", "ndsend",
					ip6_addr, iface, NULL);
		exit(0);
	}

	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (sock < 0)
	{
		logger(LOG_ERROR, "socket function returned error: %m");
		exit(EXC_SYS);
	}

	if (init_device_addresses(sock, iface) < 0)
		exit(EXC_SYS);

	create_arp_packet(&pkt);
	set_signal(SIGALRM, sender);
	sigemptyset(&block_alarm);
	sigaddset(&block_alarm, SIGALRM);
	sender();

	while(1)
	{
		u_char packet[4096];
		struct sockaddr_ll from;
		socklen_t alen = sizeof(from);
		int cc;

		cc = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *)&from, &alen);
		if (cc < 0)
		{
			logger(LOG_ERROR, "recvfrom function "
					"returned error: %m");
			continue;
		}
		sigprocmask (SIG_BLOCK, &block_alarm, NULL);
		recv_pack(packet, cc, &from);
		sigprocmask (SIG_UNBLOCK, &block_alarm, NULL);
	}

	exit(EXC_OK);
}

static char* get_buf(void)
{
	static int num = 0;
	static char buf[10][1024];
	return buf[num = (num+1) % 10];
}

char* print_arp_packet(struct arp_packet* pkt)
{
	char* point = get_buf();
	sprintf(point, "eth '%s' -> eth '%s'; arp sndr '%s' '%s'; %s; arp recipient '%s' '%s'",
		print_hw_addr(pkt->src_hw_addr),
		print_hw_addr(pkt->targ_hw_addr),
		print_hw_addr(pkt->sndr_hw_addr),
		print_ip_addr(pkt->sndr_ip_addr),
		(pkt->op == htons(REQUEST)) ? "request" : (pkt->op == htons(REPLY) ? "reply" : "unknown"),
		print_hw_addr(pkt->rcpt_hw_addr),
		print_ip_addr(pkt->rcpt_ip_addr));
	return point;
}

/* get MAC address in user form */
char* print_hw_addr(const u_char* addr)
{
	int i;
	char* point = get_buf();
	char* current = point;

	for (i = 0; i < ETH_ALEN; i++)
		point += sprintf(point, i == (ETH_ALEN-1) ? "%02x" : "%02x:",
			addr[i]);

	return current;
}

/* get IP address in user form */
char* print_ip_addr(const u_char* addr)
{
	int i;
	char* point = get_buf();
	char* current = point;

	for (i = 0; i < IP_ADDR_LEN; i++)
		point += sprintf(point, i == (IP_ADDR_LEN-1) ? "%u" : "%u.",
			addr[i]);

	return current;
}

/* trasform user -> computer freindly MAC address */
int read_hw_addr(u_char* buf, const char* str)
{
	int rc = -1;
	int i;

	for(i = 0; i < 2*ETH_ALEN; i++)
	{
		char c, val;

		c = tolower(*str++);
		if(!c)
			goto out;

		if(isdigit(c))
			val = c - '0';
		else if (c >= 'a' && c <= 'f')
			val = c - 'a' + 10;
		else
			goto out;

		if (i % 2)
		{
			*buf++ |= val;
			if(*str == ':')
				str++;
		}
		else
			*buf = val << 4;
	}
	rc = 0;
out:
	return rc;
}

/* trasform user -> computer freindly IP address */
int read_ip_addr(struct in_addr* in_addr, const char* str)
{
	in_addr->s_addr=inet_addr(str);
	if(in_addr->s_addr == -1)
		return -1;
	return 0;
}
