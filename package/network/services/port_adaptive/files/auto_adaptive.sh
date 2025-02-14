#!/bin/sh

numbers="0 1 2 3 4.1 5"
port_str="0 1 2 3 5t"

board_name() {
	[ -e /tmp/sysinfo/board_name ] && cat /tmp/sysinfo/board_name
}

dinit() {
	cp /tmp/adapt_network /etc/config/network
	rm /tmp/adapt_network
}

kill_all() {
	pid_fd=`ps | grep "send" | grep "eth" | grep -v "grep" | awk '{print $1}'`
	kill $pid_fd
}

init_switch_vlan(){
	local id="$1"

	echo $id

	a=`uci add network switch`
	uci set network.$a.name=switch"$id"
	uci set network.$a.reset="1"
	uci set network.$a.enable_vlan="1"

	for i in `seq 1 4`
	do
		a=`uci add network switch_vlan`
		uci set network.$a.device=switch"$id"
		uci set network.$a.vlan="$i"
		uci set network.$a.ports="`expr $i - 1` 5t"
	done
}

sgmii_init() {
	#eth0
	for i in `seq 1 4`
	do
		uci set network.wan"$i"='interface'
		uci set network.wan"$i".device=eth0."$i"
		uci set network.wan"$i".proto=dhcp
	done

	init_switch_vlan 0

	# eth1
	for i in `seq 5 8`
	do
		uci set network.wan"$i"='interface'
		uci set network.wan"$i".device=eth1."`expr $i - 4`"
		uci set network.wan"$i".proto=dhcp
	done

	init_switch_vlan 1

	# eth2
	uci set network.wan9='interface'
	uci set network.wan9.device=eth2
	uci set network.wan9.proto=dhcp
}

demo_init() {
	for i in `seq 1 4`
	do
		uci set network.wan"$i"='interface'
		uci set network.wan"$i".device=eth0."$i"
		uci set network.wan"$i".proto=dhcp
	done

	init_switch_vlan 0
}

evb_init(){
	for i in 0 1 2 3 5; do
		uci set network.wan"$i"='interface'
		uci set network.wan"$i".device=eth"$i"
		uci set network.wan"$i".proto=dhcp
	done

	for i in `seq 1 4`
	do
		uci set network.wan"$((i+5))"='interface'
		uci set network.wan"$((i+5))".device=eth4."$i"
		uci set network.wan"$((i+5))".proto=dhcp
	done

	init_switch_vlan 0
}


sgmii_checkport() {
	for i in `seq 1 4`
	do
		{
			send eth0."$i" &
		}&
	done

	for i in `seq 1 4`
	do
		{
			send eth1."$i" &
		}&
	done

	{
		send eth2 &
	}&
}

demo_checkport(){
	for i in `seq 1 4`
	do
		{
			send eth0."$i" &
		}&
	done
}

evb_checkport(){
	for i in 0 1 2 3 5
	do
		{
			send eth"$i" &
		}&
	done
	for i in `seq 1 4`
	do
		{
			send eth4."$i" &
		}&
	done
}

sgmii_set_wan_04() {
	i=$(echo "$1" | awk -F'.' '{print $1}')
	d=$(echo "$1" | awk -F'.' '{print $2}')

	wan_mac=$(uci get network.@device[-1].macaddr)

	uci delete network.@switch_vlan[2]
	uci delete network.@switch_vlan[1]
	uci delete network.@switch_vlan[0]

	uci delete network.@device[6]
	uci delete network.@device[5]

	uci delete network.@device[0].ports

	uci add_list network.@device[0].ports=eth0.1
	uci add_list network.@device[0].ports=eth1.1
	uci add_list network.@device[0].ports=eth0.1
	uci add_list network.@device[0].ports=eth1.1
	uci add_list network.@device[0].ports=eth2

	lan_mac=$(uci get network.@device[-1].macaddr)

	port_n=`expr $d - 1`
	port_lan=${port_str/$port_n/''}

	if [ "$i" -eq 0 ]; then
		a=`uci add network switch_vlan`
		uci set network.$a.device="switch0"
		uci set network.$a.vlan=1
		uci set network.$a.ports="$port_lan"

		b=`uci add network switch_vlan`
		uci set network.$b.device="switch0"
		uci set network.$b.vlan=2
		uci set network.$b.ports="$port_n 5t"

		c=`uci add network switch_vlan`
		uci set network.$c.device="switch1"
		uci set network.$c.vlan=1
		uci set network.$c.ports="$port_str"

		uci set network.wan.device=eth0.2
		uci set network.wan6.device=eth0.2

		uci add network device
		uci set network.@device[-1].name=eth2
		uci set network.@device[-1].macaddr=$lan_mac

		uci add network device
		uci set network.@device[-1].name=eth0.2
		uci set network.@device[-1].macaddr=$wan_mac
	else
		a=`uci add network switch_vlan`
		uci set network.$a.device="switch0"
		uci set network.$a.vlan=1
		uci set network.$a.ports="$port_str"

		b=`uci add network switch_vlan`
		uci set network.$b.device="switch1"
		uci set network.$b.vlan=1
		uci set network.$b.ports="$port_lan"

		c=`uci add network switch_vlan`
		uci set network.$c.device="switch1"
		uci set network.$c.vlan=2
		uci set network.$c.ports="$port_n 5t"

		uci set network.wan.device=eth1.2
		uci set network.wan6.device=eth1.2

		uci add network device
		uci set network.@device[-1].name=eth2
		uci set network.@device[-1].macaddr=$lan_mac

		uci add network device
		uci set network.@device[-1].name=eth1.2
		uci set network.@device[-1].macaddr=$wan_mac
	fi
}

sgmii_set_wan_2(){
	uci set network.wan.device=eth2
	uci set network.wan6.device=eth2

	wan_mac=$(uci get network.@device[-1].macaddr)

	uci delete network.@device[6]
	uci delete network.@device[5]

	uci del_list network.@device[0].ports=eth2
	uci delete network.@switch_vlan[2]
	uci delete network.@switch_vlan[1]
	uci delete network.@switch_vlan[0]

	a=`uci add network switch_vlan`
		uci set network.$a.device="switch0"
		uci set network.$a.vlan=1
		uci set network.$a.ports="$port_str"

	b=`uci add network switch_vlan`
		uci set network.$b.device="switch1"
		uci set network.$b.vlan=1
		uci set network.$b.ports="$port_str"

	uci add network device
	uci set network.@device[-1].name=eth2
	uci set network.@device[-1].macaddr=$wan_mac
}

evb_set_wan_4() {
	uci delete network.@device[0].ports
	uci delete network.@switch_vlan[1]
	uci delete network.@switch_vlan[0]

	port_n=`expr $1 - 1`' '
	port_lan=${port_str/$port_n/''}

	for i in 0 1 2 3 5
	do
		uci add_list network.@device[0].ports=eth$i
	done
	uci add_list network.@device[0].ports=eth4.1

	uci set network.wan.device=eth4.2
	uci set network.wan6.device=eth4.2

	a=`uci add network switch_vlan`
	uci set network.$a.device="switch0"
	uci set network.$a.vlan=1
	uci set network.$a.ports="$port_lan"

	b=`uci add network switch_vlan`
	uci set network.$b.device="switch0"
	uci set network.$b.vlan=2
	uci set network.$b.ports="`expr $1 - 1` 5t "
}

evb_set_wan_phy() {
	uci delete network.@device[0].ports
	uci delete network.@switch_vlan[1]

	for number in $numbers; do
	if [ "$number" = "$1" ];then
		uci set network.wan.device=eth$1
		uci set network.wan6.device=eth$1
	else
		uci add_list network.@device[0].ports=eth$number
	fi
	done

	uci set network.@switch_vlan[0].ports="$port_str"
}

sgmii_set_wan(){
	if [[ "$1" =~ [.] ]]; then
		sgmii_set_wan_04 $1
	else
		if [ "$1" -ne 6 ];then
		sgmii_set_wan_2 $1
		fi
	fi
}

demo_set_wan() {
	if [[ "$1" =~ [.] ]]; then
		d=$(echo "$1" | awk -F'.' '{print $2}')
		port_n=`expr $d - 1`
		port_lan=${port_str/$port_n/''}
		uci set network.@switch_vlan[0].ports="$port_lan"
		uci set network.@switch_vlan[1].ports="$port_n 5t"
	fi
}

evb_set_wan(){
	if [[ "$1" =~ [.] ]]; then
		d=$(echo "$1" | awk -F'.' '{print $2}')
		echo "$d"
		evb_set_wan_4 $d
	else
		if [ "$1" -ne 6 ];then
		evb_set_wan_phy $1
		fi
	fi
}

init(){
	local board="$1"

	cp /etc/config/network /tmp/adapt_network
	echo " " > /etc/config/network

	case $board in
	siflower,sf21h8898-evb)
		evb_init
		;;
	siflower,sf21h8898-demo)
		demo_init
		;;
	siflower,sf21h8898-sgmii-evb)
		sgmii_init
		;;
	esac

	uci commit network
}

checkport(){
	local board="$1"

	case $board in
	siflower,sf21h8898-evb)
		evb_checkport
		;;
	siflower,sf21h8898-demo)
		demo_checkport
		;;
	siflower,sf21h8898-sgmii-evb)
		sgmii_checkport
		;;
	esac

	wait
	sleep 4
	kill_all
}

set_wan(){
	local board="$1"
	local port="$2"

	case $board in
	siflower,sf21h8898-evb)
		evb_set_wan $port
		;;
	siflower,sf21h8898-demo)
		demo_set_wan $port
		;;
	siflower,sf21h8898-sgmii-evb)
		sgmii_set_wan $port
		;;
	esac

	uci commit network
}

start(){
	board=$(board_name)
	echo $board
	init $board
	a=6
	echo "init success"
	/etc/init.d/network reload &
	wait
	sleep 2
	for j in `seq 1 5`
	do
		echo "start check"
		checkport $board
		if [ -f "/tmp/dhcp_iface" ]; then
			a=`cat /tmp/dhcp_iface | awk -F 'eth' '{print $2}'`
			rm /tmp/dhcp_iface
			echo "$a"
			break
		fi
	done
	dinit
	set_wan $board $a
}

case $1 in
	start)
		start
		;;
	init)
		init
		;;
	dinit)
		dinit
		;;
	*)
		echo "commod error"
		;;
esac