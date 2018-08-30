mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))  #获取当前正在执行的makefile的绝对路径
PATH_CURR := $(strip $(patsubst %/, %, $(dir ${mkfile_path})))
PATH_MICROPY := $(strip $(patsubst %/ports/, %, $(dir ${PATH_CURR})))
CSDK_PATH := ${PATH_MICROPY}/lib/GPRS_C_SDK
include ../../py/mkenv.mk

include ${CSDK_PATH}/platform/csdk/memd.def
include ${CSDK_PATH}/platform/chip/defs/8955_base.def
include ${CSDK_PATH}/init/target.def

RELEASE = 0

ifeq ($(strip $(RELEASE)),1)
	CFG_RELEASE = release
else
	CFG_RELEASE = debug
endif

$(info Compile Version:$(CFG_RELEASE))

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
FIRMWARE_NAME := firmware_${CFG_RELEASE}
BIN := $(BUILD_PATH_ABS)/${FIRMWARE_NAME}.elf
HEX := $(BUILD_PATH_ABS)/${FIRMWARE_NAME}.srec
BIN_PATH := ${PATH_CURR}/hex
LODBASE   := $(BIN_PATH)/${FIRMWARE_NAME}_
LOD_FILE := ${LODBASE}flash.lod
WITH_PLT_LOD_FILE := ${BIN_PATH}/${FIRMWARE_NAME}_full.lod
WITH_PLT_OTA_FILE := ${BIN_PATH}/${FIRMWARE_NAME}_ota.lod
STRIP_SYMBOL_FILE := ${CSDK_PATH}/platform/compilation/platform_symbols_to_strip
PLATFORM_ELF_FOLDER_PATH := ${CSDK_PATH}/platform/csdk/debug
PLATFORM_ELF_PATH := ${wildcard ${PLATFORM_ELF_FOLDER_PATH}/*.elf}
PLATFORM_ELF_NAME := ${notdir ${PLATFORM_ELF_PATH}}
PLATFORM_LOD_PATH := ${CSDK_PATH}/platform/csdk/${strip ${CFG_RELEASE}}
PLATFORM_LOD_FILE := ${wildcard ${PLATFORM_LOD_PATH}/*.lod}
ELFCOMBINE_TOOL := ${CSDK_PATH}/platform/compilation/elfCombine.pl
TARGET_FILE := ${BUILD_PATH_ABS}/targetgen
MAP_CFG_FILE := ${CSDK_PATH}/platform/compilation/8955_map_cfg
LODCOMBINE_TOOL := ${CSDK_PATH}/platform/compilation/lodCombine.pl


LIBGCC_PATH := ${CSDK_PATH}/platform/lib/gcc/mips-elf/4.4.2
MAP := ${BUILD_PATH_ABS}/${FIRMWARE_NAME}.map
FULL_LIBRARY_PATH := -L${CSDK_PATH}/hex/libcsdk
MYLIBFILES := 
FULL_LIBRARY_EXT := -lcsdk_debug 


LDPPFLAGS += \
        -DUSER_ROM_BASE=${FLASH_BASE}+${USER_ROM_BASE} \
        -DUSER_ROM_SIZE=${USER_ROM_SIZE} \
        -DUSER_RAM_BASE=${EXT_RAM_BASE}+${USER_RAM_BASE} \
        -DUSER_RAM_SIZE=${USER_RAM_SIZE} \
        -DDUSE_BINUTILS_2_19=1
LD_SRC := ${CSDK_PATH}/platform/compilation/cust.ld
LD_FILE := ${BUILD_PATH_ABS}/cust.ld
OFORMAT := --oformat=elf32-littlemips
SREC	:= --input-target=elf32-littlemips --output-target=srec
LDFLAG_USED_ELF_FILES := -just-symbols ${CSDK_PATH}/platform/chip/rom/8955/lib/mem_bridge_rom_CHIP.elf
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
INC += -I${CSDK_PATH}/include \
       -I${CSDK_PATH}/include/std_inc \
	   -I${CSDK_PATH}/include/api_inc \
	   -I${CSDK_PATH}/libs/utils/include

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
	main.c \
	$(BUILD)/_frozen_mpy.c

LIB_SRC_C = $(addprefix lib/,\
	utils/stdout_helpers.c \
	utils/pyexec.c \
	mp-readline/readline.c \
	)

all: lod


lod: ${HEX}
	$(Q)$(MAKE) targetgen
	$(Q)srecmap -c ${MAP_CFG_FILE} -m ${FLSH_MODEL} -b ${TARGET_FILE} ${HEX} ${LODBASE} ${STDOUT_NULL}
	$(Q)$(LODCOMBINE_TOOL) openat -l  $(PLATFORM_LOD_FILE) -i $(LOD_FILE) -o $(WITH_PLT_LOD_FILE) -u $(WITH_PLT_OTA_FILE);
	$(ECHO) "BUILD OK"

OBJ = $(PY_CORE_O) $(addprefix $(BUILD_PATH)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD_PATH)/, $(LIB_SRC_C:.c=.o))

$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@


${BIN}: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)test -f ${LD_FILE} && chmod +w ${LD_FILE} || echo ""
	$(Q)${LDPP} ${LDPPFLAGS} ${LD_SRC} > ${LD_FILE}
	$(Q)${LD} -nostdlib -o ${BIN} ${OFORMAT} $^ ${FULL_SRC_OBJECTS} ${LD_OPTIONAL_OBJECTS} \
	${LDFLAG_USED_ELF_FILES}	\
	--script ${LD_FILE} \
	$(LDFLAGS) \
	-Map ${MAP}

bin: ${BIN}
	$(Q)${MKDIR} -p ${BIN_PATH}
	$(Q)${CP} -f ${BIN} ${BIN_PATH}
	$(Q)${CP} -f ${PLATFORM_ELF_PATH} ${BIN_PATH}
	$(Q)${OBJCOPY} --strip-symbols=${STRIP_SYMBOL_FILE} ${BIN_PATH}/${FIRMWARE_NAME}.elf ${BIN_PATH}/${FIRMWARE_NAME}.elf
	$(Q)${ELFCOMBINE_TOOL} -e1 ${BIN_PATH}/${FIRMWARE_NAME}.elf -e2 ${BIN_PATH}/${PLATFORM_ELF_NAME} -o ${BIN_PATH}/${FIRMWARE_NAME}_final.elf
	$(Q)${RM} -f ${BIN_PATH}/${FIRMWARE_NAME}.elf
	$(Q)${RM} -f ${BIN_PATH}/${PLATFORM_ELF_NAME}

${HEX}:bin
	$(Q)${MAKE} ${BIN}
	$(Q)$(OBJCOPY) ${SREC} ${BIN} ${HEX}

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
