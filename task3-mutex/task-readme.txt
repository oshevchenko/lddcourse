0. Added mutex

1. Script for recompiling and reloading chardev.ko

root@tarask-u1610:/home/tarask/git/lddcourse/lddcourse/task3-mutex# cat module-restart.sh 
rmmod chardev
make clean
make 
insmod chardev.ko
mknod /dev/chardev c 245 0
mknod /dev/chardev1 c 245 1
mknod /dev/chardev2 c 245 2
sleep 3
echo 000 > /dev/chardev
sleep 1
echo 111 > /dev/chardev1
sleep 1
echo 2222 > /dev/chardev2

2. Script for reading data from chardev node file

root@tarask-u1610:/home/tarask/git/lddcourse/lddcourse/task3-mutex# cat cat-chardev.sh 
cat /dev/chardev &
cat /dev/chardev1 &
cat /dev/chardev2 &
cat /dev/chardev &
cat /dev/chardev1 &
cat /dev/chardev2 &
cat /dev/chardev &
cat /dev/chardev1 &
cat /dev/chardev2 &
cat /dev/chardev &
cat /dev/chardev &
cat /dev/chardev &
cat /dev/chardev &
cat /dev/chardev &
cat /dev/chardev &




Results
root@tarask-u1610:/home/tarask/git/lddcourse/lddcourse/task3-mutex# ./cat-chardev.sh 


Entered data is chardev[0] = 000

Entered data is chardev[1] = 111

Entered data is chardev[2] = 2222

Entered data is chardev[1] = 111

Entered data is chardev[0] = 000

Entered data is chardev[2] = 2222

Entered data is chardev[0] = 000

Entered data is chardev[0] = 000

Entered data is chardev[1] = 111

Entered data is chardev[0] = 000

root@tarask-u1610:/home/tarask/git/lddcourse/lddcourse/task3-mutex# cat /proc/chardev 
 chardev=000

 chardev1=111

 chardev2=2222


