#!/bin/sh

OS=`uname -s`

SELF=`basename "$0"`
PGM=hwaet
PGMD="$PGM"d
INSTALL_DIR='/usr/local'
DAEMON_NAME='Hostname Responder Daemon'
# DAEMON_USER is system specific: OpenBSD has an underscore prepended

msg()
{
    echo $SELF: $*
}

installSystemdService()
(
    DAEMON_USER="$1"
    SVC="$PGMD.service"
    INSTALLED_SVC_DIR=/usr/lib/systemd/system
    if [ "$OP" = "install" ]
    then
        # Copy pre-built service file into the system directory.
        install -p -o root -g root -m u=rwx,g=rx,o=rx "$SVC" \
            "$INSTALLED_SVC_DIR"
        # Recognize the new service file
        systemctl daemon-reload
        systemctl enable "$SVC"
        systemctl start "$SVC"
        systemctl status "$SVC"
    else
        systemctl stop "$SVC" || exit
        systemctl disable "$SVC" || exit
        rm "$INSTALLED_SVC_DIR"/"$SVC" || exit
    fi
)

#########################
#
# Main script starts here
#
#########################

if [ `whoami` != 'root' ]
then
    msg You must be root for this to work. 1>&2
    exit 1
fi

if [ "$1" = "-u" ] || [ "$1" = "uninstall" ] || [ "$1" = "--uninstall" ]
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
            useradd -g =uid -c "$DAEMON_NAME" -L daemon -d /var/empty -s \
                /sbin/nologin "$DAEMON_USER" # Ok if user exists
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
        msg Don\'t know how on $OS. Please write me.
        exit
    ;;
    
    # Mac
    Darwin)
        # This won't work. It needs to be modified for Mac.
        if [ "$OP" = "install" ]
        then
            install -sp -o root -g bin $PGM $INSTALL_DIR/bin
            install -sp -o root -g bin $PGMD $INSTALL_DIR/sbin
            install -p -o root -g wheel -m a=rx $PGMD.rc.sh /etc/rc.d/$PGMD
            if grep -q $DAEMON_USER /etc/passwd
            then
                useradd -g =uid -c "$DAEMON_NAME" -L daemon -d /var/empty -s \
                    /sbin/nologin "$DAEMON_USER"
            fi
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
        msg Recognized Linux
        DAEMON_USER="$PGMD"
        if [ "$OP" = "install" ]
        then
            install -sp -o root -g root $PGM $INSTALL_DIR/bin
            install -sp -o root -g root $PGMD $INSTALL_DIR/sbin
            # If no group is given, it should create a group for the user.
            useradd --comment "$DAEMON_NAME" --no-create-home \
                --home-dir /var/empty \
                --shell /sbin/nologin "$DAEMON_USER" 
        fi
        
        if type systemctl >/dev/null 2>&1
        then
            installSystemdService "$DAEMON_USER"
        elif type rcctl >/dev/null 2>&1
        then
            if [ "$OP" = "install" ]
            then
                install -p -o root -g wheel -m a=rx $PGMD.rc.sh /etc/rc.d/$PGMD
                rcctl set $PGMD user $DAEMON_USER
                rcctl enable $PGMD
                rcctl start $PGMD
            else
                rcctl stop $PGMD
                rcctl disable $PGMD
                rm /etc/rc.d/$PGMD
            fi
        elif [ -d /etc/init.d ] && type service >/dev/null
        then
            msg Recognized System V Init
            if [ "$OP" = "install" ]
            then
                msg Installing init.d script
                install -p -o root -g root -m u=rwx,g=rx,o=rx $PGMD.sysVinit \
                    /etc/init.d/$PGMD
                update-rc.d $PGMD defaults
                service $PGMD start
            else
                if [ -f /etc/init.d/$PGMD ]
                then
                    service $PGMD stop
                    msg Removing init.d script
                    update-rc.d $PGMD defaults-disabled
                    rm /etc/init.d/$PGMD
                else
                    msg Service already uninstalled.
                fi
            fi
            
        else
            msg Unrecognized Linux init system. Write me.
        fi
        
        if [ "$OP" = "uninstall" ]
        then
            if [ -f "$INSTALL_DIR/bin/$PGM" ]
            then
                rm "$INSTALL_DIR/bin/$PGM"
            else
                msg Client program already removed.
            fi

            if [ -f "$INSTALL_DIR/bin/$PGMD" ]
            then
                rm "$INSTALL_DIR/bin/$PGMD"
            else
                msg Daemon program already removed.
            fi

            if grep "$DAEMON_USER" /etc/passwd >/dev/null
            then
                userdel $DAEMON_USER
            else
                msg Daemon user already removed.
            fi
            # Remove mail spool file but not home directory. Doing this with
            # "userdel --remove" would remove the home directory also, but
            # it is a shared directory in this case. So don't do that.
            # Awk is needed rather than cut. Cut can use spaces or tabs as
            # delimiters but not both.
            MAIL_DIR=`grep '^MAIL_DIR' /etc/login.defs | awk '{print $2}'`
            MAIL_SPOOL="$MAIL_DIR/$DAEMON_USER"
            if [ -f "$MAIL_SPOOL" ]
            then
                rm "$MAIL_SPOOL"
            else
                msg Daemon mail spool file already removed.
            fi
        fi
        ;;
        
    *Win* | *win*)
        # Scoop could be used here.
        msg $OS? Stop being a looser. Go to http://getfedora.org.
        ;;

    *)
        msg Unknown OS: $OS
        ;;
esac
