#!/bin/bash

BOARD=crespo
DEVICEDIR=/sdcard/AROM
KNAME=kernel.cyano.RELEASE
DEVICE_CM_DIR=$ANDROID_BUILD_TOP/device/samsung/${BOARD}

WaitForDevice()
{
    adb start-server
    if [ $(adb get-state) != device -a $(adb shell busybox test -e /sbin/recovery 2> /dev/null; echo $?) != 0 ] ; then
        echo "No device is online. Waiting for one..."
        echo "Please connect USB and/or enable USB debugging"
        until [ $(adb get-state) = device -o $(adb shell busybox test -e /sbin/recovery 2> /dev/null; echo $?) = 0 ];do
            sleep 1
        done
        echo "Device Found.."
    fi
}

setup ()
{
    if [ x = "x$ANDROID_BUILD_TOP" ] ; then
        echo "Android build environment must be configured"
        exit 1
    fi
    . "$ANDROID_BUILD_TOP"/build/envsetup.sh

    KERNEL_DIR="$(dirname "$(readlink -f "$0")")"
    BUILD_DIR="$KERNEL_DIR/build"

    if [ x = "x$NO_CCACHE" ] && ccache -V &>/dev/null ; then
        CCACHE=ccache
        CCACHE_BASEDIR="$KERNEL_DIR"
        CCACHE_COMPRESS=1
        CCACHE_DIR="$BUILD_DIR/.ccache"
        export CCACHE_DIR CCACHE_COMPRESS CCACHE_BASEDIR
    else
        CCACHE=""
    fi

    CROSS_PREFIX="$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-"
}

CheckVersion ()
{
    if [ ! -f .Mayor ]
    then
        echo 1 > .Mayor
    fi
    if [ ! -f .Minor ]
    then
        echo 0 > .Minor
    fi
    sVersion=$(cat build/${BOARD}/include/config/kernel.release)
    echo $sVersion
    sVersionMerge=${sVersion:(-8)}
    echo sVersionMerge
    iMayor=$(cat .Mayor)
    iMinor=$(cat .Minor)
}

CreateKernelZip ()
{
    cd bin
    rm *.zip
    KZIPNAME=$KNAME.v$iMayor.$iMinor.$sVersionMerge.zip
    zip $KZIPNAME * -r
    if [ "$responseSend" == "y" ] ; then
        WaitForDevice
        echo Going into fastboot
        if adb reboot-bootloader ; then
            sleep 4
            echo Pushing kernel file
            if fastboot flash zimage kernel/zImage ; then
                sleep 1
                fastboot reboot
                WaitForDevice
                sleep 2
                adb root
                sleep 4
                adb remount
                sleep 2
                echo Sending modules
                for filename in system/modules/* ; do
                    echo Sending $filename to /system/modules
                    if adb push $filename /system/modules ; then
                        echo "Rebooting again"
                    fi
                done
                adb reboot
            fi
        fi
    fi
    cd ..
    echo $KZIPNAME
}

UpgradeMinor ()
{
    iMinor=$(($iMinor+1))
    echo $iMinor > .Minor
}

CompileError ()
{
    echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    echo ! COMPILACION FAILED
    echo !
    echo !
    echo ! ----------------------   COMPILACION ERROR CODE: $RET
    echo !
    echo !
    echo !
    echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

SenModulesToCMDevice()
{
    CURDIR=`pwd`;
    cd bin/system/modules
    rm $DEVICE_CM_DIR/KernelModules.mk
    echo "# Kernel Modules TO BE COPIED" > $DEVICE_CM_DIR/KernelModules.mk
    echo 'PRODUCT_COPY_FILES += \' >> $DEVICE_CM_DIR/KernelModules.mk
    MODULEF=( $(find * -type f) )
    # get length of an array
    tLen=${#MODULEF[@]}
    # use for loop read all modules but not the last
    for (( i=0; i<${tLen}-1; i++ ));
    do
      echo '    device/samsung/'${BOARD}'/'${MODULEF[$i]}':system/modules/'${MODULEF[$i]}' \' >>$DEVICE_CM_DIR/KernelModules.mk
      echo ${MODULEF[$i]}
    done
    echo '    device/samsung/'${BOARD}'/'${MODULEF[$i]}':system/modules/'${MODULEF[$i]} >>$DEVICE_CM_DIR/KernelModules.mk
    cd $CURDIR
}

build ()
{
    local target=$1
    echo "Building for $target"
    local target_dir="$BUILD_DIR/$target"
    local module
    [ x = "x$NO_RM" ]
    mkdir -p "$target_dir"
    [ x = "x$NO_DEFCONFIG" ] && mka -C "$KERNEL_DIR" O="$target_dir" ARCH=arm ${BOARD}_defconfig HOSTCC="$CCACHE gcc"
    if [ x = "x$NO_BUILD" ] ; then
        mka -C "$KERNEL_DIR" O="$target_dir" ARCH=arm HOSTCC="$CCACHE gcc" CROSS_COMPILE="$CCACHE $CROSS_PREFIX"  modules
        RET=$?
        if [[ $RET == 0 ]] ; then
            mka -C "$KERNEL_DIR" O="$target_dir" ARCH=arm HOSTCC="$CCACHE gcc" CROSS_COMPILE="$CCACHE $CROSS_PREFIX" zImage
            RET=$?
            if [[ $RET == 0 ]] ; then
                cp "$target_dir"/arch/arm/boot/zImage bin/kernel/zImage
                MODULES=( $(find $target_dir/* -type f -name *.ko) )
                for module in "${MODULES[@]}" ; do
                    echo $module
                    cp "$module" bin/system/lib/modules
                done
                # SenModulesToCMDevice
                CheckVersion
                CreateKernelZip
                UpgradeMinor
            else
                CompileError
            fi
        else
            CompileError
        fi
    fi
}

setup

echo Type y + \"intro\" to send kernel to your mobile:
read -t 10 responseSend;
if [ "$responseSend" == "y" ] ; then
    echo Sending to device after build .. $responseSend
else
    echo Only compile .. $responseSend
fi


if [ "$1" = clean ] ; then
    rm -fr "$BUILD_DIR"/*
    exit 0
fi

targets=(crespo)

START=$(date +%s)

for target in "${targets[@]}" ; do 
    build $target
done

END=$(date +%s)
ELAPSED=$((END - START))
E_MIN=$((ELAPSED / 60))
E_SEC=$((ELAPSED - E_MIN * 60))
printf "Elapsed: "
[ $E_MIN != 0 ] && printf "%d min(s) " $E_MIN
printf "%d sec(s)\n" $E_SEC
