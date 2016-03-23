#!/bin/sh

if [ $UID -ne 0 ]; then
    echo Please run as root.
    exit
fi

if [ $# -ne 1 ]; then
    echo "usage:$0 [auto|device]"
    exit
fi

if [ "x$1" = "xauto" ]; then
    boot_part=$(mount | grep /media/*/boot | awk '{print $3}')
    echo "boot partition:$boot_part"
    if [ -z $boot_part ] ; then
        echo "fail to find sdcard boot partition"
        exit 1
    fi
    device=`mount | grep ${boot_part}`
    device=${device:5:3}

    cp -fv out/sun8iw7p1/linux/common/uImage $boot_part
    cp -fv tools/pack/out/script.bin $boot_part/script.bin
    rsync -a --exclude=".gitignore" ./script $boot_part
else
    device=$1
    device=${device:5:3}
fi

echo "sdcard:$device"
cd ./tools/pack > /dev/null
./fuse_uboot.sh /dev/${device}
cd - > /dev/null

#eject /dev/${device}
