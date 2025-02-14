#!/bin/sh

append DRIVERS "mac80211"

lookup_phy() {
	[ -n "$phy" ] && {
		[ -d /sys/class/ieee80211/$phy ] && return
	}

	local devpath
	config_get devpath "$device" path
	[ -n "$devpath" ] && {
		phy="$(iwinfo nl80211 phyname "path=$devpath")"
		[ -n "$phy" ] && return
	}

	local macaddr="$(config_get "$device" macaddr | tr 'A-Z' 'a-z')"
	[ -n "$macaddr" ] && {
		for _phy in /sys/class/ieee80211/*; do
			[ -e "$_phy" ] || continue

			[ "$macaddr" = "$(cat ${_phy}/macaddress)" ] || continue
			phy="${_phy##*/}"
			return
		done
	}
	phy=
	return
}

find_mac80211_phy() {
	local device="$1"

	config_get phy "$device" phy
	lookup_phy
	[ -n "$phy" -a -d "/sys/class/ieee80211/$phy" ] || {
		echo "PHY for wifi device $1 not found"
		return 1
	}
	config_set "$device" phy "$phy"

	config_get macaddr "$device" macaddr
	[ -z "$macaddr" ] && {
		config_set "$device" macaddr "$(cat /sys/class/ieee80211/${phy}/macaddress)"
	}

	return 0
}

check_mac80211_device() {
	config_get phy "$1" phy
	[ -z "$phy" ] && {
		find_mac80211_phy "$1" >/dev/null || return 0
		config_get phy "$1" phy
	}
	[ "$phy" = "$dev" ] && found=1
}


__get_band_defaults() {
	local phy="$1"

	( iw phy "$phy" info; echo ) | awk '
BEGIN {
        bands = ""
}

($1 == "Band" || $1 == "") && band {
        if (channel) {
		mode="NOHT"
		if (ht) mode="HT20"
		if (vht && band != "1:") mode="VHT80"
		if (he) mode="HE80"
		if (he && band == "1:") mode="HE20"
                sub("\\[", "", channel)
                sub("\\]", "", channel)
                bands = bands band channel ":" mode " "
        }
        band=""
}

$1 == "Band" {
        band = $2
        channel = ""
	vht = ""
	ht = ""
	he = ""
}

$0 ~ "Capabilities:" {
	ht=1
}

$0 ~ "VHT Capabilities" {
	vht=1
}

$0 ~ "HE Iftypes" {
	he=1
}

$1 == "*" && $3 == "MHz" && $0 !~ /disabled/ && band && !channel {
        channel = $4
}

END {
        print bands
}'
}

get_band_defaults() {
	local phy="$1"

	for c in $(__get_band_defaults "$phy"); do
		local band="${c%%:*}"
		c="${c#*:}"
		local chan="${c%%:*}"
		c="${c#*:}"
		local mode="${c%%:*}"

		case "$band" in
			1) band=2g;;
			2) band=5g;;
			3) band=60g;;
			4) band=6g;;
			*) band="";;
		esac

		[ -n "$band" ] || continue
		[ -n "$mode_band" -a "$band" = "6g" ] && return

		mode_band="$band"
		channel="$chan"
		htmode="$mode"
	done
}

detect_mac80211() {
	devidx=0
	# RM#1003581. The detection must begin after the path has been modified
	sleep 2
	config_load wireless
	while :; do
		config_get type "radio$devidx" type
		[ -n "$type" ] || break
		devidx=$(($devidx + 1))
	done

	for _dev in /sys/class/ieee80211/*; do
		[ -e "$_dev" ] || continue

		dev="${_dev##*/}"

		found=0
		config_foreach check_mac80211_device wifi-device
		[ "$found" -gt 0 ] && continue

		mode_band="g"
		channel="1"
		channels=""
		htmode=""
		ht_capab=""
		ssidprefix="-2.4G"
		noscan="1"
		band="2.4G"
		htcodex="0"
        txpower="20"
        rd_disabled="0"


		iw phy "$dev" info | grep -q 'Capabilities:' && htmode=VHT20

		iw phy "$dev" info | grep -q '5180 MHz' && {
			mode_band="a"
			channel="36"
			channels="36-165"
			ssidprefix=""
			band="5G"
            txpower="25"
            rd_disabled="1"
			iw phy "$dev" info | grep -q 'HE PHY Capabilities' && htmode="HE80"
		}

		[ -n "$htmode" ] && ht_capab="set wireless.radio${devidx}.htmode=$htmode"

		path="$(iwinfo nl80211 path "$dev")"
		if [ -n "$path" ]; then
			dev_id="set wireless.radio${devidx}.path='$path'"
		else
			dev_id="set wireless.radio${devidx}.macaddr=$(cat /sys/class/ieee80211/${dev}/macaddress)"
		fi
		[ -f "/sys/devices/factory-read/countryid" ] && {
			country=`cat /sys/devices/factory-read/countryid`
		}
		ssid=SiWiFi-`cat /sys/class/ieee80211/${dev}/macaddress | cut -c 13- | sed 's/://g'`$ssidprefix
		ssid_lease=SiWiFi-租赁-$ssidprefix`cat /sys/class/ieee80211/${dev}/macaddress | cut -c 13- | sed 's/://g'`
		if [ ! -n "$country" ]; then
			country='CN'
		fi

		txpower_lvl=2
		[ -f "/etc/ext_pa_exist" ] && {
			txpower_lvl=1
		}
		uci -q batch <<-EOF
			set wireless.radio${devidx}=wifi-device
			set wireless.radio${devidx}.type=mac80211
			set wireless.radio${devidx}.country=${country}
			set wireless.radio${devidx}.txpower_lvl=${txpower_lvl}
			set wireless.radio${devidx}.txpower=${txpower}
			set wireless.radio${devidx}.channel=${channel}
			set wireless.radio${devidx}.channels=${channels}
			set wireless.radio${devidx}.band=${band}
			set wireless.radio${devidx}.hwmode=11${mode_band}
			set wireless.radio${devidx}.noscan=${noscan}
			set wireless.radio${devidx}.netisolate=0
			set wireless.radio${devidx}.max_all_num_sta=64
			set wireless.radio${devidx}.rd_disabled=${rd_disabled}
			set wireless.radio${devidx}.ht_coex=${ht_coex}
			${dev_id}
			${ht_capab}
			set wireless.radio${devidx}.disabled=0

			set wireless.default_radio${devidx}=wifi-iface
			set wireless.default_radio${devidx}.device=radio${devidx}
			set wireless.default_radio${devidx}.network=lan
			set wireless.default_radio${devidx}.mode=ap
			set wireless.default_radio${devidx}.ssid=${ssid}
			set wireless.default_radio${devidx}.encryption=sae-mixed
			set wireless.default_radio${devidx}.key=12345678
			set wireless.default_radio${devidx}.hidden=0
			set wireless.default_radio${devidx}.ifname=wlan${devidx}
			set wireless.default_radio${devidx}.wpa_group_rekey=36000
			set wireless.default_radio${devidx}.isolate=0
			set wireless.default_radio${devidx}.group=1
			set wireless.default_radio${devidx}.disable_input=0
			set wireless.default_radio${devidx}.wps_pushbutton=1
			set wireless.default_radio${devidx}.wps_label=0
			set wireless.default_radio${devidx}.band_steering=0

			set wireless.guest_radio${devidx}=wifi-iface
			set wireless.guest_radio${devidx}.device=radio${devidx}
			set wireless.guest_radio${devidx}.network=guest
			set wireless.guest_radio${devidx}.mode=ap
			set wireless.guest_radio${devidx}.ssid=${ssid}-guest
			set wireless.guest_radio${devidx}.encryption=none
			set wireless.guest_radio${devidx}.hidden=0
			set wireless.guest_radio${devidx}.ifname=wlan${devidx}-guest
			set wireless.guest_radio${devidx}.isolate=1
			set wireless.guest_radio${devidx}.group=1
			set wireless.guest_radio${devidx}.netisolate=0
			set wireless.guest_radio${devidx}.disable_input=0
			set wireless.guest_radio${devidx}.disabled=1
EOF
		uci -q commit wireless

		devidx=$(($devidx + 1))
	done
}
