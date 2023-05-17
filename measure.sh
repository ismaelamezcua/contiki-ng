#!/bin/bash
ts=$(date +%s%N)
$@
echo $((($(date +%s%N) - $ts)/1000000))

