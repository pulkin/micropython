mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))  #获取当前正在执行的makefile的绝对路径
PATH_CURR := $(strip $(patsubst %/, %, $(dir ${mkfile_path})))
include ../../py/mkenv.mk

include ${PATH_CURR}/GPRS_C_SDK/platform/csdk/memd.def
include ${PATH_CURR}/GPRS_C_SDK/platform/chip/defs/8955_base.def
include ${PATH_CURR}/GPRS_C_SDK/init/target.def


CT_RELEASE = debug


# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE = mips-elf-
#############################################
MAKE = make
LD  := $(CROSS_COMPILE)ld
LDPP := $(CROSS_COMPILE)cpp -C -P -undef
STDOUT_NULL := 1>/dev/null

BUILD_PATH := build
BUILD_PATH_ABS := ${PATH_CURR}/${BUILD_PATH}
CSDK_PATH := ${PATH_CURR}/GPRS_C_SDK
BIN := $(BUILD_PATH_ABS)/firmware.elf
HEX := $(BUILD_PATH_ABS)/firmware.srec
BIN_PATH := ${PATH_CURR}/hex
LODBASE   := $(BIN_PATH)/firmware_
LOD_FILE := ${LODBASE}flash.lod
WITH_PLT_LOD_FILE := ${BIN_PATH}/firmware_full.lod
WITH_PLT_OTA_FILE := ${BIN_PATH}/firmware_ota.lod
STRIP_SYMBOL_FILE := ${PATH_CURR}/GPRS_C_SDK/platform/compilation/platform_symbols_to_strip
PLATFORM_ELF_FOLDER_PATH := ${PATH_CURR}/GPRS_C_SDK/platform/csdk/debug
PLATFORM_ELF_PATH := ${wildcard ${PLATFORM_ELF_FOLDER_PATH}/*.elf}
PLATFORM_ELF_NAME := ${notdir ${PLATFORM_ELF_PATH}}
PLATFORM_LOD_PATH := ${CSDK_PATH}/platform/csdk/${strip ${CT_RELEASE}}
PLATFORM_LOD_FILE := ${wildcard ${PLATFORM_LOD_PATH}/*.lod}
ELFCOMBINE_TOOL := ${PATH_CURR}/GPRS_C_SDK/platform/compilation/elfCombine.pl
TARGET_FILE := ${BUILD_PATH_ABS}/targetgen
MAP_CFG_FILE := ${CSDK_PATH}/platform/compilation/8955_map_cfg
LODCOMBINE_TOOL := ${CSDK_PATH}/platform/compilation/lodCombine.pl


LIBGCC_PATH := ${PATH_CURR}/GPRS_C_SDK/platform/lib/gcc/mips-elf/4.4.2
MAP := ${BUILD_PATH_ABS}/firmware.map
FULL_LIBRARY_PATH := -L${PATH_CURR}/GPRS_C_SDK/build/csdk/lib
MYLIBFILES := 
FULL_LIBRARY_EXT := -lcsdk_debug 


LDPPFLAGS += \
        -DUSER_ROM_BASE=${FLASH_BASE}+${USER_ROM_BASE} \
        -DUSER_ROM_SIZE=${USER_ROM_SIZE} \
        -DUSER_RAM_BASE=${EXT_RAM_BASE}+${USER_RAM_BASE} \
        -DUSER_RAM_SIZE=${USER_RAM_SIZE} \
        -DDUSE_BINUTILS_2_19=1
LD_SRC := ${PATH_CURR}/GPRS_C_SDK/platform/compilation/cust.ld
LD_FILE := ${BUILD_PATH_ABS}/cust.ld
OFORMAT := --oformat=elf32-littlemips
SREC	:= --input-target=elf32-littlemips --output-target=srec
LDFLAG_USED_ELF_FILES := -just-symbols ${PATH_CURR}/GPRS_C_SDK/platform/chip/rom/8955/lib/mem_bridge_rom_CHIP.elf
LDFLAGS += --gc-sections
LDFLAGS += ${FULL_LIBRARY_PATH} --start-group ${MYLIBFILES} ${FULL_LIBRARY_EXT} --end-group
LDFLAGS	+= -L${LIBGCC_PATH} -lgcc 
LDFLAGS += --warn-common  --error-unresolved-symbols 
FULL_SRC_OBJECTS := 
LD_OPTIONAL_OBJECTS := 
########################################3###
INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD_PATH_ABS)
INC += -I${PATH_CURR}/GPRS_C_SDK/include \
       -I${PATH_CURR}/GPRS_C_SDK/include/std_inc \
	   -I${PATH_CURR}/GPRS_C_SDK/include/api_inc 

# DFU = $(TOP)/tools/dfu.py
# PYDFU = $(TOP)/tools/pydfu.py
# CFLAGS_CORTEX_M4 = -mthumb -mtune=cortex-m4 -mabi=aapcs-linux -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant -Wdouble-promotion
# CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib $(CFLAGS_CORTEX_M4) $(COPT)
CFLAGS = $(INC) -Wshadow \
		  -std=gnu99 \
          -Os -g -fno-inline-small-functions -fno-inline-functions -fno-align-functions -fno-align-jumps -fno-align-loops -fno-align-labels \
          -msoft-float \
          -Wunused-function \
          -Wuninitialized \
          -minterlink-mips16 -fno-strict-aliasing \
          -ffunction-sections -fdata-sections \
          -fno-builtin-iswspace \
          -ffixed-t3 -ffixed-t4 -ffixed-t5 -ffixed-t6 -ffixed-t7 -ffixed-s2 -ffixed-s3 -ffixed-s4 -ffixed-s5 -ffixed-s6 -ffixed-s7 -ffixed-fp \
          -G0 -Wall -march=xcpu -mtune=xcpu -Wa,-march=xcpu,-mtune=xcpu -EL -mexplicit-relocs -fweb -frename-registers -mmemcpy -mmips-tfile -nostartfiles -nostdlib -nostdinc -nodefaultlibs -c -pipe \
          -fwide-exec-charset=UTF-16LE -fshort-wchar \
		  -mips16 

LIBS =

SRC_C = \
	test.c 




all: lod


lod: ${HEX}
	$(MAKE) targetgen
	srecmap -c ${MAP_CFG_FILE} -m ${FLSH_MODEL} -b ${TARGET_FILE} ${HEX} ${LODBASE} ${STDOUT_NULL}
	$(LODCOMBINE_TOOL) openat -l  $(PLATFORM_LOD_FILE) -i $(LOD_FILE) -o $(WITH_PLT_LOD_FILE) -u $(WITH_PLT_OTA_FILE);

OBJ = $(PY_CORE_O) $(addprefix $(BUILD_PATH)/, $(SRC_C:.c=.o))

${BIN}: $(OBJ)
	$(ECHO) "LINK $@"
	test -f ${LD_FILE} && chmod +w ${LD_FILE} || echo ""
	${LDPP} ${LDPPFLAGS} ${LD_SRC} > ${LD_FILE}
	${LD} -nostdlib -o ${BIN} ${OFORMAT} $^ ${FULL_SRC_OBJECTS} ${LD_OPTIONAL_OBJECTS} \
	${LDFLAG_USED_ELF_FILES}	\
	--script ${LD_FILE} \
	$(LDFLAGS) \
	-Map ${MAP}

bin: ${BIN}
	${MKDIR} -p ${BIN_PATH}
	${CP} -f ${BIN} ${BIN_PATH}
	${CP} -f ${PLATFORM_ELF_PATH} ${BIN_PATH}
	${OBJCOPY} --strip-symbols=${STRIP_SYMBOL_FILE} ${BIN_PATH}/firmware.elf ${BIN_PATH}/firmware.elf
	${ELFCOMBINE_TOOL} -e1 ${BIN_PATH}/firmware.elf -e2 ${BIN_PATH}/${PLATFORM_ELF_NAME} -o ${BIN_PATH}/firmware_final.elf
	${RM} -f ${BIN_PATH}/firmware.elf
	${RM} -f ${BIN_PATH}/${PLATFORM_ELF_NAME}

${HEX}:bin
	${MAKE} ${BIN}
	$(OBJCOPY) ${SREC} ${BIN} ${HEX}

.PHONY: targetgen
TARGET_FILE := ${BUILD_PATH_ABS}/targetgen
targetgen:
	@${ECHO} "#############################################"     > $(TARGET_FILE)
	@${ECHO} "# Target component definitions"                   >> $(TARGET_FILE)
	@${ECHO} "#############################################"    >> $(TARGET_FILE)
	@${ECHO} "FLSH_MODEL:=           $(FLSH_MODEL)"             >> $(TARGET_FILE)
	@${ECHO} "#############################################"    >> $(TARGET_FILE)
	@${ECHO} "# Flash Mapping"                                  >> $(TARGET_FILE)
	@${ECHO} "#############################################"    >> $(TARGET_FILE)
	@${ECHO} "FLASH_SIZE:=           $(FLASH_SIZE)"             >> $(TARGET_FILE)
	@${ECHO} "RAM_SIZE:=             $(RAM_SIZE)"               >> $(TARGET_FILE)
	@${ECHO} "RAM_PHY_SIZE:=         $(RAM_PHY_SIZE)"           >> $(TARGET_FILE)
	@${ECHO} "CALIB_BASE:=           $(CALIB_BASE)"             >> $(TARGET_FILE)
	@${ECHO} "FACT_SETTINGS_BASE:=   $(FACT_SETTINGS_BASE)"     >> $(TARGET_FILE)
	@${ECHO} "CODE_BASE:=            $(CODE_BASE)"              >> $(TARGET_FILE)
	@${ECHO} "USER_DATA_BASE:=       $(USER_DATA_BASE)"         >> $(TARGET_FILE)
	@${ECHO} "USER_DATA_SIZE:=       $(USER_DATA_SIZE)"         >> $(TARGET_FILE)
	@${ECHO} "ROMU_FLASH_LAYOUT:=    $(ROMU_FLASH_LAYOUT)"      >> $(TARGET_FILE)
	@${ECHO} "USER_BASE:=            $(USER_ROM_BASE)"     		>> $(TARGET_FILE)
	@${ECHO} "USER_SIZE:=            $(USER_ROM_SIZE)"     		>> $(TARGET_FILE)
	@${ECHO} ""                                                 >> $(TARGET_FILE)
	@${ECHO} "#############################################"    >> $(TARGET_FILE)
	@${ECHO} "# others"                                  	    >> $(TARGET_FILE)
	@${ECHO} "#############################################"    >> $(TARGET_FILE)

include $(TOP)/py/mkrules.mk
