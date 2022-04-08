## @file
# This file contains the script to build UniversalPayload
#
# Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import argparse
import subprocess
import os
import shutil
import sys
from   ctypes import *

sys.dont_write_bytecode = True

class UPLD_INFO_HEADER(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Identifier',           ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint32),
        ('SpecRevision',         c_uint16),
        ('Reserved',             c_uint16),
        ('Revision',             c_uint32),
        ('Attribute',            c_uint32),
        ('Capability',           c_uint32),
        ('ProducerId',           ARRAY(c_char, 16)),
        ('ImageId',              ARRAY(c_char, 16)),
        ]

    def __init__(self):
        self.Identifier     =  b'UPLD'
        self.HeaderLength   = sizeof(UPLD_INFO_HEADER)
        self.HeaderRevision = 0x0075
        self.Revision       = 0x0000010105
        self.ImageId        = b'LINUX'
        self.ProducerId     = b'INTEL'

class Linux_HEADER(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Reserved1',            ARRAY(c_char, 0x6)),   # 0x0
        ('orig_video_mode',      c_uint8),              # 0x6
        ('orig_video_cols',      c_uint8),              # 0x7
        ('Reserved2',            ARRAY(c_char, 0x6)),   # 0x8
        ('orig_video_lines',     c_uint8),              # 0xe
        ('orig_video_isVGA',     c_uint8),              # 0xf
        ('orig_video_points',    c_uint16),             # 0x10
        ('Reserved3',            ARRAY(c_char, 0x1Df)), # 0x12
        ('setup_sects',          c_uint8),              # 0x1f1
        ('root_flags',           c_uint16),             # 0x1f2
        ('Reserved4',            ARRAY(c_char, 0x8)),   # 0x1f4
        ('root_dev',             c_uint16),             # 0x1fc
        ('Reserved5',            ARRAY(c_char, 0x12)),  # 0x1fe
        ('loader_type',          c_uint8),              # 0x210
        ('Reserved6',            ARRAY(c_char, 0x1f)),  # 0x211
        ('kernel_alignment',     c_uint32),             # 0x230
        ('relocatable_kernel',   c_uint8),              # 0x234
        ('Reserved7',            ARRAY(c_char, 0x2B)),  # 0x235
        ('init_size',            c_uint32),             # 0x260
        ('Reserved8',            ARRAY(c_char, 0xD9C)), # 0x264 - 0x1000
        ]

def RunCommand(cmd):
    print(cmd)
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,cwd=os.environ['WORKSPACE'])
    while True:
        line = p.stdout.readline()
        if not line:
            break
        print(line.strip().decode(errors='ignore'))

    p.communicate()
    if p.returncode != 0:
        print("- Failed - error happened when run command: %s"%cmd)
        raise Exception("ERROR: when run command: %s"%cmd)

def BuildUniversalPayload(Args, MacroList):
    BuildTarget = Args.Target
    LinuxKernal = Args.KernelPath
    Initramfs = Args.InitramfsPath
    ElfToolChain = Args.ToolChain

    if ElfToolChain == 'CLANGDWARF':
        if "CLANG_BIN" in os.environ:
            ObjcopyPath = os.path.join(os.environ["CLANG_BIN"], "llvm-objcopy")
        else:
            ObjcopyPath = "llvm-objcopy"
    elif  ElfToolChain == "GCC5":
        ObjcopyPath = "objcopy"
    else:
        print("- Failed - only support CLANGDWARF or GCC5 as tool chain")
        sys.exit(1)
    try:
        RunCommand(f'"{ObjcopyPath}" --version')
    except:
        print("- Failed - Please check if LLVM or objcopy is installed or if CLANG_BIN is set correctly")
        sys.exit(1)

    # LinuxKernal is mandantory
    # Initramfs is optional because it may be contained in LinuxKernal
    if LinuxKernal == "" or not os.path.exists(LinuxKernal):
        print("- Failed - can not find linux kernel")
        sys.exit(1)
    if Initramfs != "" and not os.path.exists(Initramfs):
        print("- Failed - can not find Initramfs")
        sys.exit(1)

    EntryModuleInf = os.path.normpath("UefiPayloadPkg/UefiPayloadEntry/LinuxUniversalPayloadEntry.inf")
    DscPath = os.path.normpath("UefiPayloadPkg/LinuxUefiPayloadPkg.dsc")
    BuildDir = os.path.join(os.environ['WORKSPACE'], os.path.normpath("Build/LinuxUefiPayloadPkg"))
    ModuleReportPath = os.path.join(BuildDir, "UefiUniversalPayloadEntry.txt")
    UpldInfoFile = os.path.join(BuildDir, "UniversalPayloadInfo.bin")
    LinuxBootParams = os.path.join(BuildDir, "LinuxBootParams.bin")
    VmLinux = os.path.join(BuildDir, "VmLinux.bin")

    EntryOutputDir = os.path.join(BuildDir, f"{BuildTarget}_{ElfToolChain}", os.path.normpath("IA32/UefiPayloadPkg/UefiPayloadEntry/LinuxUniversalPayloadEntry/DEBUG/UniversalPayloadEntry.dll"))

    Defines = ""
    for key in MacroList:
        Defines +=" -D {0}={1}".format(key, MacroList[key])

    #
    # Building Linux Universal Payload entry.
    #
    BuildModule = f"build -p {DscPath} -b {BuildTarget} -a IA32 -m {EntryModuleInf} -t {ElfToolChain} -y {ModuleReportPath}"
    BuildModule += Defines
    RunCommand(BuildModule)

    #
    # Buid Universal Payload Information Section ".upld_info"
    #
    upld_info_hdr = UPLD_INFO_HEADER()
    upld_info_hdr.ImageId = Args.ImageId.encode()[:16]
    fp = open(UpldInfoFile, 'wb')
    fp.write(bytearray(upld_info_hdr))
    fp.close()

    #
    # Buid Universal Payload Information Section ".upld_info"
    #
    boot_params = Linux_HEADER()
    with open(LinuxKernal, "rb") as fd:
        LinuxKernalBuffer = fd.read()
        LinuxHeader = Linux_HEADER.from_buffer_copy(LinuxKernalBuffer)
        boot_params.root_flags = LinuxHeader.root_flags
        boot_params.root_dev = LinuxHeader.root_dev
        boot_params.init_size = LinuxHeader.init_size
        boot_params.relocatable_kernel = LinuxHeader.relocatable_kernel
        boot_params.kernel_alignment = LinuxHeader.kernel_alignment
        boot_params.orig_video_mode   = 3
        boot_params.orig_video_cols   = 80
        boot_params.orig_video_lines  = 25
        boot_params.orig_video_isVGA  = 1
        boot_params.orig_video_points = 16
        boot_params.loader_type = 0xff
        setup_sects = LinuxHeader.setup_sects
        #
        # The 32-bit (non-real-mode) kernel starts at offset (setup_sects+1)*512
        # in the kernel file (if setup_sects == 0 the real value is 4.)
        #
        setup_size = 4 * 512
        if setup_sects != 0:
            setup_size = (setup_sects + 1) * 512
        LinuxKernalBuffer = LinuxKernalBuffer[setup_size:]

    if boot_params.relocatable_kernel == 0:
        print("- Failed - the linux kernel must be able to relocate")
        sys.exit(1)

    fp = open(LinuxBootParams, 'wb')
    fp.write(bytearray(boot_params))
    fp.close()

    fp = open(VmLinux, 'wb')
    fp.write(bytearray(LinuxKernalBuffer))
    fp.close()
    
    #
    # Copy the UniversalPayloadInfo.bin, Boot_params.bin, vmlinux.bin, Intiramfs.bin as a sections in elf format Universal Payload entry.
    #
    remove_section = f'"{ObjcopyPath}" -I elf32-i386 -O elf32-i386 --remove-section .upld_info --remove-section .upld.linux --remove-section .upld.uefi.fv --remove-section .upld.initramfs --remove-section .upld.bootparams {EntryOutputDir}'
    add_section    = f'"{ObjcopyPath}" -I elf32-i386 -O elf32-i386 --add-section .upld_info={UpldInfoFile} --add-section .upld.linux={VmLinux} --add-section .upld.bootparams={LinuxBootParams} {EntryOutputDir}'
    set_section    = f'"{ObjcopyPath}" -I elf32-i386 -O elf32-i386 --set-section-alignment .upld.upld_info=16 --set-section-alignment .upld.linux=16 {EntryOutputDir}'
    if Initramfs != "":
        add_section    += f' --add-section .upld.initramfs={Initramfs}'
        set_section    += ' --set-section-alignment .upld.initramfs=16'

    RunCommand(remove_section)
    RunCommand(add_section)
    RunCommand(set_section)

    shutil.copy (EntryOutputDir, os.path.join(BuildDir, 'UniversalPayload.elf'))

def main():
    parser = argparse.ArgumentParser(description='For building Linux Universal Payload')
    parser.add_argument('-b', '--Target', default='DEBUG')
    parser.add_argument('-t', '--ToolChain')
    parser.add_argument("-D", "--Macro", action="append")
    parser.add_argument('-i', '--ImageId', type=str, help='Specify payload ID (16 bytes maximal).', default ='UEFI')
    parser.add_argument('--KernelPath', type=str, default='')
    parser.add_argument('--InitramfsPath', type=str, default='')

    MacroList = {}
    args = parser.parse_args()
    if args.Macro is not None:
        for Argument in args.Macro:
            if Argument.count('=') != 1:
                print("Unknown variable passed in: %s"%Argument)
                raise Exception("ERROR: Unknown variable passed in: %s"%Argument)
            tokens = Argument.strip().split('=')
            MacroList[tokens[0].upper()] = tokens[1]
    BuildUniversalPayload(args, MacroList)
    print ("Successfully build Universal Payload")

if __name__ == '__main__':
    main()
