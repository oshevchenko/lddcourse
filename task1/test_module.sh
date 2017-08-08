#!/bin/sh
MODULE_NAME="task1"

sudo insmod $MODULE_NAME.ko print_count=5
result=$(sudo lsmod | grep -o $MODULE_NAME)
if [ "$result" = "$MODULE_NAME" ]; then
    echo "\033[32mModule '$MODULE_NAME' loaded OK.\033[0m"
else
    echo "\033[31mModule '$MODULE_NAME' NOT LOADED!\033[0m"
    exit 1
fi

message=$(dmesg | grep $MODULE_NAME | tail -1)
echo "\033[33m$message\033[0m"
result=$(echo $message | grep "^.*[^(\!)]!!!!!$")
if [ "$result" = "" ]; then
    echo "\033[31mMessage is wrong!\033[0m"
else
    echo "\033[32mMessage is OK!\033[0m"
fi

sudo rmmod $MODULE_NAME
result=$(sudo lsmod | grep -o $MODULE_NAME)
if [ "$result" = "" ]; then
    echo "\033[32mModule '$MODULE_NAME' unloaded OK.\033[0m"
else
    echo "\033[31mModule '$MODULE_NAME' NOT UNLOADED!\033[0m"
    exit 1
fi
echo "\033[33m$(dmesg | grep $MODULE_NAME | tail -1)\033[0m"
