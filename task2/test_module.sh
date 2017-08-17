#!/bin/sh
MODULE_NAME="task2"
MAJOR_NUM=0

echo "\033[32mBEGIN - TEST OF MODULE $MODULE_NAME\033[0m\n"

# Check if module already loaded - then unload
result=$(sudo lsmod | grep -o $MODULE_NAME)
if [ "$result" = "$MODULE_NAME" ]; then
    sudo rmmod $MODULE_NAME
    echo "\033[32mModule '$MODULE_NAME' loaded - unloading.\033[0m"
fi

echo "\033[35mInserting module.\033[0m"
sudo insmod $MODULE_NAME.ko major_num=$MAJOR_NUM
result=$(sudo lsmod | grep -o $MODULE_NAME)
if [ "$result" = "$MODULE_NAME" ]; then
    echo "\033[32mModule '$MODULE_NAME' loaded OK.\033[0m"
else
    echo "\033[31mModule '$MODULE_NAME' NOT LOADED!\033[0m"
    exit 1
fi

MAJOR_NUM=$(dmesg | grep -oP "Registered device for /dev/$MODULE_NAME with major=\K[0-9]*" | tail -1)

if [ -e "/dev/$MODULE_NAME" ]; then
    echo "\033[35mRemoving old node /dev/$MODULE_NAME\033[0m"
    sudo rm /dev/$MODULE_NAME
fi

echo "\033[35mMaking node for a device (/dev/$MODULE_NAME)\033[0m"
sudo mknod /dev/$MODULE_NAME c $MAJOR_NUM 0
sudo chmod a+w /dev/$MODULE_NAME

echo "\033[35mWriting to /dev/$MODULE_NAME to save the backtrace\033[0m"
echo 1 > /dev/$MODULE_NAME

message=$(dmesg | grep $MODULE_NAME | tail -1)
echo "\033[33m$message\033[0m"
result=$(echo $message | grep "Saved backtrace of the task")
if [ "$result" = "" ]; then
    echo $result
    echo "\033[31mResult ERROR!\033[0m"
else
    echo "\033[32mResult OK.\033[0m"
fi

echo "\033[35mPrinting saved backtrace\033[0m"
message=$(cat /dev/$MODULE_NAME)
cat /dev/$MODULE_NAME

result=$(echo $message | grep "Backtrace for")
if [ "$result" = "" ]; then
    echo "\033[31mResult ERROR!\033[0m"
else
    echo "\033[32mResult OK.\033[0m"
fi

sudo rmmod $MODULE_NAME
result=$(sudo lsmod | grep -o $MODULE_NAME)
echo "\033[33m$(dmesg | grep $MODULE_NAME | tail -1)\033[0m"
if [ "$result" = "" ]; then
    echo "\033[32mModule '$MODULE_NAME' unloaded OK.\033[0m"
else
    echo "\033[31mModule '$MODULE_NAME' NOT UNLOADED!\033[0m"
    exit 1
fi

echo "\033[32m\nEND - TEST OF MODULE $MODULE_NAME\033[0m"
