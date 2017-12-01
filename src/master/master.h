#ifndef _MASTER_H_INCLUDE_
#define _MASTER_H_INCLUDE_

#include "events.h"

typedef struct MASTER_SERV {
	int     flags;          /* status, features, etc. */
	char   *name;           /* service endpoint name */
	int     type;           /* UNIX-domain, INET, etc. */
	int     wakeup_time;    /* wakeup interval */
	int 	listen_fd;   	/* incoming requests*/
	
	char 	*port;			/* inet listen port*/
	char	*host; 			/* inet listen address */

	int     max_proc;           /* upper bound on # processes */
    char   *path;           	/* command pathname */
    struct ARGV *args;          /* argument vector */

	int     avail_proc;         /* idle processes */
    int     total_proc;         /* number of processes */
	int     throttle_delay;     /* failure recovery parameter */
	int     status_fd[2];       /* child status reports */
    struct BINHASH *children;       /* linkage */
    struct MASTER_SERV *next;       /* linkage */	
} MASTER_SERV;


/**/
#define MASTER_FLAG_THROTTLE    (1<<0)  /* we're having trouble */
#define MASTER_FLAG_MARK        (1<<1)  /* garbage collection support */
#define MASTER_FLAG_CONDWAKE    (1<<2)  /* wake up if actually used */
#define MASTER_FLAG_INETHOST    (1<<3)  /* endpoint name specifies host */
#define MASTER_FLAG_LISTEN	(1<<5)	/* monitor this port */

#define MASTER_THROTTLED(f) ((f)->flags & MASTER_FLAG_THROTTLE)

#define MASTER_LISTENING(f)	((f)->flags & MASTER_FLAG_LISTEN)

#define MASTER_LIMIT_OK(limit, count) ((limit) == 0 || ((count) < (limit)))

 /*
  * Service types.
  */
#define MASTER_SERV_TYPE_UNIX   1   /* AF_UNIX domain socket */
#define MASTER_SERV_TYPE_INET   2   /* AF_INET domain socket */
#define MASTER_SERV_TYPE_FIFO   3   /* fifo (named pipe) */

 /*
  * Structure of child process.
  */
typedef int MASTER_PID;         /* pid is key into binhash table */

typedef struct MASTER_PROC {
	MASTER_PID 	pid;         /* child process id */
	int     	avail;       /* availability */
	MASTER_SERV *serv;       /* parent linkage */
	int     	use_count;   /* number of service requests */
} MASTER_PROC;

#define MASTER_BUF_LEN  2048        /* logical config line length */

/*
 * define event loop 
 * */
extern event_loop *master_el;

#define MASTER_MAX_CLIENTS 10000

/*
  * master_ent.c
  */
extern void fset_master_ent(char *);
extern void set_master_ent(void);
extern void end_master_ent(void);
extern void print_master_ent(MASTER_SERV *);
extern MASTER_SERV *get_master_ent(void);
extern void free_master_ent(MASTER_SERV *);

/*
 * master_conf.c
 */
extern void master_config(void);
extern void master_refresh(void);


/*
 * master_service.c
 * */
extern MASTER_SERV *master_head;
extern void master_start_service(MASTER_SERV *);
extern void master_stop_service(MASTER_SERV *);
extern void master_restart_service(MASTER_SERV *);

/*
 * master_sig.c
 */
extern int master_gotsighup;
extern int master_gotsigchld;
extern void master_sigsetup(void);

/*
 * master_vars.c
 */
extern char *var_program_dir;
extern int var_proc_limit;
extern int var_use_limit;
extern int var_idle_limit;

extern void master_vars_init(void);


/*
 * master_listen.c
 */
extern void master_listen_init(MASTER_SERV *);
extern void master_listen_cleanup(MASTER_SERV *);

/*
 * master_avail.c
 */
extern void master_avail_listen(MASTER_SERV *);
extern void master_avail_cleanup(MASTER_SERV *);
extern void master_avail_more(MASTER_SERV *, MASTER_PROC *);
extern void master_avail_less(MASTER_SERV *, MASTER_PROC *);

/*
 * master_status.c
 */
extern void master_status_init(MASTER_SERV *);
extern void master_status_cleanup(MASTER_SERV *);

/*
 * master_wakeup.c
 */
//extern void master_wakeup_init(MASTER_SERV *);
//extern void master_wakeup_cleanup(MASTER_SERV *);

/*
 * master_spawn.c
 */
extern struct BINHASH *master_child_table;
extern void master_spawn(MASTER_SERV *);
extern void master_reap_child(void);
extern void master_delete_children(MASTER_SERV *);

#endif
