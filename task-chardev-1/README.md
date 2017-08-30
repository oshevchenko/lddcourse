Module parameters example
=========================

```
make
insmod test_cdev.ko test_int=123

ls -l /sys/module/test_cdev/parameters

    total 0
    -r-------- 1 root root 4096 сер 31 01:16 test_long
    -rw-rw---- 1 root root 4096 сер 31 01:16 test_short
    -rw-r--r-- 1 root root 4096 сер 31 01:16 test_string

echo -n abc > /sys/module/test_cdev/parameters/test_string
rmmod test_cdev
```

dmesg
-----
[11114.219589] TEST chardev: module init, data: 2
[11114.219589] TEST chardev: short: 10
[11114.219590] TEST chardev: int: 123
[11114.219590] TEST chardev: long: 30
[11114.219591] TEST chardev: string: a
[11143.458668] TEST chardev: goodbye short: 10
[11143.458668] TEST chardev: goodbye long: 30
[11143.458669] TEST chardev: goodbye string: abc
[11143.458669] TEST chardev: module deinit

