#!/bin/sh

exec > /dev/null 2>&1

for i in $(seq 0 7)
do
    echo subnet is_get 0 is_lan 0 index $i ifname "" > /proc/dpns_nat
done

i=0

for intf in "wan" "wan1" "wan2" "wan3" "wwan"
do
    line=$(uci show network | grep "network.$intf.disabled=")
    if [ -z "$line" ]; then
        disable=0
    else
        disable=$(uci get network.$intf.disabled)
    fi

    if [ $disable == 0 ]; then
        device=$(uci get network.$intf.device)
        echo subnet is_get 0 is_lan 0 index $i ifname $device > /proc/dpns_nat
        i=$((i+1))
    fi
done