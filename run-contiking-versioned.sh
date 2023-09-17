CNG_PATH=$(pwd)
IMAGE_VERSION=fc6fffa82f19351e3de2135d5a1eeff22e4ba1322f53200a8a3d81b40856aeac
docker run --privileged --sysctl net.ipv6.conf.all.disable_ipv6=0 --mount type=bind,source=$CNG_PATH,destination=/home/user/contiki-ng -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v /dev/bus/usb:/dev/bus/usb -ti contiker/contiki-ng:$IMAGE_VERSION
