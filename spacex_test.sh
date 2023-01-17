#!/bin/sh

spx_log_() {
	if [ -z "${TEST_LOG_:-}" ]; then
		echo "$@"
	fi
}

if [ -z $1 ];
then
	echo "\033[31mconf argument required\033[0m"
	echo "./spacex_test.sh [some.conf] [localhost:port_number]"
	exit
fi

location=($(cat $1 | grep "    location /" | tr "{" " " | sed "s/"\ \ \ \ location"//g" | sed "s/ //g"))
i=0
len=${#location[@]}

#spx_log_ "${location[*]}"
while [ $i -lt $len ];
do
	spx_log_ "${location[$i]}"
	let i++
done
read skip


i=0
cnt=0
#spx_log_ "$len"

method_list=("GET" "HEAD" "PUT" "POST" "DELETE" "SPACEX" )
len2=${#method_list[@]}
total=$((($len * $len2) - 1))
#spx_log_ "$len2"

if [ -z $2 ];
then
	host="localhost:8080"
else
	host=$2
fi
#should  check port parsing
############################
while [ $i -lt $len ];
do
	temp_uri=${location[$i]}

	j=0
	while [ $j -lt $len2 ];
	do
		echo "\n\n\n\n\n\n$cnt / $total -----------------------------"
		method=${method_list[$j]}
		request="$method $temp_uri HTTP/1.1"
		spx_log_ "\033[32m$request\033[0m"
		body=0;
		if [ $j -eq 2 ] || [ $j -eq 3 ];
		then
			body=1
		fi

		data=""
		cont_len=""
		if [ $body == 1 ];
		then
			data="-d \"hello_spacex\""
			#cont_len="-H \"Content-Length: 12\""
		fi

		resolved_method=$method
		if [ $j -eq 1 ];
		then
			resolved_method="--$method"
		else
			resolved_method="-X $method"
		fi

		error=0
		read wait
		echo ""
		curl $resolved_method -H "Content-Type: plain/text" $cont_len $data $host$temp_uri

		exit_status=$?
		if [ $exit_status -ne 0 ] && [  $exit_status -ne 143 ] ;
		then
			spx_log_ "\n exit_status: $exit_status"
			spx_log_ "\033[31mrequest [ \033[32m$request\033[0m \033[31m]  error occurred\033[0m\n\n"
			exit 1
		else
			spx_log_ "\n\033[32msuccess\033[0m"
		fi

		echo ""
		let j++
		let cnt++
	done
    let i++
done

#curl -X POST -H “Content-Type: plain/text” –data "" localhost:port/location
#curl –resolve example.com:80:127.0.0.1 http://example.com/
