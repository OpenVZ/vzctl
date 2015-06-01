/* $Id$
 *
 * Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 * Prototypes of functions in script.c
 */

#ifndef _SCRIPT_H_
#define _SCRIPT_H_


int call_script(char *f, struct CList *arg, struct CList *env, int quiet);
char *readscript(char *name, int use_f);
char *makecmdline(char **argv);
void freearg(char **arg);
int wait_on_fifo(void *data);
int replace_reach_runlevel_mark(void);
char *MakeIpEnvVar(char *name, struct CList *list);
#endif /* _SCRIPT_H_ */

