#!/bin/bash
#Initial wifi interface configuration
ifconfig $1 up 10.0.0.1 netmask 255.255.255.0
sleep 2
###########Start DHCP, comment out / add relevant section##########
#Thanks to Panji
#Doesn't try to run dhcpd when already running
if [ "$(ps -e | grep dhcpd)" == "" ]; then
dhcpd $1 -cf /etc/coffee_machine_dhcpd.conf&
fi
###########
#Enable NAT
#RP Disable NAT
#iptables --flush
#iptables --table nat --flush
#iptables --delete-chain
#iptables --table nat --delete-chain
#iptables --table nat --append POSTROUTING --out-interface $2 -j MASQUERADE
#iptables --append FORWARD --in-interface $1 -j ACCEPT
 
#Thanks to lorenzo
#Uncomment the line below if facing problems while sharing PPPoE, see lorenzo's comment for more details
#iptables -I FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu
 
#sysctl -w net.ipv4.ip_forward=1
#start hostapd
hostapd /etc/hostapd/coffee_machine_hostapd.conf 1>/dev/null
killall dhcpd
