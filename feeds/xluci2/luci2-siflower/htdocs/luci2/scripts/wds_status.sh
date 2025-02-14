#!/bin/sh


set_config(){
        device=`uci get wireless.wds.device`
        oc=`uci get wireless.$device.origin_channel`
        uci set wireless.$device.channel=$oc
        uci del wireless.wds
        ip=`uci get network.lan.oip`
        uci set network.lan.proto='static'
        uci set network.lan.ipaddr=$ip
        uci set network.lan.netmask='255.255.255.0'
        uci set network.lan.ip6assign='60'
        uci commit
        /etc/init.d/network restart
}

check_lan_ip(){
        sleep 5
        ip=`ubus call network.interface.lan status | grep "ipv4-address" -C 3 | grep '"address"' | awk -F '"' '{print $4}'`
        if [ "$ip" == "" ] ;then
                set_config
        else
                /etc/init.d/dnsmasq stop
    fi
}

if [ "$1" != "restart" ]; then
        sleep 5
        mode=`iwinfo | grep -C 3 $1 | grep Mode | awk -F ' ' '{print$2}'`

        if [ "$mode" != "Unknown" ]; then
                #brctl addif br-lan $1
                check_lan_ip
        else
                wifi
                sleep 5
                mode2=`iwinfo | grep -C 3 $1 | grep Mode | awk -F ' ' '{print$2}'`
                if [ $mode2 != "Unknown" ]; then
                        #brctl addif br-lan $1
                        check_lan_ip
                else
                        set_config
                fi
        fi
elif [ "$1" == "restart" ]; then
        sleep 5
        ifnames=`uci get wireless.wds.ifname`
        mode=`iwinfo | grep -C 3 $ifnames | grep Mode | awk -F ' ' '{print$2}'`
        if [ "$mode" != "Unknown" ]; then
                #brctl addif br-lan $ifnames
        fi
fi
