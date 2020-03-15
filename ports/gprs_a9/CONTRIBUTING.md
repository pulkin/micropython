## Contributing

Besides reporting and fixing bugs, there is a number of issues suitable for new-commers, please consult **Issues** on github.
This file includes some useful information for maintaining this port.

### General information

TODO

### Fixing bugs in CSDK

The underlying API (CSDK) is [published](https://github.com/Ai-Thinker-Open/GPRS_C_SDK) ([fork](https://github.com/pulkin/GPRS_C_SDK)) without source code.
It wraps another level of RDA8955 chip API which can be found in [source codes of the AT firmware](https://github.com/cherryding1/RDA8955_W17.44_IDH?).

API is physically located in [another repo](https://github.com/Ai-Thinker-Open/GPRS-C-SDK-LIB/tree/master) with 2 versions of blobs: [debug](https://github.com/Ai-Thinker-Open/GPRS-C-SDK-LIB/tree/master/debug) and [release](https://github.com/Ai-Thinker-Open/GPRS-C-SDK-LIB/tree/master/release).
Each version includes 2 files: `SW_V2129_csdk.elf` and `SW_V2129_csdk.lod`.
The `elf` file is used for linking only: the actual firmware is located in `lod` file.

Following is the procedure used for fixing #28 "Supplying parameters for `cellular.gprs` with one or two characters leads to module halt":

1. Disassemble `SW_V2129_csdk.elf` using `ghidra`.
   It is important to supply the disassembler proper function signatures which can be found [here](https://github.com/Ai-Thinker-Open/GPRS_C_SDK/blob/master/include/sdk_init.h) (CSDK) and [here](https://github.com/cherryding1/RDA8955_W17.44_IDH/blob/master/soft/platform/service/include/cfw.h) (RDA8995).
2. Locate functions `Network_StartActive` and `ApiPdpContextToCFWPdpContext`.
   The issue is inside function `ApiPdpContextToCFWPdpContext`: it copies APN, username and password into a `struct` used by the RDA8955 chip API.
   However, instructions at addresses `88185a14`, `88185a16`, `88185a18`, and `88185a1a` zero-out first four bytes of the new username string which may have not be allocated if username is one- or two-char string.
   This might have been caused by the type mismatch: `int * new_user = OS_Malloc(strlen(old_user) + 1); new_user[0] = 0;` (`char` should be instead of `int`).
3. To fix it, the `sb` (store byte) instruction can be replaced by `nop` (no operation) at last 3 addresses.
   The same has to be performed at addresses `88185a48`, `88185a4a`, `88185a4c` (password) and `88185a76`, `88185a78`, `88185a7a` (APN).
   Unfortunately, `ghidra` fails at re-compiling MIPS `elf` files: radare2 can be used instead `r2 -a mips -b 32 -w SW_V2129_csdk.elf`
4. Finally, the corresponding `SW_V2129_csdk.lod` has to be re-generated.
   In this simple case `SW_V2129_csdk.lod` can be opened as a text file and searched for signatures such as "c280c281c282c283" (4 buggy operations: the byte order is reversed, newlines have to be adjusted).
   Afterwards, the checksum has to be fixed: this can be done by running micropython build and copying the checksum from the error message.

#### Disassembly

`elf` is a standard ELF file with MIPS 32-bit arch.
The `debug` version can be analyzed gracefully by [ghidra](https://ghidra-sre.org/) and, probably, most other decompilers.

*Note: micropython is currently compiled against `debug` version.*

#### Generating `lod` from `elf`

*Note: this is not a working recipe.*

It is possible to produce `lod` from `elf` file by running the following:
```bash
cd micropython/lib
csdtk42-linux/bin/mips-elf-objcopy \
    --input-target=elf32-littlemips \
    --output-target=srec \
    GPRS_C_SDK/platform/csdk/debug/SW_V2129_csdk.elf \
    GPRS_C_SDK/platform/csdk/debug/SW_V2129_csdk.srec
csdtk42-linux/bin/srecmap -c \
    GPRS_C_SDK/platform/compilation/8955_map_cfg \
    -m rom \
    -b targetgen \
    GPRS_C_SDK/platform/csdk/debug/SW_V2129_csdk.srec \
    GPRS_C_SDK/platform/csdk/debug/SW_V2129 \
    1>/dev/null
```
The file `targetgen` can be extracted by opening `SW_V2129_csdk.lod` in a text editor.
```
XCV_MODEL:=xcv_8955
PA_MODEL:=pasw_rda6625
FLSH_MODEL:=flsh_spi32m
FLASH_SIZE:=0x400000
RAM_SIZE:=0x00265000
RAM_PHY_SIZE:=0x00400000
RAM_EXTAPP_SIZE:=0
FLASH_EXTAPP_SIZE:=0
CALIB_BASE:=0x3FA000
FACT_SETTINGS_BASE:=0x3FE000
CODE_BASE:=0x00000000
USER_DATA_BASE:=0x00361000
USER_DATA_SIZE:=0x00099000
USER_BASE:=0x00240000
USER_SIZE:=0x00100000
BOOT_SECTOR:=0x00000000
PM_MODEL:=pmu_8955
FM_MODEL:=rdafm_8955
```
This produces a file `SW_V2129bcpurom.lod`.

**TODO**: the produced file is different from the original `SW_V2129_csdk.lod`: this procedure has to be refined.

