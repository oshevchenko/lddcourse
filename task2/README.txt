chardev_minor.c: Creates char device. Read/Write access.

/dev/chardev   dev read/write device device
/proc/hello_proc -proc readonly device(return last enterred line to any of chardev device)
                  Stores only last write does not matter what minor was used to write


# echo 2222 >/dev/hello2 
# cat /proc/hello_proc 
2222
# cat /dev/hello2 
2222
# echo 1111 >/dev/hello1 
# cat /dev/hello1
1111
# cat /proc/hello_proc 
1111
