; starting from the mov that save in A the qword I need to calculate the offset.
; A is the main area, S in the area when the snapshots are saved and M is the area tha contains the bitmap
; sizeA = 4096 byte, sizeS = 4096 byte, sizeM = 64 byte
; addressA is in rax
; rax, rbx, rcx will be saved in TLS at displacement 0, 8, 16

    mov %rax, %gs:0             ; save rax in TLS
    mov %rbx, %gs:8             ; save rbx in TLS
    mov %rcx, %gs:16            ; save rcx in TLS   
    mov %rax, %rcx              ; rcx = rax = addressA
    and $-4096, %rcx            ; rcx = BaseA, A base address        
    and $0xFFF, %rax            ; rax = offset
    test $7, %rax               ; I need to check if the quadword is not aligned
    jz second_qword             ; if aligned jump to manage a single quadword. 
                                ; if not aligned manage two quadword aligned that contains the quadword at addressA
    and $-8, %rax               ; the last three bits of rax are now 0, rax is now aligned
    shr $3, %rax                ; rax = offset/8
    mov %rax, %rbx              ; rbx = rax
    and $15, %rbx               ; rbx = (offset/8) mod 16, the bit to set in M
    shr $4, %rax                ; rax = offset/128
    bts %bx, 8192(%rcx, %rax, 2); test and set the bit in the word at baseA + 8192 + rax * 2, (baseA +8192 = baseM)
    jc next_qword               ; the quadword is already saved
    shl $4, %rax                ; rax = (offset/128) * 16
    add %rbx, %rax              ; rax = (offset/128) * 16 + (offset/8) mod 16
    mov (%rcx, %rax, 8), %rbx   ; rbx = addressA aligned
    mov %rbx, 4096(%rcx, %rax, 8); save in S (baseA + 4096 = baseS)
next_qword:
    mov %%gs:0, %%rax           ; rax = addressA
    and $0xFF8, %%rax           ; rax = offset aligned
    add $8, %rax                ; rax += 8
    cmp %rax, $4096             ; is the quadword outside A?
    jge end
second_qword:
    shr $3, %rax
    mov %rax, %rbx
    and $15, %rbx
    shr $4, %rax
    bts %bx, 8192(%rcx, %rax, 2)
    jc end
    shl $4, %rax
    add %rbx, %rax
    mov (%rcx, %rax, 8), %rbx
    mov %rbx, 4096(%rcx, %rax, 8)
end:
    mov %gs:0, %rax            ; restore rax
    mov %gs:8, %rbx            ; restore rbx
    mov %gs:16, %rcx           ; restore rcx