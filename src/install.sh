#!/bin/sh

OS=`uname -s`

PGMD=hwaetd
INSTALL_DIR='/usr/local'
DAEMON_USER="_$PGMD"
DAEMON_NAME='Hostname Responder Daemon'

if [ "$1" = "-u" ]
then
    OP=uninstall
else
    OP=install
fi

case $OS in
    OpenBSD)
        if [ "$OP" = "install" ]
        then
	    install -sp -o root -g bin $PGM) $INSTALL_DIR/bin
	    install -sp -o root -g bin $PGMD $INSTALL_DIR/sbin
	    install -p -o root -g wheel -m a=rx $PGMD.rc.sh /etc/rc.d/$PGMD
            useradd -g =uid -c "$DAEMON_NAME" -L daemon -d /var/empty -s /sbin/nologin "$DAEMON_USER"
	    rcctl set $PGMD user $DAEMON_USER
        else
            rcctl stop $PGMD
            rcctl disable $PGMD
            rm $INSTALL_DIR/bin/$PGM $INSTALL_DIR/sbin/$PGMD /etc/rc.d/$PGMD
            userdel $DAEMON_USER
        fi
        ;;
        
esac
