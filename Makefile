# Makefile for vzctl - Virtual Environments control utility
# Copyright (c) 1999-2017, Parallels International GmbH
# Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.

include Makefile.incl

        INSTALL = install
         PREFIX = /usr
          VZDIR = /etc/vz
        SBINDIR = ${PREFIX}/sbin
        CONFDIR = ${PREFIX}/etc/vz/conf
   NETSCRIPTDIR = /etc/sysconfig/network-scripts
     SYSTEMDDIR = ${PREFIX}/lib/systemd/system
SYSTEMDSCRIPTDIR = ${PREFIX}/libexec
 VZCTLSCRIPTDIR = ${SYSTEMDSCRIPTDIR}/vzctl
   MODPROBEDDIR = /etc/modprobe.d
     SYSCTLDDIR = /etc/sysctl.d
MODULESLOADDDIR = /etc/modules-load.d
         MANDIR = ${PREFIX}/man
      VZLOCKDIR = /vz/lock
   VZCTLLOCKDIR = /var/lock/vzctl
     VZSPOOLDIR = /var/vz
        LOGRDIR = /etc/logrotate.d
   BASHCOMPLDIR = /etc/bash_completion.d
     VENAMESDIR = $(VZDIR)/names
       VZDEVDIR = $(VZDIR)/dev
     VZEVENTDIR = $(VZDIR)/vzevent.d
 DRACUT_MOD_DIR = /usr/lib/dracut/modules.d

### Target names
 SBINSCRIPTS = vzpurge vzgetpa vzcpucheck vzdiskcheck vzpid user_beancounters.py

      SYSCTL = 99-vzctl.conf
    MODPROBE = vz.conf
     MODLOAD = vz.conf
SYSTEMDUNITS = vzevent.service vz.service vz-k8s-inside-ct.service vz-cgroup-dummy.service
SYSTEMDSCRIPTS = vz vz-k8s-inside-ct
  DRACUTMODS = 99vzctl

   VE0CONFIG = 0.conf networks_classes
VECONFIG_VSWAP = ve-vswap.256MB.conf-sample ve-vswap.512MB.conf-sample \
		ve-vswap.1024MB.conf-sample ve-vswap.2048MB.conf-sample \
		ve-vswap.plesk.conf-sample

    VECONFIG = ve-basic.conf-sample \
	       ve-confixx.conf-sample \
	       vps.vzpkgtools.conf-sample \
		networks_classes

    VZCONFIG = vz.conf
        MAN8 = vzctl.8 vzpurge.8 vzgetpa.8 vznetstat.8 \
	       vzsplit.8 vzcpucheck.8 vzpid.8 vzcfgscale.8 vzcfgvalidate.8 \
               vzdiskcheck.8 \
	       vzauth.8 vztactl.8 vzlist.8 \
	       vzeventd.8
        MAN5 = vz.5 ve.conf.5 vz-start.5 vz-stop.5 \
               vz-create_prvt.5 \
               vz-postinst.5 vz-net_add.5 vz-net_del.5 networks_classes.5
BASHCOMPLSCRIPT = vzctl.sh

 VZEVENTSCRIPTS = ve-reboot ve-stop ve-fserror

##################################################
default: all

define do_rebrand
	sed -e "s,@PRODUCT_NAME_SHORT@,$(PRODUCT_NAME_SHORT),g" -i $(1) || exit 1;
endef

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

real-all:
	(cd src && $(MAKE) $(MAKEOPTS))

clean:
	(cd src && ${MAKE} $@)

installsbinscripts:
	for file in $(SBINSCRIPTS); do \
		$(INSTALL) $$file $(DESTDIR)$(SBINDIR)/$$file; \
	done

installbashcompl:
	for file in $(BASHCOMPLSCRIPT); do \
		$(INSTALL) etc/bash_completion.d/$$file $(DESTDIR)$(BASHCOMPLDIR)/$$file; \
	done

installveconfig:
	for file in $(VECONFIG_VSWAP); do \
		$(INSTALL) etc/$$file $(DESTDIR)$(CONFDIR)/$$file; \
		echo 'ARCH="$(ARCH)"' >> $(DESTDIR)$(CONFDIR)/$$file; \
	done
	for file in $(VECONFIG); do \
		$(INSTALL) etc/$$file $(DESTDIR)$(CONFDIR)/$$file; \
	done

install-systemd:
	for file in $(SYSTEMDUNITS); do \
		$(INSTALL) -m 0644 etc/systemd.d/$$file $(DESTDIR)$(SYSTEMDDIR)/$$file; \
		$(call do_rebrand,$(DESTDIR)$(SYSTEMDDIR)/$$file) \
	done
	for file in $(SYSTEMDSCRIPTS); do \
		$(INSTALL) -m 0755 etc/systemd.d/$$file $(DESTDIR)$(SYSTEMDSCRIPTDIR)//$$file; \
		$(call do_rebrand,$(DESTDIR)$(SYSTEMDSCRIPTDIR)/$$file) \
	done

install-modulesd:
	for file in $(MODPROBE); do \
		$(INSTALL) -m 644 etc/modprobe.d/$$file $(DESTDIR)$(MODPROBEDDIR)/$$file; \
	done

install-sysctld:
	for file in $(SYSCTL); do \
		$(INSTALL) -m 644 etc/sysctl.d/$$file $(DESTDIR)$(SYSCTLDDIR)/$$file; \
	done

install-modules-load:
	for file in $(MODLOAD); do \
		$(INSTALL) -m 644 etc/modules-load.d/$$file $(DESTDIR)$(MODULESLOADDDIR)/$$file; \
	done

installconfig:
	for file in $(VZCONFIG); do \
		$(INSTALL) -m 644  etc/$$file $(DESTDIR)$(VZDIR)/$$file; \
	done
installmans:
	for file in $(MAN8); do \
		$(INSTALL) -m 0644 man/$$file $(DESTDIR)$(MANDIR)/man8/$$file; \
		$(call do_rebrand,$(DESTDIR)$(MANDIR)/man8/$$file) \
	done
	for file in $(MAN5); do \
		$(INSTALL) -m 0644 man/$$file $(DESTDIR)$(MANDIR)/man5/$$file; \
		$(call do_rebrand,$(DESTDIR)$(MANDIR)/man5/$$file) \
	done

installvzevent:
	for file in $(VZEVENTSCRIPTS); do \
		$(INSTALL) etc/vzevent.d/$$file $(DESTDIR)$(VZEVENTDIR)/$$file; \
	done

installdracutmod:
	for dir in $(DRACUTMODS); do \
		$(INSTALL) -d $(DESTDIR)$(DRACUT_MOD_DIR)/$$dir || : ; \
		for file in etc/dracut/$$dir/*; do \
			$(INSTALL) -m 755 $$file $(DESTDIR)$(DRACUT_MOD_DIR)/$$dir/`basename $$file`; \
		done; \
	done

installdirs:
	$(INSTALL) -d $(DESTDIR)$(VZDIR)
	$(INSTALL) -d $(DESTDIR)$(CONFDIR)
	$(INSTALL) -d $(DESTDIR)$(NETSCRIPTDIR)
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man5
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -d $(DESTDIR)$(SYSTEMDDIR)
	$(INSTALL) -d $(DESTDIR)$(SYSTEMDSCRIPTDIR)
	$(INSTALL) -d $(DESTDIR)$(VZCTLSCRIPTDIR)
	$(INSTALL) -d $(DESTDIR)$(MODPROBEDDIR)
	$(INSTALL) -d $(DESTDIR)$(SYSCTLDDIR)
	$(INSTALL) -d $(DESTDIR)$(MODULESLOADDDIR)
	$(INSTALL) -d $(DESTDIR)$(VZLOCKDIR)
	$(INSTALL) -d $(DESTDIR)$(VZCTLLOCKDIR)
	$(INSTALL) -d $(DESTDIR)$(LOGRDIR)
	$(INSTALL) -d $(DESTDIR)$(VENAMESDIR)
	$(INSTALL) -d $(DESTDIR)$(VZSPOOLDIR)
	$(INSTALL) -m 644 etc/logrotate.d/vzctl $(DESTDIR)$(LOGRDIR)/vzctl
	$(INSTALL) -d $(DESTDIR)$(BASHCOMPLDIR)
	$(INSTALL) -d $(DESTDIR)$(VZEVENTDIR)
	$(INSTALL) -d $(DESTDIR)$(VZDEVDIR)

install: installdirs installsbinscripts installmans \
	installconfig install-modulesd install-systemd \
	installveconfig install-sysctld \
	install-modules-load \
	installbashcompl installvzevent \
	installdracutmod
	(cd src && ${MAKE} $@)


depend:
	(cd src && ${MAKE} $@)

rpms: clean
	(cd .. && tar cvjf `rpm --eval "%{_sourcedir}"`/vzctl.tar.bz2 vzctl)
	rpmbuild -ba --nodeps vzctl.spec

-include .depend

.PHONY: clean depend install installdirs installmans installconfig \
 install-modulesd install-modules-load all real-all debug default rpms \
 installveconfig installlib install-systemd

