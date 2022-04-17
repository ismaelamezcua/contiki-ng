CNG_PATH=$(pwd)
docker run --privileged --sysctl net.ipv6.conf.all.disable_ipv6=0 --mount type=bind,source=$CNG_PATH,destination=/home/user/contiki-ng -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v /dev/bus/usb:/dev/bus/usb -ti contiking-with-node:1.0.0
