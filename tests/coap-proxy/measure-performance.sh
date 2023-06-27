#!/bin/bash

measure_proxy()
{
    ADDRESS=$1
    echo "Making proxy requests for $1"
    counter=1
    while [ $counter -le 24 ]
    do
        ts=$(date +%s%N)
        coap-client -m GET coap://[$1]/sensors/humidity -P coap://[fd00::202:2:2:2]
        echo $counter [$1]: $((($(date +%s%N) - $ts))) >> coap-result-proxy-$1.txt
        echo $counter [$1]: $((($(date +%s%N) - $ts)))
        sleep 10
        ((counter++))
    done
}

measure_normal()
{
    ADDRESS=$1
    echo "Making normal requests for $1"
    counter=1
    while [ $counter -le 24 ]
    do
        ts=$(date +%s%N)
        coap-client -m GET coap://[$1]/sensors/humidity
        echo $counter [$1]: $((($(date +%s%N) - $ts))) >> coap-result-normal-$1.txt
        echo $counter [$1]: $((($(date +%s%N) - $ts)))
        sleep 10
        ((counter++))
    done
}

while read input_text
do
    # measure_normal $input_text
    measure_proxy $input_text
done < coap-addresses.txt
