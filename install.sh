#!/bin/bash

########################################################################
# START OF PROGRAM
########################################################################
export PATH=/bin:/usr/bin:/sbin:/usr/sbin

. ./func.inc

case "$1" in

	base)
		check_sanity

		if [ "$CPUCORES" = "detect" ]; then
			CPUCORES=`grep -c processor //proc/cpuinfo`
		fi

		if [ "$USER" = "Lv" ]; then
			die "User Lv is not allowed,please change USER in install.conf"
		fi
		if [ -f "./conf/install.conf" ]; then
		  apt-get update
			check_install nano nano
			nano ./conf/install.conf
		fi

		[ -r ./conf/install.conf ] && . ./conf/install.conf
		update_upgrade
		remove_unneeded
		install_exim4
		install_dropbear $SSH_PORT
		#install_iptables $SSH_PORT
		install_syslogd
		;;
	nvapm)
		[ -r ./conf/install.conf ] && . ./conf/install.conf
		install_nginx
		conf_nginx
		install_mysql
		conf_mysql
		install_php5
		conf_php5
		install_apache2
		conf_apache2
		install_varnish
		conf_varnish
		;;
	domain)
		[ -r ./conf/install.conf ] && . ./conf/install.conf
		install_domain $DOMAIN
		;;

	*)
        echo 'Usage:' `basename $0` '[option] [argument]'
        echo 'Available options (in recomended order):'
        echo '  - base'
		echo '	- nvapm'
        echo '  - domain'
        echo '  '
        ;;
esac