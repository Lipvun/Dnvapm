#主要基本函数
#本文件不建议修改

#检查后安装 带两个参数
function check_install {
        if [ -z "`which "$1" 2>/dev/null`" ]
        then
                executable=$1
                shift
                while [ -n "$1" ]
                do
                        DEBIAN_FRONTEND=noninteractive apt-get -q -y install "$1"
                        print_info "$1 installed for $executable"
                        shift
                done
        else
                print_warn "$2 already installed"
        fi
}
#检查后卸载 带两个参数
function check_remove {
        if [ -n "`which "$1" 2>/dev/null`" ]
        then
                DEBIAN_FRONTEND=noninteractive apt-get -q -y remove --purge "$2"
                print_info "$2 removed"
        else
                print_warn "$2 is not installed"
        fi
}
#检查用户及系统版本是否适用本脚本
function check_sanity {
        if [ $(/usr/bin/id -u) != "0" ]
        then
                die 'Must be run by root user'
        fi
        if [ ! -f /etc/debian_version ]
        then
                die "Distribution is not supported"
        fi
}
#错误中断
function die {
        echo "ERROR: $1" > /dev/null 1>&2
        exit 1
}
#获取域名
function get_domain_name() {
        # Getting rid of the lowest part.
        domain=${1%.*}
        lowest=`expr "$domain" : '.*\.\([a-z][a-z]*\)'`
        case "$lowest" in
        com|net|org|gov|edu|co|me|info|name)
                domain=${domain%.*}
                ;;
        esac
        lowest=`expr "$domain" : '.*\.\([a-z][a-z]*\)'`
        [ -z "$lowest" ] && echo "$domain" || echo "$lowest"
}
#获取随机13位密码，主要用于mysql密码
function get_password() {
        # Check whether our local salt is present.
        SALT=/var/lib/radom_salt
        if [ ! -f "$SALT" ]
        then
                head -c 512 /dev/urandom > "$SALT"
                chmod 400 "$SALT"
        fi
        password=`(cat "$SALT"; echo $1) | md5sum | base64`
        echo ${password:0:13}
}

#添加用户
function add_user {
	if [ -z `grep $USER: /etc/passwd` ]; then
		useradd -m $USER
		cat >> /etc/sudoers.d/users <<END
$USER   ALL=(ALL:ALL) ALL
END
		if [ ! -d /home/$USER/Maildir ]; then
			mkdir /home/$USER/Maildir
		fi
		if [ ! -d /home/$USER/Maildir/cur ]; then
			mkdir /home/$USER/Maildir/cur
		fi
		if [ ! -d /home/$USER/Maildir/tmp ]; then
			mkdir /home/$USER/Maildir/tmp
		fi
		if [ ! -d /home/$USER/Maildir/new ]; then
			mkdir /home/$USER/Maildir/new
		fi

		chown -R $USER:$USER /home/$USER/Maildir
		echo Set password for $USER
		passwd $USER
	fi
}

#输出信息
function print_info {
        echo -n -e '\e[1;36m'
        echo -n $1
        echo -e '\e[0m'
}
#输出警告
function print_warn {
        echo -n -e '\e[1;33m'
        echo -n $1
        echo -e '\e[0m'
}

#发行版检测：
function check_lsb {
        # Detect distribution. Debian or Ubuntu
        DISTRO=`lsb_release -i -s`
        # Distribution's release. Squeeze, wheezy, precise etc
        RELEASE=`lsb_release -c -s`
        if [ $DISTRO = "" ]; then
                echo -e "\033[35;1mPlease run 'aptitude -y install lsb-release' before using this script.\033[0m"
          exit 1
        fi
}

#更新系统及源

function update_upgrade {
    cat > /etc/apt/sources.list.d/backports.list <<END
deb http://ftp.debian.org/debian/ wheezy-backports main
#deb-src http://ftp.debian.org/debian/ wheezy-backports main
END
    apt-get -q -y update
    if [ "$OPENVZ" = 'yes' ]; then
        if [ -z "`grep 'ulimit -s 256' /etc/init.d/rc`" ];then
           sed -i "s/export PATH/export PATH\\nulimit -s 256/" /etc/init.d/rc
        fi
        if [ ! -f /etc/security/limits.d/stack.conf ]; then
            cat > /etc/security/limits.d/stack.conf <<END
root            -       stack           256
*               -       stack           256
END
        fi
    fi
	check_install sudo "sudo"
	add_user
	apt-get -q -y update
    check_install dialog "dialog"
    check_install locales "locales"  #设置编码
    dpkg-reconfigure locales
    apt-get -q -y upgrade
    check_install tzdata "tzdata"  #设置时区
    dpkg-reconfigure tzdata
    install_dash
    echo -n "To change root password press y then [ENTER]: "
    read -e reply
    if [ "$reply" = "y" ]; then
        passwd
    fi
}

#删除原有不需要的软件
function remove_unneeded {
        # Some Debian have portmap installed. We don't need that.
        check_remove /sbin/portmap portmap

        # Remove rsyslogd, which allocates ~30MB privvmpages on an OpenVZ system,
        # which might make some low-end VPS inoperatable. We will do this even
        # before running apt-get update.
        check_remove /usr/sbin/rsyslogd rsyslog

        # Other packages that seem to be pretty common in standard OpenVZ
        # templates.
        check_remove /usr/sbin/apache2 'apache2*'
        check_remove /usr/sbin/named bind9
        check_remove /usr/sbin/smbd 'samba*'
        check_remove /usr/sbin/nscd nscd

        # Need to stop sendmail as removing the package does not seem to stop it.
        if [ -f /usr/lib/sm.bin/smtpd ]
        then
                invoke-rc.d sendmail stop
                check_remove /usr/lib/sm.bin/smtpd 'sendmail*'
        fi
        apt-get autoremove && apt-get clean
}

#安装部分有用工具：

#dash
function install_dash {
        check_install dash dash
        rm -f /bin/sh
        ln -s dash /bin/sh
}
#nano
function install_nano {
        apt-get -q -y install nano
}
#htop
function install_htop {
        check_install htop htop
}
#mc
function install_mc {
        check_install mc mc
}
#iotop
function install_iotop {
        check_install iotop iotop
}
#iftop
function install_iftop {
        check_install iftop iftop
        print_warn "Run IFCONFIG to find your net. device name"
        print_warn "Example usage: iftop -i venet0"
}
#vim
function install_vim {
        check_install vim vim
}

#syslogd
function install_syslogd {
        # We just need a simple vanilla syslogd. Also there is no need to log to
        # so many files (waste of fd). Just dump them into
        # /var/log/(cron/mail/messages)
        check_install /usr/sbin/syslogd inetutils-syslogd
        invoke-rc.d inetutils-syslogd stop

        for file in /var/log/*.log /var/log/mail.* /var/log/debug /var/log/syslog
        do
                [ -f "$file" ] && rm -f "$file"
        done
        for dir in fsck news
        do
                [ -d "/var/log/$dir" ] && rm -rf "/var/log/$dir"
        done

        cat > /etc/syslog.conf <<END
*.*;mail.none;cron.none -/var/log/messages
cron.*                                  -/var/log/cron
mail.*                                  -/var/log/mail
END

        [ -d /etc/logrotate.d ] || mkdir -p /etc/logrotate.d
        cat > /etc/logrotate.d/inetutils-syslogd <<END
/var/log/cron
/var/log/mail
/var/log/messages {
        rotate 4
        weekly
        missingok
        notifempty
        compress
        sharedscripts
        postrotate
        /etc/init.d/inetutils-syslogd reload >/dev/null
        endscript
}
END

        invoke-rc.d inetutils-syslogd start
}

##安装服务器软件varnish nginx mysql php

#varnish

function install_varnish {
  check_install curl curl
	curl http://repo.varnish-cache.org/debian/GPG-key.txt | apt-key add -
	echo "deb http://repo.varnish-cache.org/debian/ wheezy varnish-3.0" >> /etc/apt/sources.list
	apt-get update
	check_install varnish varnish
}

function conf_varnish {
	if [ -f /etc/varnish/default.vcl ]; then
		rm -f /etc/varnish/default.vcl
		cp ./conf/varnish/varnish /etc/varnish/default.vcl
	fi
	if [ -f /etc/default/varnish ]; then
		sed -i "s/-a :6081/-a localhost:6081/g" /etc/default/varnish
	fi
	invoke-rc.d varnish restart
}

#nginx

function install_nginx {
    wget -O - http://nginx.org/keys/nginx_signing.key | apt-key add -
    cat > /etc/apt/sources.list.d/nginx.list <<END
deb http://nginx.org/packages/mainline/debian/ wheezy nginx
deb-src http://nginx.org/packages/mainline/debian/ wheezy nginx
END
    apt-get update
    apt-get -y remove nginx nginx-full nginx-common
    apt-get install nginx
    sed -i "s/rotate 52/rotate 1/" /etc/logrotate.d/nginx  #减少回滚日志
}

function conf_nginx {
    #cache部分
    mkdir -p /var/lib/nginx/cache
    chown -R www-data:www-data /var/lib/nginx/cache

	cp -r ./conf/nginx/nginx.conf /etc/nginx/
	cp -r ./conf/nginx/proxy_params /etc/nginx/
	if [ ! -d "/etc/nginx/sites-enabled" ]; then
		mkdir -p /etc/nginx/sites-enabled
	fi
	if [ ! -d "/etc/nginx/sites-available" ]; then
		mkdir -p /etc/nginx/sites-available
	fi
	cp ./conf/nginx/server-default /etc/nginx/sites-available/server-default

	ln -s /etc/nginx/sites-available/server-default /etc/nginx/sites-enabled/default.conf
	invoke-rc.d nginx reload
}

#mysql

function install_mysql {
    # Install the MySQL packages
    check_install mysqld mysql-server
    check_install mysql mysql-client
}

function conf_mysql {
	# Install a low-end copy of the my.cnf to disable InnoDB
	invoke-rc.d mysql stop
	#lowendmemory.cnf和innodb.cnf
	cp ./conf/mysql/* /etc/mysql/conf.d/
    invoke-rc.d mysql start

	# Generating a new password for the root user.
	passwd=`get_password root@mysql`
	mysqladmin -u root password $passwd
	cat > ~/.my.cnf <<END
[client]
user = root
password = $passwd
END
	chmod 600 ~/.my.cnf
}

function adduser_mysql {
        if [ -z "$1" ]
        then
                die "Usage: `basename $0` mysqluser [domain]"
        fi

        if [ ! -d "/srv/www/$1/" ]
        then
                echo "no site found at /srv/www/$1/"
                exit
        fi

        # Setting up the MySQL database
        dbname=`echo $1 | tr . _`
        userid=`get_domain_name $1`
        # MySQL userid cannot be more than 15 characters long
        userid="${userid:0:15}"
        passwd=`get_password "$userid@mysql"`

        cat > "/srv/www/$1/mysql.conf" <<END
[mysql]
user = $userid
password = $passwd
database = $dbname
END
        chmod 600 "/srv/www/$1/mysql.conf"

        mysqladmin create "$dbname"
        echo "GRANT ALL PRIVILEGES ON \`$dbname\`.* TO \`$userid\`@localhost IDENTIFIED BY '$passwd';" | \
                mysql

        # We could also add these...
        #echo "DROP USER '$userid'@'localhost';" | \ mysql
        #echo "DROP DATABASE IF EXISTS  `$dbname` ;" | \ mysql

        echo 'MySQL Username: ' $userid
        echo 'MySQL Password: ' $passwd
        echo 'MySQL Database: ' $dbname
}
#apache2
function install_apache2 {
	apt-get -q -y install apache2 apache2-mpm-prefork libapache2-mod-rpaf
}

function conf_apache2 {

	sed -i "13a     RPAFheader X-Real-IP" /etc/apache2/mods-available/rpaf.conf

	a2enmod rewrite
	a2enmod cache
	a2enmod rpaf
	a2enmod deflate
	cp -r ./conf/apache2/ports.conf /etc/apache2/ports.conf

	cat >> "/etc/apache2/apache2.conf" << END
#去版本号
ServerTokens Prod
ServerSignature Off
END

	invoke-rc.d apache2 restart
}

#php5

function install_php5 {
    if [ "$DISTRIBUTION" = "wheezy" ]; then
			apt-get -q -y install php5-common php-pear php5-dev php5-mysql php5-curl php-apc php5-mcrypt php5-imagick php5-xmlrpc php5-gd libapache2-mod-php5 php5-memcache memcached
    fi
	check_install memcached memcached
}

function conf_php5 {
	#apc
	if [ -f /etc/php5/apache2/conf.d/20-apc.ini ]; then
		rm /etc/php5/apache2/conf.d/20-apc.ini
	fi
	cp ./conf/php/apc.ini /etc/php5/apache2/conf.d/

	#去版本号
	sed -i "s/expose_php = on/expose_php = off/" /etc/php5/apache2/php.ini
}


#Domain

function install_domain {
    if [ -z "$1" ]
    then
            die "Usage: `basename $0` site [domain]"
    fi
    # Setup folder
	if [ ! -d "/srv/www" ]
	then
		mkdir /srv/www
		chown root:root /srv/www
	fi
	if [ ! -d "/srv/www/$1" ]
	then
    	mkdir /srv/www/$1
    	mkdir /srv/www/$1/public
	fi

	adduser_mysql $1


    # Setting up Nginx mapping
    cp ./conf/nginx/server-proxy /etc/nginx/sites-available/$1
    sed -i "s/DOMAIN/$1/g" /etc/nginx/sites-available/$1

    ln -s /etc/nginx/sites-available/$1 /etc/nginx/sites-enabled/$1.conf

    # Setting up Apache2 mapping
    cp ./conf/apache2/virtualhost /etc/apache2/sites-available/$1
    sed -i "s/DOMAIN/$1/g" /etc/apache2/sites-available/$1

    a2dissite default
    a2ensite $1

    sed -i "s/DOMAIN/$1/g" /etc/varnish/default.vcl

    invoke-rc.d apache2 restart
    invoke-rc.d varnish restart
    invoke-rc.d nginx restart
}


# iptables
#防火墙 开启ssh端口,防ssh攻击
function install_iptables {

        check_install iptables iptables

        if [ -z "$1" ]
        then
                die "Usage: `basename $0` iptables [ssh-port-#]"
        fi

        # Create startup rules
        cp ./conf/iptables/iptables.up.rules /etc/
        sed -i "s/SSH_PORT/$1/g" /etc/iptables.up.rules

        # Set these rules to load on startup
        cp ./conf/iptables/iptables /etc/network/if-pre-up.d/

        # Make it executable
        chmod +x /etc/network/if-pre-up.d/iptables

        # Load the rules
        iptables-restore < /etc/iptables.up.rules

        # You can flush the current rules with /sbin/iptables -F
        echo 'Created /etc/iptables.up.rules and startup script /etc/network/if-pre-up.d/iptables'
        echo 'If you make changes you can restore the rules with';
        echo '/sbin/iptables -F'
        echo 'iptables-restore < /etc/iptables.up.rules'
        echo ' '
}

#exim4
#exim4安装，并开启internet配置
function install_exim4 {
        check_install mail exim4
        if [ -f /etc/exim4/update-exim4.conf.conf ]
        then
                sed -i \
                        "s/dc_eximconfig_configtype='local'/dc_eximconfig_configtype='internet'/" \
                        /etc/exim4/update-exim4.conf.conf
                invoke-rc.d exim4 restart
        fi
        dpkg-reconfigure exim4-config
}

#dropbear
#dropbear安装端口为参数
function install_dropbear {
        if [ -z "$1" ]
        then
                die "Usage: `basename $0` dropbear [ssh-port-#]"
        fi
        #安装dropbear和xinetd
        check_install dropbear dropbear
        check_install /usr/sbin/xinetd xinetd
        # Disable SSH
        touch /etc/ssh/sshd_not_to_be_run
        invoke-rc.d ssh stop
        # 把droopbear加入xinetd,xinetd还可用于其它
        mv ./conf/dropbear/dropbear /etc/xinetd.d
        sed -i "s/SSH_PORT/$1/" /etc/xinetd.d/dropbear
        invoke-rc.d xinetd restart
}
