# Makefile for vzctl - Virtual Environments control utility
# Copyright (c) 1999-2017, Parallels International GmbH

     INSTALL = install
      PREFIX = /usr
     SBINDIR = ${PREFIX}/sbin
      LIBVER = 1
### Compiler flags
	  CC = gcc

     VERSION ?= $(shell cat ../Makefile.version)
      CFLAGS += -pipe -Werror -Wall -Winline -Wcast-align -Wno-unused-parameter -Wstrict-prototypes -Wunused-variable -g2 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -DVERSION=\"$(VERSION)\"
	 INC = -I../include -Ilib
     LDFLAGS += -L lib

#     CFLAGS += -DDEBUG

# We need -m64 for ppc64 in order to get proper ioctls
# Do NOT use ARCH since it is always set to ppc from the spec file
ifeq ($(shell uname -m),ppc64)
override CFLAGS += -m64
endif

### Object files
	OBJS = script.o vz.o \
	       config.o tmplmn.o \
	       list.o \
	       vzmonitor.o \
	       snapshot-list.o technologies.o wrap.o util.o

   OBJS_vzctl = vzctl.o enter.o $(OBJS)
 OBJ_vzeventd = vzeventd.o
       OBJ_nd = ndsend.o
 OBJS_ubclogd = ubclogd.o timeutils.o
   OBJS_split = vzsplit.o
 OBJS_convert = vzcfgconvert.o config.o list.o
OBJS_cfgscale = vzcfgscale.o mm.o $(OBJS)
OBJS_vzauth   = vzauth.o
OBJS_proxyctl = vzproxyctl.o
OBJS_vztactl  = vztactl.o tc.o list.o util.o wrap.o
OBJS_vznetstat= vznetstat.o tc.o
OBJS_vzlist   = vzlist.o $(OBJS)
### Target names
	PROGS = vzctl ubclogd vzsplit \
		vzauth \
		vztactl vznetstat vzlist ndsend vzeventd

	 LIB = $(LGCOV) -lpthread -lutil -luuid -lploop -ldl -lvzctl2
      LIBDIR = /usr/lib64

##################################################
default: all

.force:;

.stamp-debug:
	$(MAKE) clean depend
	touch $@

.stamp-all:
	$(MAKE) clean depend
	touch $@

debug:.stamp-debug .force
	$(MAKE) $(MAKEOPTS) real-all DEBUG='-DDEBUG -g'

all: .stamp-all .force
	$(MAKE) $(MAKEOPTS) real-all

real-all: $(PROGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

vzeventd: $(OBJ_vzeventd)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) -lvzctl2 -ludev $^ -o $@

ndsend: $(OBJ_nd)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ -o $@

vzctl: $(OBJS_vzctl) $(OBJS)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ -lutil $(LIB) -o $@

ubclogd: $(OBJS_ubclogd)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ -o $@

vzsplit: $(OBJS_split)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ $(LIB) -o $@

vzcfgconvert: $(OBJS_convert)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ $(LIB) -o $@

vzcfgscale: $(OBJS_cfgscale)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ $(LIB) -o $@

vzauth: $(OBJS_vzauth)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ $(LIB) -o $@

vzproxyctl: $(OBJS_proxyctl)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ -o $@
vztactl: $(OBJS_vztactl)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ $(LIB) -o $@
vznetstat: $(OBJS_vznetstat)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ $(LIB) -luuid -o $@
vzlist: $(OBJS_vzlist)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $^ $(LIB) -lm -o $@

clean:
	$(RM) $(PROGS) $(OBJS) $(OBJS_memcheck)\
	$(OBJS_ubclogd) $(OBJS_split) $(OBJS_calc) $(OBJS_convert) \
	$(OBJS_cfgscale) $(OBJS_vzauth) \
	$(OBJS_proxyctl) $(OBJS_vztactl) $(OBJS_vznetstat) \
	$(OBJS_vzlist) $(OBJ_nd).stamp* .depend *[~\#]

installsbin: $(PROGS)
	for file in $(PROGS); do \
		$(INSTALL) $$file $(DESTDIR)$(SBINDIR)/$$file; \
	done

install: installsbin

depend:
	$(CC) $(CFLAGS) $(INC) -M $(OBJS:.o=.c) >.depend

-include .depend

.PHONY: clean depend install all real-all debug default installsbin \
