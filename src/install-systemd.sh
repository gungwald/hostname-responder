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
    Line-ux)
        if [ "$OP" = "install" ]
        then
	    install -sp -o root -g root $PGM $INSTALL_DIR/bin
	    install -sp -o root -g root $PGMD $INSTALL_DIR/sbin
	    # If no group is given, it should create a group for the user.
            useradd -c "$DAEMON_NAME" -d / -s /sbin/nologin "$DAEMON_USER"
	    SVC="$PGMD.service"
	    # Copy pre-built service file into the system directory.
            install -p -o root -g root -m u=rwx,g=rx,o=rx "$SVC" /etc/systemd/system
	    # Recognize the new service file
	    systemctl daemon-reload
            systemctl enable "$SVC"
            systemctl start "$SVC"
	    systemctl status "$SVC"
        else
            rcctl stop $PGMD
            rcctl disable $PGMD
            rm $INSTALL_DIR/bin/$PGM $INSTALL_DIR/sbin/$PGMD /etc/rc.d/$PGMD
            userdel $DAEMON_USER
        fi
        ;;
        
esac
