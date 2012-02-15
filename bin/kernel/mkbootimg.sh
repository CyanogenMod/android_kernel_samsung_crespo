#!/sbin/sh
echo \#!/sbin/sh > /tmp/createnewboot.sh
PAGESIZE_HEX=`cat /tmp/boot.img-pagesize`
PAGESIZE_DEC=`printf "%d" 0x$PAGESIZE_HEX`
echo /tmp/mkbootimg --kernel /tmp/zImage --ramdisk /tmp/boot.img-ramdisk.gz --cmdline \"$(cat /tmp/boot.img-cmdline)\" --base $(cat /tmp/boot.img-base) --pagesize $PAGESIZE_DEC --output /tmp/newboot.img >> /tmp/createnewboot.sh
chmod 777 /tmp/createnewboot.sh
/tmp/createnewboot.sh
return $?
