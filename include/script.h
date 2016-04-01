/* $Id$
 *
 * Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 * Prototypes of functions in script.c
 */

#ifndef _SCRIPT_H_
#define _SCRIPT_H_
char *readscript(char *name, int use_f);
char *makecmdline(char **argv);
void freearg(char **arg);
#endif /* _SCRIPT_H_ */

