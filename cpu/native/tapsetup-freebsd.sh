#!/bin/sh

ifconfig bridge0 create
echo "start riot instances"
ifconfig bridge0 up
ifconfig tap0 up
ifconfig tap1 up
ifconfig bridge0 addm tap0 addm tap1
