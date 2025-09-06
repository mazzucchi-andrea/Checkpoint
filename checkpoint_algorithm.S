; A is the main area, S in the area when the snapshots are saved and M is the area tha contains the bitmap
; sizeA = 2^21 byte, sizeS = 2^21 byte, sizeM = 0x40000 byte
; memory alignment: 8 * 1024 * 2^21
; addressA is in rax
; rax, rbx, rcx will be saved in TLS at displacements 0, 8 and 16

    mov %rax, %gs:0                 ; save rax in TLS
    mov %rbx, %gs:8                 ; save rbx in TLS
    mov %rcx, %gs:16                ; save rcx in TLS   
    mov %rax, %rcx                  ; rcx = rax = addressA
    and $0xffffffffffe00000, %rcx   ; rcx = BaseA, A base address        
    and $0x1fffff, %rax             ; rax = offset
    test $7, %rax                   ; I need to check if the quadword is not aligned
    jz second_qword                 ; if aligned jump to manage a single quadword. 
                                    ; if not aligned manage two quadword aligned that contains the quadword at addressA
    and $0x3ffff8, %rax             ; the last three bits of rax are now 0, rax is now aligned
    shr $3, %rax                    ; rax = offset/8
    mov %rax, %rbx                  ; rbx = rax
    and $15, %rbx                   ; rbx = (offset/8) mod 16, the bit to set in M
    shr $4, %rax                    ; rax = offset/128
    bts %bx, 0x400000(%rcx, %rax, 2); test and set the bit in the word at baseA + 8192 + rax * 2, (baseA +8192 = baseM)
    jc next_qword                   ; the quadword is already saved jump to the second quadword
    shl $4, %rax                    ; rax = (offset/128) * 16
    add %rbx, %rax                  ; rax = (offset/128) * 16 + (offset/8) mod 16
    mov (%rcx, %rax, 8), %rbx       ; rbx = addressA aligned
    mov %rbx,0x200000(%rcx, %rax, 8); save in S (baseA + 0x200000 = baseS)
    jmp check_last
next_qword:
    shl $4, %rax
    add %%rbx, %rax
check_last:
    shl $3, %rax                    ; rax = offset aligned
    add $8, %rax                    ; rax += 8
    cmp $0x200000, %rax             ; is the quadword outside A?
    jge end_checkpoint
second_qword:
    shr $3, %rax
    mov %rax, %rbx
    and $15, %rbx
    shr $4, %rax
    bts %bx, 0x400000(%rcx, %rax, 2)
    jc end_checkpoint
    shl $4, %rax
    add %rbx, %rax
    mov (%rcx, %rax, 8), %rbx
    mov %rbx,0x200000(%rcx, %rax, 8)
end_checkpoint:
    mov %gs:0, %rax                 ; restore rax
    mov %gs:8, %rbx                 ; restore rbx
    mov %gs:16, %rcx                ; restore rcx