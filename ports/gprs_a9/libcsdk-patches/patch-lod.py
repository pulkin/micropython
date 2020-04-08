#!/usr/bin/env python3
import sys, re

fname, = sys.argv[1:]
nop = "0065"
patch_list = {
    0x08185a14: ("Issue 28 -- user name", bytes.fromhex('c0c2c1c2c2c2c3c2'), bytes.fromhex('c0c2' + (nop * 3))),
    0x08185a46: ("Issue 28 -- password", bytes.fromhex('a0c2a1c2a2c2a3c2'), bytes.fromhex('a0c2' + (nop * 3))),
    0x08185a74: ("Issue 28 -- APN", bytes.fromhex('80c281c282c283c2'), bytes.fromhex('80c2' + (nop * 3))),
    0x0817aa3e: ("Issue 50 -- double socket.connect", bytes.fromhex('701836ea'), bytes.fromhex(nop * 2)),
}
keys_l = sorted(patch_list.keys())

address = None
patching_in_progress = None
patching_now = None
checksum = 0
data = []

with open(fname, 'r') as f:
    for n, line in enumerate(f):
        if line.startswith("#"):
            if line.startswith("#checksum"):
                data.append("#checksum={:08x}\n".format(checksum))
            else:
                data.append(line)  # Comment line
        elif line.startswith("@"):
            address = int(line[1:], 16)
            data.append(line)
        else:
            line_bytes = bytes.fromhex(line[:-1])[::-1]
            address_to = address + len(line_bytes)
            for address_p, (name, old, new) in patch_list.items():
                address_p_to = address_p + len(old)
                address_o = max(address, address_p)
                address_o_to = min(address_to, address_p_to)
                if address_o < address_o_to:
                    if patching_in_progress is None:
                        patching_in_progress = address_p
                        print("{} @ 0x{:08x}".format(name, address_p))
                    elif patching_in_progress != address_p:
                        raise RuntimeError("Failed to patch 0x{:08x}: overlapping addresses".format(patching_in_progress))
                    current_patch = line_bytes[address_o - address:address_o_to - address]
                    old_patch = old[address_o - address_p:address_o_to - address_p]
                    new_patch = new[address_o - address_p:address_o_to - address_p]
                    if old_patch != new_patch:
                        if new_patch == current_patch:
                            if patching_now in (None, False):
                                print("  .", end=" ")
                                patching_now = False
                            else:
                                raise RuntimeError("Failed to patch 0x{:08x}: data mismatch".format(patching_in_progress))
                        elif old_patch == current_patch:
                            if patching_now in (None, True):
                                print("  ✓", end=" ")
                                patching_now = True
                                line_bytes = line_bytes[:address_o - address] + new_patch + line_bytes[address_o_to - address:]
                            else:
                                raise RuntimeError("Failed to patch 0x{:08x}: data mismatch".format(patching_in_progress))
                        else:
                            raise RuntimeError("Failed to patch 0x{:08x}: data mismatch".format(patching_in_progress))
                        print("{:d}:{:d}".format(address_o - address_p, address_o_to - address_p))
                    if address_o_to == address_p_to:
                        patching_in_progress = None
                        patching_now = None
            address = address_to
            checksum += int.from_bytes(line_bytes, 'little')
            checksum = checksum & 0xFFFFFFFF
            data.append(line_bytes[::-1].hex()+"\n")
with open(fname, 'w') as f:
    f.write(''.join(data))
print("Done")

