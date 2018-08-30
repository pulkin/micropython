#!/bin/bash


######################## 1 ################################
# check path
curr_path_abs=$(cd `dirname $0`; pwd)
curr_folder_name="${curr_path_abs##*/}"
echo $curr_path_abs
if [[ "${curr_folder_name}" != "gprs_a9" ]]; then
    echo "Plese exec build.sh in gprs_a9 folder"
    exit 1
fi
###########################################################

function clean_CSDK_lib()
{
    echo ">> Clean CSDK lib now"
    cd ../../  #root path of micropython project
    cd lib/GPRS_C_SDK/platform/tools/genlib
    ./genlib.sh clean
    cd ${curr_path_abs}
}

######################### 2 ###############################
# check parameters
CFG_RELEASE=debug
if [[ "$1xx" == "releasexx" ]]; then
    CFG_RELEASE=release
elif [[ "$1xx" == "cleanxx" ]]; then
    echo ">> Clean build & hex"
    rm -rf build
    rm -rf hex
    clean_CSDK_lib
    echo ">> Clean complete"
    exit 0
fi

# variable
FIRMWARE_NAME=firmware_${CFG_RELEASE}
MAP_FILE_PATH=./build/${FIRMWARE_NAME}.map
MEMD_DEF_PATH=../../lib/GPRS_C_SDK/platform/csdk/memd.def
start_time=`date +%s`
###########################################################


#check gprs sdk files
function check_CSDK()
{
    echo ">> Check GPRS CSDK lib files"
    cd ../../ # root of micropython project
    if [ "`ls -A lib/GPRS_C_SDK`xx" == "xx" ]; then
        echo "-- No CSDK, now pull"
        git submodule update --init lib/GPRS_C_SDK
        cd lib/GPRS_C_SDK
        git submodule update --init
        cd ../../
        echo "-- Pull CSDK complete"
    else
        echo "-- GPRS CSDK exist"
    fi
    if [ "`ls -A lib/GPRS_C_SDK/platform/csdk`xx" == "xx" ]; then
        echo "-- No CSDK lib, now pull"
        cd lib/GPRS_C_SDK
        proxy git submodule update --init
        cd ../../
        echo "-- Pull CSDK lib complete"
    else
        echo "-- GPRS CSDK submodule exist"
    fi
    cd ${curr_path_abs}
    echo ">> Check GPRS CSDK lib files end"
}

function generate_CSDK_lib()
{
    echo ">> Generate CSDK lib now"
    cd ../../  #root path of micropython project
    cd lib/GPRS_C_SDK/platform/tools/genlib
    ./genlib.sh ${CFG_RELEASE}
    cd ../../../../../
    if [[ -f "lib/GPRS_C_SDK/hex/libcsdk/libcsdk_${CFG_RELEASE}.a" ]]; then
        echo ">> Geneate CSDK lib compelte"
    else
        echo ">> Generate CSDK lib fail, please check error"
        exit 1
    fi
    cd ${curr_path_abs}
}

function show_time()
{
    end_time=`date +%s`
    time_distance=`expr ${end_time} - ${start_time}`
    date_time_now=$(date +%F\ \ %H:%M:%S)
    echo "=================================="
    echo "       Build Time: ${time_distance}s "
    echo "       ${date_time_now} "
    echo "=================================="
}

function show_mem_info()
{
    ram_total=$(grep  -n  "USER_RAM_SIZE" $MEMD_DEF_PATH | awk  '{print $3}')
    rom_total=$(grep  -n  "USER_ROM_SIZE" $MEMD_DEF_PATH | awk  '{print $3}')

    rom_start=$(grep  -n  "__rom_start = ." $MAP_FILE_PATH | awk  '{print $2}')
    rom_rw_start=$(grep  -n  "__user_rw_lma = ." $MAP_FILE_PATH | awk  '{print $2}')

    ram_start=$(grep  -n  "__user_rw_start = ." $MAP_FILE_PATH | awk  '{print $2}')
    ram_rw_data_end=$(grep  -n  "__user_rw_end = ." $MAP_FILE_PATH | awk  '{print $2}')
    ram_end=$(grep  -n  "__user_bss_end = ." $MAP_FILE_PATH | awk  '{print $2}')

    # echo $ram_start $ram_end
    ram_used=$(($ram_end-$ram_start))
    ram_used_percent=$(awk 'BEGIN{printf "%.2f%%\n",('$ram_used'/'$((${ram_total}))')*100}')

    rw_data_size=$(($ram_rw_data_end-$ram_start))
    rom_used=$(($rom_rw_start-$rom_start+$rw_data_size))
    rom_used_percent=$(awk 'BEGIN{printf "%.2f%%\n",('$rom_used'/'$(($rom_total))')*100}')

    echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
    echo ROM total: ${rom_total}\($((${rom_total}))\) Bytes, used: $rom_used Bytes \($rom_used_percent\)
    echo RAM total: ${ram_total}\($((${ram_total}))\) Bytes, used: $ram_used Bytes \($ram_used_percent\)
    echo "__________________________________________________________________"
}

function build_micropy()
{
    echo ">> Build micropython"
    echo "-- Remove old firmware"
    rm -f hex/*
    MAKE_J_NUMBER=`cat /proc/cpuinfo | grep vendor_id | wc -l`
    echo "-- Core number:$MAKE_J_NUMBER"
    make -j${MAKE_J_NUMBER} CFG_RELEASE=${CFG_RELEASE}
    if [[ -f "hex/${FIRMWARE_NAME}_full.lod" ]]; then
        echo "----------------------------------"
        echo "          BUILD SUCCESS           "
        echo "----------------------------------"
        show_time
        show_mem_info
    else
        echo "----------------------------------"
        echo "         !!BUILD FAIL!!           "
        echo "----------------------------------"
        show_time
        exit 1
    fi
}

########################## 3 ##############################
# Check GPRS_C_SDK files
check_CSDK
# Generate a lib from GPRS_C_SDK
generate_CSDK_lib
# Build MicroPython application
build_micropy
###########################################################
