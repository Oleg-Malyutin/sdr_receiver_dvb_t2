#compile linux (in linux dir)

make ARCH=arm CROSS_COMPILE=/usr/local/bin/gcc-linaro-7.2.1-2017.11-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-



#compile module

make KERNEL=./linux CROSS=/usr/local/bin/gcc-linaro-7.2.1-2017.11-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
