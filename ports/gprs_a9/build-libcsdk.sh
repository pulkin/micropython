#!/bin/bash
set -e

######################## 1 ################################
# check path
curr_path_abs=$(cd `dirname $0`; pwd)
curr_folder_name="${curr_path_abs##*/}"
echo $curr_path_abs
if [[ "${curr_folder_name}" != "gprs_a9" ]]; then
    echo "Plese exec build.sh in gprs_a9 folder"
    exit 1
fi

######################### 2 ###############################
# check parameters
CFG_RELEASE=debug
if [[ "$1xx" == "releasexx" ]]; then
    CFG_RELEASE=release
fi

###########################################################

function apply_patches()
{
    echo ">> patching csdk lod"
    ./libcsdk-patches/patch-lod.py ../../lib/GPRS_C_SDK/platform/csdk/debug/SW_V2131_csdk.lod
}

function generate_CSDK_lib()
{
    echo ">> Generate CSDK lib now"
    cd ../../  #root path of micropython project
    cd lib/GPRS_C_SDK/platform/tools/genlib
    chmod +x genlib.sh
    PATH=$PATH:$curr_path_abs/../../lib/csdtk42-linux/bin
    export LD_LIBRARY_PATH=$curr_path_abs/../../lib/csdtk42-linux/lib
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

apply_patches
generate_CSDK_lib

