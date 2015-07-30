#!/bin/sh
# point-to-point || Server B
# eth1 -> Server A
INSTALLATION_PATH= "/usr/local/irati"


ip link add link eth1 name eth1.100 type vlan id 100
ip link set dev eth1 up
ip link set dev eth1.100 up

modprobe shim-eth-vlan
modprobe rina-default-plugin
modprobe normal-ipcp

$INSTALLATION_PATH/bin/ipcm -c ./p2p/ipcmanager_B.conf
