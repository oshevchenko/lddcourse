sudo insmod chardev.ko

root@tarask:/dev# mknod /dev/chardev c 244 0
root@tarask:/dev# mknod /dev/chardev c 244 1
root@tarask:/dev# mknod /dev/chardev c 244 2

root@tarask-u1610:/dev# echo 00000 >  chardev
root@tarask-u1610:/dev# echo 111 >  chardev1
root@tarask-u1610:/dev# echo 222222 >  chardev2
root@tarask-u1610:/dev# cat chardev
Entered data is = 00000

root@tarask-u1610:/dev# cat chardev2
Entered data is = 222222

root@tarask-u1610:/dev# cat chardev1
Entered data is = 111

root@tarask-u1610:/dev# cat /proc/chardev 
  chardev=00000

 chardev1=111

 chardev2=222222

root@tarask-u1610:/dev# 


