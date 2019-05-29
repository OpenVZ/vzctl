/*
 * Copyright (c) 1999-2017, Parallels International GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
 *
 * This file is part of OpenVZ. OpenVZ is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef _LINUX_BEANCOUNTER_H
#define _LINUX_BEANCOUNTER_H

/*
 * Generic ratelimiting stuff.
 */

struct ub_rate_info {
	int burst;
	int interval; /* jiffy_t per event */
	int bucket; /* kind of leaky bucket */
	unsigned long last; /* last event */
};

/* Return true if rate limit permits. */
int ub_ratelimit(struct ub_rate_info *);


/*
 * This magic is used to distinuish user beancounter and pages beancounter
 * in struct page. page_ub and page_bc are placed in union and MAGIC
 * ensures us that we don't use pbc as ubc in ub_page_uncharge().
 */
#define UB_MAGIC		0x62756275

/*
 *	Resource list.
 */

#define UB_KMEMSIZE	0	/* Unswappable kernel memory size including
				 * struct task, page directories, etc.
				 */
#define UB_LOCKEDPAGES	1	/* Mlock()ed pages. */
#define UB_PRIVVMPAGES	2	/* Total number of pages, counting potentially
				 * private pages as private and used.
				 */
#define UB_SHMPAGES	3	/* IPC SHM segment size. */
#define UB_DUMMY	4	/* Dummy resource (compatibility) */
#define UB_NUMPROC	5	/* Number of processes. */
#define UB_PHYSPAGES	6	/* All resident pages, for swapout guarantee. */
#define UB_VMGUARPAGES	7	/* Guarantee for memory allocation,
				 * checked against PRIVVMPAGES.
				 */
#define UB_OOMGUARPAGES	8	/* Guarantees against OOM kill.
				 * Only limit is used, no accounting.
				 */
#define UB_NUMTCPSOCK	9	/* Number of TCP sockets. */
#define UB_NUMFLOCK	10	/* Number of file locks. */
#define UB_NUMPTY	11	/* Number of PTYs. */
#define UB_NUMSIGINFO	12	/* Number of siginfos. */
#define UB_TCPSNDBUF	13	/* Total size of tcp send buffers. */
#define UB_TCPRCVBUF	14	/* Total size of tcp receive buffers. */
#define UB_OTHERSOCKBUF	15	/* Total size of other socket
				 * send buffers (all buffers for PF_UNIX).
				 */
#define UB_DGRAMRCVBUF	16	/* Total size of other socket
				 * receive buffers.
				 */
#define UB_NUMOTHERSOCK	17	/* Number of other sockets. */
#define UB_DCACHESIZE	18	/* Size of busy dentry/inode cache. */
#define UB_NUMFILE	19	/* Number of open files. */

#define UB_RESOURCES_COMPAT	24

/* Add new resources here */

#define UB_NUMXTENT	23
#define UB_SWAPPAGES	24
#define UB_RESOURCES	25

#define UB_UNUSEDPRIVVM	(UB_RESOURCES + 0)
#define UB_TMPFSPAGES	(UB_RESOURCES + 1)
#define UB_HELDPAGES	(UB_RESOURCES + 2)

struct ubparm {
	/* 
	 * A barrier over which resource allocations are failed gracefully.
	 * If the amount of consumed memory is over the barrier further sbrk()
	 * or mmap() calls fail, the existing processes are not killed. 
	 */
	unsigned long	barrier;
	/* hard resource limit */
	unsigned long	limit;
	/* consumed resources */
	unsigned long	held;
	/* maximum amount of consumed resources through the last period */
	unsigned long	maxheld;
	/* minimum amount of consumed resources through the last period */
	unsigned long	minheld;
	/* count of failed charges */
	unsigned long	failcnt;
};

/*
 * Kernel internal part.
 */

#ifdef __KERNEL__

#include <linux/config.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/cache.h>
#include <linux/threads.h>
#include <linux/task_io_accounting.h>
#include <linux/percpu.h>
#include <ub/ub_debug.h>
#include <ub/ub_decl.h>
#include <asm/atomic.h>
#include <ub/io_prio.h>

/*
 * UB_MAXVALUE is essentially LONG_MAX declared in a cross-compiling safe form.
 */
#define UB_MAXVALUE	( (1UL << (sizeof(unsigned long)*8-1)) - 1)


/*
 *	Resource management structures
 * Serialization issues:
 *   beancounter list management is protected via ub_hash_lock
 *   task pointers are set only for current task and only once
 *   refcount is managed atomically
 *   value and limit comparison and change are protected by per-ub spinlock
 */

struct page_beancounter;
struct task_beancounter;
struct sock_beancounter;

struct page_private {
	unsigned long		ubp_unused_privvmpages;
	unsigned long		ubp_tmpfs_respages;
	long long		ubp_held_pages;
};

struct sock_private {
	unsigned long		ubp_rmem_thres;
	unsigned long		ubp_wmem_pressure;
	unsigned long		ubp_maxadvmss;
	unsigned long		ubp_rmem_pressure;
	int			ubp_tw_count;
#define UB_RMEM_EXPAND          0
#define UB_RMEM_KEEP            1
#define UB_RMEM_SHRINK          2
	struct list_head	ubp_other_socks;
	struct list_head	ubp_tcp_socks;
#if 0
	atomic_t		ubp_orphan_count;
#endif
};

struct ub_percpu_struct {
	unsigned long unmap;
	unsigned long swapin;
	int pbcs;
	int dirty_pages;
#ifdef CONFIG_UBC_IO_ACCT
	unsigned long async_write_complete;
	unsigned long async_write_canceled;
	unsigned long long sync_write_bytes;
	unsigned long long sync_read_bytes;
#endif
#ifdef CONFIG_UBC_DEBUG_KMEM
	long	pages_charged;
	long	vmalloc_charged;
#endif
	unsigned long	sync;
	unsigned long	sync_done;

	unsigned long	fsync;
	unsigned long	fsync_done;

	unsigned long	fdsync;
	unsigned long	fdsync_done;

	unsigned long	frsync;
	unsigned long	frsync_done;

	unsigned long		write;
	unsigned long		read;
	unsigned long long	wchar;
	unsigned long long	rchar;

	int		held_pages;

	int		fast_refcount;
};

struct user_beancounter
{
	unsigned long		ub_magic;
	atomic_t		ub_refcount;
	struct list_head	ub_list;
	struct hlist_node	ub_hash;

	union {
		struct rcu_head rcu;
		struct execute_work cleanup;
	};

	spinlock_t		ub_lock;
	uid_t			ub_uid;
	unsigned int		ub_cookie;

	struct ub_rate_info	ub_limit_rl;
	int			ub_oom_noproc;

	atomic_long_t		pbcs;

	atomic_long_t		dirty_pages;

	struct page_private	ppriv;
#define ub_unused_privvmpages	ppriv.ubp_unused_privvmpages
#define ub_tmpfs_respages	ppriv.ubp_tmpfs_respages
#define ub_held_pages		ppriv.ubp_held_pages
	struct sock_private	spriv;
#define ub_rmem_thres		spriv.ubp_rmem_thres
#define ub_maxadvmss		spriv.ubp_maxadvmss
#define ub_rmem_pressure	spriv.ubp_rmem_pressure
#define ub_wmem_pressure	spriv.ubp_wmem_pressure
#define ub_tcp_sk_list		spriv.ubp_tcp_socks
#define ub_other_sk_list	spriv.ubp_other_socks
#define ub_orphan_count		spriv.ubp_orphan_count
#define ub_tw_count		spriv.ubp_tw_count
#ifdef CONFIG_UBC_IO_PRIO
	struct ub_iopriv	iopriv;
#endif

	struct user_beancounter *parent;
	int			ub_childs;
	void			*private_data;
	unsigned long		ub_aflags;

	void			*private_data2;

#ifdef CONFIG_PROC_FS
	struct proc_dir_entry	*proc;
#endif
	unsigned long		ub_mem_size;
	int			dirty_exceeded;

	/* resources statistic and settings */
	struct ubparm		ub_parms[UB_RESOURCES];
	/* resources statistic for last interval */
	struct ubparm		ub_store[UB_RESOURCES];

	struct ub_percpu_struct	*ub_percpu;
#ifdef CONFIG_UBC_DEBUG_KMEM
	struct list_head	ub_cclist;
#endif
	atomic_t		ub_fastcount;
};

extern int ub_count;

enum ub_severity { UB_HARD, UB_SOFT, UB_FORCE };

#define UB_AFLAG_NOTIF_PAGEIN	0

static inline
struct user_beancounter *top_beancounter(struct user_beancounter *ub)
{
	while (ub->parent != NULL)
		ub = ub->parent;
	return ub;
}

static inline int ub_barrier_hit(struct user_beancounter *ub, int resource)
{
	return ub->ub_parms[resource].held > ub->ub_parms[resource].barrier;
}

static inline int ub_hfbarrier_hit(struct user_beancounter *ub, int resource)
{
	return (ub->ub_parms[resource].held > 
		((ub->ub_parms[resource].barrier) >> 1));
}

static inline int ub_barrier_farnr(struct user_beancounter *ub, int resource)
{
	struct ubparm *p;
	p = ub->ub_parms + resource;
	return p->held <= (p->barrier >> 3);
}

static inline int ub_barrier_farsz(struct user_beancounter *ub, int resource)
{
	struct ubparm *p;
	p = ub->ub_parms + resource;
	return p->held <= (p->barrier >> 3) && p->barrier >= 1024 * 1024;
}

static inline struct user_beancounter *switch_exec_ub(
		struct user_beancounter *ub)
{
	return ub ? set_exec_ub(ub) : NULL;
}

#ifndef CONFIG_USER_RESOURCE

#define ub_percpu_add(ub, f, v)	do { } while (0)
#define ub_percpu_sub(ub, f, v)	do { } while (0)
#define ub_percpu_inc(ub, f)	do { } while (0)
#define ub_percpu_dec(ub, f)	do { } while (0)

#define mm_ub(mm)	(NULL)

extern inline struct user_beancounter *get_beancounter_byuid
		(uid_t uid, int create) { return NULL; }
extern inline struct user_beancounter *get_beancounter
		(struct user_beancounter *ub) { return NULL; }
extern inline void put_beancounter(struct user_beancounter *ub) { }

static inline void ub_init_late(void) { };
static inline void ub_init_early(void) { };

static inline int charge_beancounter(struct user_beancounter *ub,
			int resource, unsigned long val,
			enum ub_severity strict) { return 0; }
static inline void uncharge_beancounter(struct user_beancounter *ub,
			int resource, unsigned long val) { }

#else /* CONFIG_USER_RESOURCE */

#define ub_percpu(ub, cpu) (per_cpu_ptr((ub)->ub_percpu, (cpu)))

#define __ub_percpu_sum(ub, field)	({			\
		struct user_beancounter *__ub = (ub);		\
		typeof(ub_percpu(__ub, 0)->field) __sum = 0;	\
		int __cpu;					\
		for_each_possible_cpu(__cpu)			\
			__sum += ub_percpu(__ub, __cpu)->field;	\
		__sum;						\
	})

#define ub_percpu_sum(ub, field) max(0l, __ub_percpu_sum(ub, field))

#define ub_percpu_add(ub, field, v)		do {			\
		per_cpu_ptr(ub->ub_percpu, get_cpu())->field += (v);	\
		put_cpu();						\
	} while (0)
#define ub_percpu_inc(ub, field) ub_percpu_add(ub, field, 1)

#define ub_percpu_sub(ub, field, v)		do {			\
		per_cpu_ptr(ub->ub_percpu, get_cpu())->field -= (v);	\
		put_cpu();						\
	} while (0)
#define ub_percpu_dec(ub, field) ub_percpu_sub(ub, field, 1)

#define UB_STAT_BATCH	64

#define ub_stat_add(ub, name, val)	do {		\
	unsigned long __flags;				\
	int *__pcpu;					\
							\
	local_irq_save(__flags);			\
	__pcpu = &(per_cpu_ptr((ub)->ub_percpu, smp_processor_id())->name); \
	if (*__pcpu + (val) <= UB_STAT_BATCH)		\
		*__pcpu += val;				\
	else {						\
		atomic_long_add(*__pcpu + (val), &(ub)->name);	\
		*__pcpu = 0;				\
	}						\
	local_irq_restore(__flags);			\
} while (0)

#define ub_stat_sub(ub, name, val)	do {		\
	unsigned long __flags;				\
	int *__pcpu;					\
							\
	local_irq_save(__flags);			\
	__pcpu = &(per_cpu_ptr((ub)->ub_percpu, smp_processor_id())->name); \
	if (*__pcpu - (val) >= -UB_STAT_BATCH)		\
		*__pcpu -= val;				\
	else {						\
		atomic_long_add(*__pcpu - (val), &(ub)->name);	\
		*__pcpu = 0;				\
	}						\
	local_irq_restore(__flags);			\
} while (0)

#define ub_stat_flush_pcpu(ub, name)	do {		\
	unsigned long __flags;				\
	int *__pcpu;					\
							\
	local_irq_save(__flags);			\
	__pcpu = &(per_cpu_ptr((ub)->ub_percpu, smp_processor_id())->name); \
	atomic_long_add(*__pcpu, &(ub)->name);		\
	*__pcpu = 0;					\
	local_irq_restore(__flags);			\
} while (0)

#define ub_stat_inc(ub, name)		ub_stat_add(ub, name, 1)
#define ub_stat_dec(ub, name)		ub_stat_sub(ub, name, 1)
#define ub_stat_mod(ub, name, val)	atomic_long_add(val, &(ub)->name)
#define __ub_stat_get(ub, name)		atomic_long_read(&(ub)->name)
#define ub_stat_get(ub, name)		max(0l, atomic_long_read(&(ub)->name))
#define ub_stat_get_exact(ub, name)	max(0l, __ub_stat_get(ub, name) + __ub_percpu_sum(ub, name))

#define mm_ub(mm)	((mm)->mm_ub)
/*
 *  Charge/uncharge operations
 */

extern int __charge_beancounter_locked(struct user_beancounter *ub,
		int resource, unsigned long val, enum ub_severity strict);

extern void __uncharge_beancounter_locked(struct user_beancounter *ub,
		int resource, unsigned long val);

extern void put_beancounter_safe(struct user_beancounter *ub);
extern void __put_beancounter(struct user_beancounter *ub);

extern void uncharge_warn(struct user_beancounter *ub, int resource,
		unsigned long val, unsigned long held);

extern const char *ub_rnames[];
/*
 *	Put a beancounter reference
 */

static inline void put_beancounter(struct user_beancounter *ub)
{
	if (unlikely(ub == NULL))
		return;

	/* FIXME - optimize not to disable interrupts and make call */
	__put_beancounter(ub);
}

static inline
struct user_beancounter *get_beancounter_fast(struct user_beancounter *ub)
{
	if (unlikely(ub == NULL))
		return NULL;

	preempt_disable();
	if (likely(atomic_read(&ub->ub_fastcount) == 0))
		per_cpu_ptr(ub->ub_percpu, smp_processor_id())->fast_refcount++;
	else
		atomic_inc(&ub->ub_fastcount);
	preempt_enable();

	return ub;
}

static inline void put_beancounter_fast(struct user_beancounter *ub)
{
	if (unlikely(ub == NULL))
		return;

	preempt_disable();
	if (likely(atomic_read(&ub->ub_fastcount) == 0))
		per_cpu_ptr(ub->ub_percpu, smp_processor_id())->fast_refcount--;
	else if (atomic_dec_and_test(&ub->ub_fastcount))
		__put_beancounter(ub);
	preempt_enable();
}

/* fast put, refcount can't reach zero */
static inline void __put_beancounter_batch(struct user_beancounter *ub, int n)
{
	atomic_sub(n, &ub->ub_refcount);
}

static inline void put_beancounter_batch(struct user_beancounter *ub, int n)
{
	if (n > 1)
		__put_beancounter_batch(ub, n - 1);
	__put_beancounter(ub);
}

/*
 *	Create a new beancounter reference
 */
extern struct user_beancounter *get_beancounter_byuid(uid_t uid, int create);

static inline 
struct user_beancounter *get_beancounter(struct user_beancounter *ub)
{
	if (unlikely(ub == NULL))
		return NULL;

	atomic_inc(&ub->ub_refcount);
	return ub;
}

static inline 
struct user_beancounter *get_beancounter_rcu(struct user_beancounter *ub)
{
	return atomic_inc_not_zero(&ub->ub_refcount) ? ub : NULL;
}

static inline void get_beancounter_batch(struct user_beancounter *ub, int n)
{
	atomic_add(n, &ub->ub_refcount);
}

/* UB_CREATE* are bit masks */
#define UB_CREATE		1
#define UB_CREATE_ATOMIC	2
extern struct user_beancounter *get_subbeancounter_byid(
		struct user_beancounter *,
		int id, int create);

extern void ub_init_late(void);
extern void ub_init_early(void);

extern int print_ub_uid(struct user_beancounter *ub, char *buf, int size);

/*
 *	Resource charging
 * Change user's account and compare against limits
 */

static inline void ub_adjust_maxheld(struct user_beancounter *ub, int resource)
{
	if (ub->ub_parms[resource].maxheld < ub->ub_parms[resource].held)
		ub->ub_parms[resource].maxheld = ub->ub_parms[resource].held;
	if (ub->ub_parms[resource].minheld > ub->ub_parms[resource].held)
		ub->ub_parms[resource].minheld = ub->ub_parms[resource].held;
}

int charge_beancounter(struct user_beancounter *ub, int resource,
		unsigned long val, enum ub_severity strict);
void uncharge_beancounter(struct user_beancounter *ub, int resource,
		unsigned long val);
void __charge_beancounter_notop(struct user_beancounter *ub, int resource,
		unsigned long val);
void __uncharge_beancounter_notop(struct user_beancounter *ub, int resource,
		unsigned long val);

static inline void charge_beancounter_notop(struct user_beancounter *ub,
		int resource, unsigned long val)
{
	if (ub->parent != NULL)
		__charge_beancounter_notop(ub, resource, val);
}

static inline void uncharge_beancounter_notop(struct user_beancounter *ub,
		int resource, unsigned long val)
{
	if (ub->parent != NULL)
		__uncharge_beancounter_notop(ub, resource, val);
}

#define ub_mapped_pages(ub)	ub_stat_get(ub, pbcs)

void ub_flush_held_pages(struct user_beancounter *ub);

void ub_held_snapshot(struct user_beancounter *ub, unsigned long *held);

#endif /* CONFIG_USER_RESOURCE */

#ifndef CONFIG_USER_RSS_ACCOUNTING
static inline void ub_ini_pbc(void) { }
#else
extern void ub_init_pbc(void);
#endif
#endif /* __KERNEL__ */
#endif /* _LINUX_BEANCOUNTER_H */
