#!/bin/sh

usage() {
    echo "usage"
}

version_compare() {
    ota_version=$1
    current_version=$2

    current_a=$(echo "$current_version" | sed 's/V\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)/\1/')
    current_b=$(echo "$current_version" | sed 's/V\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)/\2/')
    current_c=$(echo "$current_version" | sed 's/V\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)/\3/')

    ota_a=$(echo "$ota_version" | sed 's/V\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)/\1/')
    ota_b=$(echo "$ota_version" | sed 's/V\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)/\2/')
    ota_c=$(echo "$ota_version" | sed 's/V\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)/\3/')

    if [ "$current_a" -gt "$ota_a" ]; then
        return 0
    elif [ "$current_a" -lt "$ota_a" ]; then
        return 1
    else
        if [ "$current_b" -gt "$ota_b" ]; then
            return 0
        elif [ "$current_b" -lt "$ota_b" ]; then
            return 1
        else
            if [ "$current_c" -gt "$ota_c" ]; then
                return 0
            elif [ "$current_c" -lt "$ota_c" ]; then
                return 1
            else
                return 0
            fi
        fi
    fi
}

auto_ota() {
    local device_model=$1
    local url=$(uci get ota.ota.url)
    local req_url="$url/ota/files/getLatestByDevice/"

    # Perform the curl request and capture the response
    response=$(curl -s -X POST "$req_url" -F "device_model=$device_model")

    # Extract values from the JSON response using sed
    mongoFileId=$(echo "$response" | sed -n 's/.*"mongoFileId":"\([^"]*\)".*/\1/p')
    name=$(echo "$response" | sed -n 's/.*"name":"\([^"]*\)".*/\1/p')
    downloadUrl=$(echo "$response" | sed -n 's/.*"downloadUrl":"\([^"]*\)".*/\1/p')
    md5Str=$(echo "$response" | sed -n 's/.*"md5Str":"\([^"]*\)".*/\1/p')
    deviceModel=$(echo "$response" | sed -n 's/.*"deviceModel":"\([^"]*\)".*/\1/p')
    ota_version=$(echo "$response" | sed -n 's/.*"version":"\([^"]*\)".*/\1/p')

    if [ -z "$downloadUrl" ]; then
        echo "Error: downloadUrl is empty" > /dev/console
        update_fail_config "downloadUrl is empty"
        exit 1
    fi

    current_version=$(grep '^DISTRIB_DESCRIPTION' /etc/openwrt_release | sed 's/^DISTRIB_DESCRIPTION=.*V\([0-9]\+\.[0-9]\+\.[0-9]\+\).*/V\1/')

    version_compare "$ota_version" "$current_version"
    need_upgrade=$?

    if [ $need_upgrade -eq 0 ]; then
        echo "No need to upgrade" > /dev/console
        update_fail_config "No need to upgrade"
        exit 1
    fi

    # Download the file
    rm /tmp/firmware.bin > /dev/null 2>&1
    curl -s -o /tmp/firmware.bin "$downloadUrl"
    download_result=$?

    # Check the result of the download
    if [ $download_result -eq 0 ]; then
        download_md5sum=$(md5sum /tmp/firmware.bin | awk '{print $1}')
        echo "$download_md5sum $md5Str"
        if [ "$download_md5sum" = "$md5Str" ]; then
            echo "Download successful" > /dev/console
        else
            echo "Download checksum error" > /dev/console
            update_fail_config "Download checksum error"
            rm /tmp/firmware.bin > /dev/null 2>&1
            exit 1
        fi
    else
        echo "Download failed" > /dev/console
        update_fail_config "Download failed"
        rm /tmp/firmware.bin > /dev/null 2>&1
        exit 1
    fi

    upgrade_prepare $md5Str $ota_version

    /sbin/sysupgrade /tmp/firmware.bin
}

check_updates() {
    romtype=$(uci get ota.ota.romtype)
    local url=$(uci get ota.ota.url)
    local req_url="$url/ota/files/getLatestByDevice/"

    # Perform the curl request and capture the response
    response=$(curl -s -X POST "$req_url" -F "device_model=$romtype")

    # Extract values from the JSON response using sed
    mongoFileId=$(echo "$response" | sed -n 's/.*"mongoFileId":"\([^"]*\)".*/\1/p')
    name=$(echo "$response" | sed -n 's/.*"name":"\([^"]*\)".*/\1/p')
    downloadUrl=$(echo "$response" | sed -n 's/.*"downloadUrl":"\([^"]*\)".*/\1/p')
    md5Str=$(echo "$response" | sed -n 's/.*"md5Str":"\([^"]*\)".*/\1/p')
    deviceModel=$(echo "$response" | sed -n 's/.*"deviceModel":"\([^"]*\)".*/\1/p')
    ota_version=$(echo "$response" | sed -n 's/.*"version":"\([^"]*\)".*/\1/p')

    if [ -z "$downloadUrl" ]; then
        uci set ota.ota.post_result='0'
    else
        current_version=$(grep '^DISTRIB_DESCRIPTION' /etc/openwrt_release | sed 's/^DISTRIB_DESCRIPTION=.*V\([0-9]\+\.[0-9]\+\.[0-9]\+\).*/V\1/')
        uci set ota.ota.post_result='1'
        uci set ota.ota.download_url="$downloadUrl"
        uci set ota.ota.latest_ver="$ota_version"
        uci set ota.ota.md5sum="$md5Str"
        
        version_compare "$ota_version" "$current_version"
        need_upgrade=$?

        if [ $need_upgrade -eq 0 ]; then
            uci set ota.ota.need_ota='0'
            uci commit ota
            exit 1
        else
            uci set ota.ota.need_ota='1'
        fi
    fi
    uci commit ota
    exit 1
}

download_firmware() {
    # Download the file
    rm /tmp/firmware.bin > /dev/null 2>&1
    downloadUrl=$(uci get ota.ota.download_url)
    ota_md5=$(uci get ota.ota.md5sum)
    curl -s -o /tmp/firmware.bin "$downloadUrl"
    download_result=$?

    # Check the result of the download
    if [ $download_result -eq 0 ]; then
        download_md5sum=$(md5sum /tmp/firmware.bin | awk '{print $1}')
        uci set ota.ota.download_res='1'
        if [ "$download_md5sum" = "$ota_md5" ]; then
            echo "Download successful" > /dev/console
            uci set ota.ota.check_file='1'
            uci commit ota
        else
            echo "Download checksum error" > /dev/console
            uci set ota.ota.check_file='0'
            uci commit ota
            rm /tmp/firmware.bin > /dev/null 2>&1
            exit 1
        fi
    else
        echo "Download failed" > /dev/console
        uci set ota.ota.download_res='0'
        uci set ota.ota.check_file='0'
        uci commit ota
        rm /tmp/firmware.bin > /dev/null 2>&1
        exit 1
    fi
}

do_upgrade() {
    echo 3 > /proc/sys/vm/drop_caches
    /sbin/sysupgrade /tmp/firmware.bin
}

update_fail_config() {
    uci set ota.ota_confirm_fail='setting'
    local current_time=$(date '+%Y-%m-%d %H:%M:%S')
    uci set ota.ota_confirm_fail.updatetime="$current_time"
    uci set ota.ota_confirm_fail.fail_reason="$1"

    uci commit ota
}

update_config() {
    uci set ota.ota_confirm='setting'
    local current_time=$(date '+%Y-%m-%d %H:%M:%S')
    uci set ota.ota_confirm.updatetime="$current_time"
    uci set ota.ota_confirm.md5sum="$1"
    uci set ota.ota_confirm.ota_ver="$2"

    uci commit ota
}

upgrade_prepare() {
    echo 3 > /proc/sys/vm/drop_caches
    update_config $1 $2
}

case "$1" in
    auto_ota)
        rm /tmp/firmware.bin > /dev/null 2>&1
        auto_ota_en=$(uci get ota.auto_ota.enable)
        if [ $auto_ota_en -eq 1 ]; then
            romtype=$(uci get ota.ota.romtype)
            auto_ota "$romtype"
        fi
        ;;
    check_updates)
        check_updates
        ;;
    download_firmware)
        download_firmware
        ;;
    do_upgrade)
        do_upgrade
        ;;
    upgrade_prepare)
        if [ "$#" -ne 3 ]; then
            echo "Error: Invalid number of arguments for 'update_config'"
            usage
            exit 1
        fi
        upgrade_prepare "$2" "$3"
        ;;
    *|--help|help)
        usage
        ;;
esac
