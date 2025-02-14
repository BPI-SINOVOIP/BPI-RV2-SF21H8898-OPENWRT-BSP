#!/bin/sh
LOG_FILE=/tmp/storage_test.log
SPEED_LOG=/tmp/storage_speed.log

rand(){
        min=$1
        max=$(($2-$min+1))
        num=$(cat /proc/sys/kernel/random/uuid | sed "s/[a-zA-Z]//g" | awk -F '-' '{printf"%d\n",$1+$4}')
        echo $((num%max+min))
}

rand_filename(){
        length=$(rand $1 $2)
        head /dev/urandom | tr -dc a-z0-9 > /tmp/pre.log
        prefix=$(head -c $length /tmp/pre.log)

        length=$(rand $3 $4)
        head /dev/urandom | tr -dc a-z0-9 > /tmp/suf.log
        suffix=$(head -c $length /tmp/suf.log)

        temp_filename=$prefix.$suffix

        echo $temp_filename
}

gen_random_path(){
        path=$1
        path_depth=$(rand 0 20)
        i=0

        while [ $((i)) -lt $((path_depth)) ]; do
                randname=$(rand_filename 1 10 1 10)
                path=${path}/$randname
                i=$(($i+1))
        done
        if [ ! -d $path ]; then
                mkdir -p $path
        fi
        echo $path
}

do_one_test(){
	file=$(rand_filename 1 10 1 10)
	src=$(gen_random_path $1)/$file
	dst=$(gen_random_path $2)/$file
	TMP=/tmp/tmp-${$}
	file_len_KB=$(rand 0 $(($3-1)))
	file_len_B=$(rand 1 200)
	CMD=cp

	dd if=/dev/urandom of=${src} bs=1k count=${file_len_KB} > /dev/null 2>&1
	dd if=/dev/urandom of=${TMP} bs=${file_len_B} count=1 > /dev/null 2>&1
	dd if=${TMP} of=${src} seek=${file_len_KB} obs=1k > /dev/null 2>&1
	rm ${TMP}
	echo "$(date "+%H:%M:%S")  $CMD $((file_len_KB*200+file_len_B)) bytes data..." >> $LOG_FILE

	o_md5=$(md5sum $src | awk '{print $1}')
	origin_md5=$(cut -f 1 $src)
	$CMD $src $dst >> $LOG_FILE
	sync
	n_md5=$(md5sum $dst | awk '{print $1}')
	new_md5=$(cut -f 1 $dst)
	if [ "$new_md5" != "$origin_md5" ]; then
		echo "transmission error:" >> $LOG_FILE
		echo "origin_md5=$origin_md5" >> $LOG_FILE
		echo "new_md5=$new_md5" >> $LOG_FILE
		echo "cmp: $(cmp $src $dst)" >> $LOG_FILE
		return 1
	fi
	echo "$(date "+%H:%M:%S")  success" >> $LOG_FILE
	rm -rf ${1} ${2}
	return 0
}

get_current_time(){
	echo $(cat /proc/uptime | awk '{print $1}')
}

do_speed_test(){
    sync
    echo 3 > /proc/sys/vm/drop_caches
	start_time=$(get_current_time)
	dd if=$1 of=$2 count=200 bs=1M $3
	end_time=$(get_current_time)
	awk -v a=$start_time -v b=$end_time 'BEGIN {print 200/(b-a)}'
}

speed_test(){
    write_speed=$(do_speed_test /dev/zero ${1}/test.usb conv=fsync)
    echo "write speed = $write_speed MB/s" | tee -a $SPEED_LOG
	read_speed=$(do_speed_test ${1}/test.usb /dev/null)
	echo "read speed = $read_speed MB/s" | tee -a $SPEED_LOG
    rm ${1}/test.usb
	return 0
}

check(){
	errors=$(grep "error" $1)
	if [ "$errors" != "" ]; then
		echo "test fail"
	else
		echo "test success"
	fi
}

case $1 in
start)
    if [ $# -lt 5 ]; then
		echo "too less parameters!!!"
	else
		echo "start storage_test" | tee -a $LOG_FILE
	fi
    echo "$$" > /tmp/storage_test_pid
	index=0
	while [ $index -lt $5 ]
	do
		do_one_test $2 $3 $4
		if [ $? -ne 0 ]; then
			echo "stop test" >> $LOG_FILE
			break
		fi
		index=$(($index+1))
	done
	check $LOG_FILE
;;
speed)
	speed_test $2
	if [ $? -ne 0 ]; then
		echo "error: speed test failed" | tee -a $SPEED_LOG
	else
		echo "success: speed test success" | tee -a $SPEED_LOG
	fi
;;
stop)
	rm -r /src /dst
	pid=$(cat /tmp/storage_test_pid)
	echo "kill $pid"
	kill $pid
;;
*)
	echo "invalid command"
esac

if [ "$2" = "start" ];then
	pid=$(cat /tmp/storage_test_pid)
	echo "kill $pid"
	kill $pid
fi