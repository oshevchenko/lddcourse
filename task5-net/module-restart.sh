rmmod snull
make clean
make 
insmod snull.ko
ifconfig sn0 local0
ifconfig sn1 local1
ping -c2 remote1
