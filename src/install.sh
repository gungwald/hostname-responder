#!/bin/sh

OS=`uname -s`

PGM=hwaet
PGMD="$PGM"d
INSTALL_DIR='/usr/local'
DAEMON_NAME='Hostname Responder Daemon'

installSystemdService()
(
    SVC="$PGMD.service"
    if [ "$OP" = "install" ]
    then
	# If no group is given, it should create a group for the user.
        useradd -c "$DAEMON_NAME" -d / -s /sbin/nologin "$DAEMON_USER"
	# Copy pre-built service file into the system directory.
        install -p -o root -g root -m u=rwx,g=rx,o=rx "$SVC" /etc/systemd/system
	# Recognize the new service file
	systemctl daemon-reload
        systemctl enable "$SVC"
        systemctl start "$SVC"
	systemctl status "$SVC"
    else
        systemctl stop "$SVC"
        systemctl disable "$SVC"
        rm /etc/systemd/system/"$SVC"
        userdel $DAEMON_USER
    fi
)

if [ "$1" = "-u" ]
then
    OP=uninstall
else
    OP=install
fi

case $OS in

    OpenBSD)
        DAEMON_USER="_$PGMD"
        if [ "$OP" = "install" ]
        then
	    install -sp -o root -g bin $PGM $INSTALL_DIR/bin
	    install -sp -o root -g bin $PGMD $INSTALL_DIR/sbin
	    install -p -o root -g wheel -m a=rx $PGMD.rc.sh /etc/rc.d/$PGMD
            # Loosely based on info from there: https://www.maklin.fi/post/sysadmin/2023-10-22_how-to-add-services-on-openbsd/
            useradd -g =uid -c "$DAEMON_NAME" -L daemon -d /var/empty -s /sbin/nologin "$DAEMON_USER"
	    rcctl set $PGMD user $DAEMON_USER
            rcctl enable $PGMD
            rcctl start $PGMD
        else
            rcctl stop $PGMD
            rcctl disable $PGMD
            rm $INSTALL_DIR/bin/$PGM $INSTALL_DIR/sbin/$PGMD /etc/rc.d/$PGMD
            userdel $DAEMON_USER
        fi
        ;;
        
    FreeBSD | NetBSD)
        echo Don\'t know how on $OS. Please write me.
        exit
        
    Darwin)
        # This won't work. It needs to be modified for Mac.
        if [ "$OP" = "install" ]
        then
	    install -sp -o root -g bin $PGM $INSTALL_DIR/bin
	    install -sp -o root -g bin $PGMD $INSTALL_DIR/sbin
	    install -p -o root -g wheel -m a=rx $PGMD.rc.sh /etc/rc.d/$PGMD
            useradd -g =uid -c "$DAEMON_NAME" -L daemon -d /var/empty -s /sbin/nologin "$DAEMON_USER"
	    rcctl set $PGMD user $DAEMON_USER
            rcctl enable $PGMD
            rcctl start $PGMD
        else
            rcctl stop $PGMD
            rcctl disable $PGMD
            rm $INSTALL_DIR/bin/$PGM $INSTALL_DIR/sbin/$PGMD /etc/rc.d/$PGMD
            userdel $DAEMON_USER
        fi
        ;;

    Linux)
        DAEMON_USER="$PGMD"
        if [ "$OP" = "install" ]
        then
            install -sp -o root -g root $PGM $INSTALL_DIR/bin
            install -sp -o root -g root $PGMD $INSTALL_DIR/sbin
        else
            rm $INSTALL_DIR/bin/$PGM $INSTALL_DIR/sbin/$PGMD 
        fi
        
        if type systemd
        then
            installSystemdService
        elif type rcctl
            if [ "$OP" = "install" ]
            then
                install -p -o root -g wheel -m a=rx $PGMD.rc.sh /etc/rc.d/$PGMD
                useradd -c "$DAEMON_NAME" -d / -s /sbin/nologin "$DAEMON_USER"
	        rcctl set $PGMD user $DAEMON_USER
                rcctl enable $PGMD
                rcctl start $PGMD
            else
                rcctl stop $PGMD
                rcctl disable $PGMD
                rm /etc/rc.d/$PGMD
                userdel $DAEMON_USER
            fi
        else
            echo Only know how to install on systemd and rcctl Linux. Write me.
        fi
        ;;
        
    *Win* | *win*)
        # Scoop could be used here.
        echo $OS? Stop being a looser. Go to http://getfedora.org.
        ;;

    *)
        echo Unknown OS: $OS
        ;;
esac
