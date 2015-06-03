#!/bin/bash
# Copyright (c) 1999-2015 Parallels IP Holdings GmbH.
# All rights reserved.

_get_ves()
{
	local cmd=$1
	local rm_empty='sed s/^-[[:space:]]*//'
	[ "$EUID" -ne 0 ] && exit 1
	case $cmd in
		create)
			# create a new VEID, by increasing the last one
			local veids=`/usr/sbin/vzlist -H -a -octid | tail -1`
			[ -n "$veids" ] || veids=100
			echo $((veids+1))
			;;
		start|mount|umount|destroy|delete)
			# stopped VEs
			/usr/sbin/vzlist -H -S -octid
			/usr/sbin/vzlist -H -S -oname | $rm_empty
			;;
		stop|enter|exec*)
			# running VEs
			/usr/sbin/vzlist -H -octid
			/usr/sbin/vzlist -H -oname | $rm_emtpy
			;;
		*)
			# All VEs
			/usr/sbin/vzlist -H -a -octid
			/usr/sbin/vzlist -H -a -oname | $rm_empty
			;;
	esac
}

_vzctl()
{
	#echo "ARGS: $*"
	#echo "COMP_WORDS: $COMP_WORDS"
	#echo "COMP_CWORD: $COMP_CWORD"
	#echo "COMP_WORDS[1]: ${COMP_WORDS[1]}"

	COMPREPLY=()
	local cur=${COMP_WORDS[COMP_CWORD]}
	local prev=${COMP_WORDS[COMP_CWORD-1]}

	local vzctl_common_opts="--quiet --verbose --help --version"
	local vzctl_cmds="create destroy delete mount umount chkpnt restore \
		set start stop restart status enter exec exec2 runscript \
		console convert snapshot snapshot-switch snapshot-delete \
		snapshot-mount snapshot-umount snapshot-list"
	local vzctl_create_opts="--ostemplate --config --private --root \
		--ipadd --hostname --name --description --skip_app_templates"
	local vzctl_set_opts="--save --onboot \
		--numproc --numtcpsock --numothersock \
		--kmemsize --tcpsndbuf --tcprcvbuf \
		--othersockbuf --dgramrcvbuf --oomguarpages \
		--lockedpages --privvmpages --shmpages \
		--numfile --numflock --numpty \
		--numsiginfo --dcachesize \
		--numiptent --physpages --swappages --diskspace \
		--diskinodes --quotatime --quotaugidlimit \
		--cpuunits --cpulimit --cpus --slmmode \
		--slmmemorylimit --ipadd --ipdel \
		--root --hostname --nameserver --searchdomain \
		--userpasswd --onboot --rate --ratebound \
		--noatime --bindmount_add --bindmount_del \
		--capability --devices  --devnodes \
		--netdev_add --netdev_del \
		--netif_add --netif_del \
		--netfilter --disabled --applyconfig --setmode \
		--bootorder --ha_enable --ha_prio"
	local vzctl_snapshot_create_opts="--id --name --description"
	local vzctl_snapshot_mount_opts="--id --target"
	local vzctl_snapshot_list_opts="-H --no-header -o --output --id \
		-L --list --help"
	local vzctl_console_opts="-s --start 1 2"


	local netfilter_names="disabled stateless stateful full"
	local cap_names="chown dac_override dac_read_search fowner fsetid kill
		setgid setuid setpcap linux_immutable net_bind_service
		net_broadcast net_admin net_raw ipc_lock ipc_owner
		sys_module sys_rawio sys_chroot sys_ptrace sys_pacct
		sys_admin sys_boot sys_nice sys_resource sys_time
		sys_tty_config mknod lease setveid ve_admin"



	case $COMP_CWORD in
	1)
		# command or global option
		COMPREPLY=( $( compgen -W "$vzctl_cmds $vzctl_common_opts" -- $cur ) )
		;;

	2)
		case "$prev" in
		--help|--version)
			COMPREPLY=()
			;;
		--*)
			# command
			COMPREPLY=( $( compgen -W "$vzctl_cmds" -- $cur ) )
			;;
		*)
			# VEID
			COMPREPLY=( $( compgen -W "$(_get_ves $prev)" -- $cur ) )
			;;
		esac
		;;

	*) # COMP_CWORD >= 3
		if [[ $COMP_CWORD -eq 3 && ! $prev != '^[1-9][0-9]*$' ]]; then
			# VEID
			COMPREPLY=( $( compgen -W "$(_get_ves $prev)" -- $cur ) )
		else

			# flag or option
			case $prev in
			--ostemplate)
				COMPREPLY=( $( compgen -W "$(/usr/sbin/vzpkg list -O -q;
					/usr/sbin/vzpkg list -O -q --available)" -- $cur ) )
				;;
			--onboot|--disabled|--noatime|--ha_enable)
				COMPREPLY=( $( compgen -W "yes no" -- $cur ) )
				;;
			--setmode)
				COMPREPLY=( $( compgen -W "restart ignore" -- $cur ) )
				;;
			--config|--applyconfig)
				local configs=$(command ls -1 /etc/vz/conf/*.conf-sample | \
						    cut -d "-" -f 2- | sed -e 's#.conf-sample$##')

				configs=${configs/.conf-sample/}
				COMPREPLY=( $( compgen -W "$configs" -- $cur ) )
				;;
			--netfilter)
				COMPREPLY=( $( compgen -W "$netfilter_names" -- $cur ) )
				;;
			--netdev*)
				local devs=`ip addr show | awk '/^[0-9]/ && /UP/ && !/venet/ && !/lo/ \
						    { print $2 }' | sed s/://`

				COMPREPLY=( $( compgen -W "$devs" -- $cur ) )
				;;
			--meminfo)
				local meminfo="none privvmpages:1"
				COMPREPLY=( $( compgen -W "$meminfo" -- $cur ) )
				;;
			--ioprio)
				# ioprio range: 0 - 7 (default : 4)
				local ioprio='0 1 2 3 4 5 6 7'
				COMPREPLY=( $( compgen -W "$ioprio" -- $cur ) )
				;;
			--capability)
				# capname:on|off
				COMPREPLY=( $( compgen -W "$cap_names" -- $cur ) )
				# FIXME: add :on or :off -- doesn't work :(
#				if [[ ${#COMPREPLY[@]} -le 1 ]]; then
#					if [[ $cur =~ ":" ]]; then
#						cap=${cur%%:*}
#					else
#						cap=${COMPREPLY[0]%%:*}
#					fi
					# Single match: add :on|:off
#					COMPREPLY=( $(compgen -W \
#					"${cap}:on ${cap}:off" -- $cur) )
#				fi
				;;
			--devnodes)
				# FIXME: device:r|w|rw|none
				local devs=''
				COMPREPLY=( $( compgen -W "$devs" -- $cur ) )
				;;
			--devices)
				# FIXME: b|c:major:minor|all:r|w|rw
				local devices=''
				COMPREPLY=( $( compgen -W "$devices" -- $cur ) )
				;;
			--ipdel)
				[ "$EUID" -ne 0 ] && exit 1
				# Get VEID
				local ve=${COMP_WORDS[2]}
				if [[ ! ${ve} != '^[1-9][0-9]*$' ]] ; then
					# --verbose or --quiet used
					ve=${COMP_WORDS[3]}
				fi
				# VENAME or VEID ?
				LIST_OPT=`echo $ve | awk '/[a-zA-Z]/ {print "-N"}'`
				local ips="`/usr/sbin/vzlist -H -o ip $LIST_OPT $ve | grep -vi -`"
				COMPREPLY=( $( compgen -W "$ips all" -- $cur ) )
				;;
			--netif_del)
				local netif
				if [ -f /proc/vz/veth ]; then
					local ve=${COMP_WORDS[2]}
					if [[ ! ${ve} != '^[1-9][0-9]*$' ]] ; then
						# --verbose or --quiet used
						ve=${COMP_WORDS[3]}
					fi
					# get veth(ernet) device of VE, proc and config
					local netif_proc="`awk "/${ve}/ { print \\$4 }" /proc/vz/veth`"
					local netif_conf="`grep ^NETIF /etc/vz/conf/${ve}.conf | \
						sed -e 's/"/\n/g' -e 's/;/\n/g' -e 's/,/\n/g' -e 's/=/ /g' | awk '/^ifname/ {print \$2}'`"
					netif="$netif_proc $netif_conf"
				fi
				COMPREPLY=( $( compgen -W "$netif" -- $cur ) )
				;;
			--private|--root)
				# FIXME
				# Dir autocompletion works bad since there is
				# a space added. Alternatively, we could use
				# -o dirname option to 'complete' -- but it
				# makes no sense for other parameters (UBCs
				# etc). So no action for now.
				;;
			-s|--start)
				# vzctl console -s|--start
				# Complete tty number, except 1 and 2
				# which already should have getty running
				COMPREPLY=( $( compgen -W "3 4 5 6 7 8 9" -- $cur ) )
				;;
			--id|--uuid)
				# Only allow this completion
				# when user has root privileges
				[ "$EUID" -ne 0 ] && exit 1
				# Get command and CTID
				local cmd=${COMP_WORDS[1]}
				local ve=${COMP_WORDS[2]}
				if [ ${cmd::2} = "--" ] ; then
					# --verbose or --quiet used
					cmd=${COMP_WORDS[2]}
					ve=${COMP_WORDS[3]}
				fi
				local ids
				case ${cmd} in
				    snapshot-umount)
					# For umount, only give mounted IDs
					ids="`/usr/sbin/vzctl snapshot-list $ve -H -ouuid,device 2>/dev/null | awk 'NF==2 {print $1}' | tr -d '{}'`"
					;;
				    snapshot-*)
					# For other commands, give all IDs
					ids="`/usr/sbin/vzctl snapshot-list $ve -H -ouuid 2>/dev/null | tr -d '{}'`"
					;;
				    *)
					# Do not give anything for other cmds
					# including 'snapshot' itself
					# since it requires a new unused ID
					exit 0
					;;
				esac
				COMPREPLY=( $( compgen -W "$ids" -- $cur ) )
				;;
			*)
				if [[ "${prev::2}" != "--" || "$prev" = "--save" ]]; then
					# List options
					local cmd=${COMP_WORDS[1]}
					if [ ${cmd::2} = "--" ] ; then
						# --verbose or --quiet used
						cmd=${COMP_WORDS[2]}
					fi

					case "$cmd" in
					create)
						COMPREPLY=( $( compgen -W "$vzctl_create_opts" -- $cur ) )
						;;
					set)
						COMPREPLY=( $( compgen -W "$vzctl_set_opts" -- $cur ) )
						;;
					chkpnt|restore)
						COMPREPLY=( $( compgen -W "--dumpfile" -- $cur ) )
						;;
					start)
						COMPREPLY=( $( compgen -W "--wait" -- $cur ) )
						;;

					stop)
						COMPREPLY=( $( compgen -W "--fast --skip-umount" -- $cur ) )
						;;
					snapshot)
						COMPREPLY=( $( compgen -W "$vzctl_snapshot_create_opts" -- $cur ) )
						;;
					snapshot-switch|snapshot-delete|snapshot-umount)
						COMPREPLY=( $( compgen -W "--id" -- $cur ) )
						;;
					snapshot-mount)
						COMPREPLY=( $( compgen -W "$vzctl_snapshot_mount_opts" -- $cur ) )
						;;
					snapshot-list)
						COMPREPLY=( $( compgen -W "$vzctl_snapshot_list_opts" -- $cur ) )
						;;
					console)
						COMPREPLY=( $( compgen -W "$vzctl_console_opts" -- $cur ) )
						;;

					*)
						;;
					esac
				else
					# Option that requires an argument
					# which we can't autocomplete
					COMPREPLY=( $( compgen -W "" -- $cur ) )
				fi
				;;
			esac
		fi
	esac

	return 0
}

complete -F _vzctl vzctl

# EOF
