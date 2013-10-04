#!/bin/sh

COMMAND=${1}
COUNT=${2}

DEFCOUNT="2"

if [ -z "${USER}" ]; then
    echo 'need to export $USER'
    exit 1
fi
if [ -z "${COMMAND}" ]; then
    echo "usage: $(basename $0) <create [count]|delete>"
    exit 1
fi

if [ "${COMMAND}" = 'create' ]; then
    if [ -z "${COUNT}" ]; then
        COUNT="${DEFCOUNT}"
    fi

    # load kernel modules
    sudo kldload if_tap
    sudo kldload if_bridge

    # set permissions
    sudo sysctl net.link.tap.user_open=1
    sudo chown ${USER} /dev/tap0

    # create network
    echo "creating ${BRNAME} ..."
    ifconfig bridge0 create
    ifconfig bridge0 up

    for N in $(seq 0 "$((COUNT - 1))"); do
        echo "start riot for tap${N} ..."
        ifconfig tap${N} up
        ifconfig bridge0 addm tap${N}
    done

elif [ "${COMMAND}" = 'delete' ]; then
    # unload kernel modules
    sudo kldunload if_tap
    sudo kldunload if_bridge

    # reset permissions (devices already destroyed)
    sudo sysctl net.link.tap.user_open=0

else
    echo 'unknown command'
    exit 1
fi

exit 0
