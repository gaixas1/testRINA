#!/bin/sh
# point-to-point || Server B
# eth1 -> Server A


ip link add link eth1 name eth1.100 type vlan id 100
ip link set dev eth1 up
ip link set dev eth1.100 up

modprobe shim-eth-vlan
modprobe rina-default-plugin
modprobe normal-ipcp

/usr/local/irati/bin/ipcm -c ./p2pd/ipcmanager_B.conf
