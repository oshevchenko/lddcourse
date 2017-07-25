sudo insmod chardev.ko

root@tarask:/dev# mknod /dev/chardev c 244 0
root@tarask:/dev# mknod /dev/chardev c 244 1
root@tarask:/dev# mknod /dev/chardev c 244 2

root@tarask:/dev# echo 1234 >  chardev
root@tarask:/dev# echo 3333 >  chardev1
root@tarask:/dev# echo 2222 >  chardev2


root@tarask:/dev# cat chardev2
Entered data is = 2222

root@tarask:/dev# cat chardev
Entered data is = 1234

root@tarask:/dev# cat chardev1
Entered data is = 3333

tarask@tarask:~$ cat /proc/chardev 
1234
3333
2222

