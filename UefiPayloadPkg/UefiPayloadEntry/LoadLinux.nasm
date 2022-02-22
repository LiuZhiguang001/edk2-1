global ASM_PFX(LoadLinux)
ASM_PFX(LoadLinux):
        mov   esi, [esp + 4]
        mov   eax, [esp + 8]
        jmp eax
        ret