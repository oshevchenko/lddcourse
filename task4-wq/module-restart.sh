rmmod chardev
make clean
make 
insmod chardev.ko
mknod /dev/chardev c 245 0
mknod /dev/chardev1 c 245 1
mknod /dev/chardev2 c 245 2
sleep 3
echo 000 > /dev/chardev
#sleep 1
#echo 111 > /dev/chardev1
#sleep 1
#echo 2222 > /dev/chardev2
