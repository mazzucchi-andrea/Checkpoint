.global the_patch_assembly;
.global ckpt_assembly;

.section .text;

// ASM_PREAMBLE
the_patch_assembly:

// CKPT ASM CODE
ckpt_assembly:
#if MOD == 64
	lea -0x1000(%rsp),%rsp //this is for the stack red zone
	pushf
        mov %rax,%rcx
        and $0xffffffffffc00000, %rcx
        and $0x3fffff, %rax
        test $7,%rax
        jz second_qword
        and $0x7ffff8,%rax
        shr $3, %rax
        mov %rax, %rbx
        and $15, %rbx
        shr $4, %rax
        bts %bx, (2 *ALLOCATOR_AREA_SIZE)(%rcx, %rax, 2)
        jc next_qword
        shl $4, %rax
        add %rbx, %rax
        mov (%rcx, %rax, 8), %rbx
        mov %rbx, ALLOCATOR_AREA_SIZE(%rcx, %rax, 8)
        jmp check_last
next_qword:
        shl $4, %rax
        add %rbx, %rax
check_last:
        lea 8(,%rax, 8), %rax
        cmp $ALLOCATOR_AREA_SIZE, %rax
        jge end_checkpoint
second_qword:
        shr $3, %rax
        mov %rax, %rbx
        and $15, %rbx
        shr $4, %rax
        bts %bx, (2 * ALLOCATOR_AREA_SIZE)(%rcx, %rax, 2)
        jc end_checkpoint
        shl $4, %rax
        add %rbx, %rax
        mov (%rcx, %rax, 8), %rbx
        mov %rbx, ALLOCATOR_AREA_SIZE(%rcx, %rax, 8)
end_checkpoint:
        mov %gs:0, %rax
        mov %gs:8, %rbx
        mov %gs:16, %rcx
	popf
	lea 0x1000(%rsp),%rsp

#elif MOD == 128
	lea -0x1000(%rsp),%rsp //this is for the stack red zone
	pushf
        mov %rax,%rcx
        and $0xffffffffffc00000, %rcx
        and $0x3fffff, %rax
        test $15,%rax
        jz second_qword
        and $0x7ffff0,%rax
        shr $4, %rax
        mov %rax, %rbx
        and $15, %rbx
        shr $4, %rax
        bts %bx, (2 *ALLOCATOR_AREA_SIZE)(%rcx, %rax, 2)
        jc next_qword
        shl $4, %rax
        add %rbx, %rax
        shl $4, %rax
        movdqu (%rcx, %rax,), %xmm1
        movdqu %xmm1, ALLOCATOR_AREA_SIZE(%rcx, %rax,)
        jmp check_last
next_qword:
        shl $4, %rax
        add %rbx, %rax
check_last:
        shl $4, %rax
        add $16, %rax
        cmp $ALLOCATOR_AREA_SIZE, %rax
        jge end_checkpoint
second_qword:
        shr $4, %rax
        mov %rax, %rbx
        and $15, %rbx
        shr $4, %rax
        bts %bx, (2 * ALLOCATOR_AREA_SIZE)(%rcx, %rax, 2)
        jc end_checkpoint
        shl $4, %rax
        add %rbx, %rax
        shl $4, %rax
        movdqu (%rcx, %rax,), %xmm1
        movdqu %xmm1, ALLOCATOR_AREA_SIZE(%rcx, %rax,)
end_checkpoint:
        mov %gs:0, %rax
        mov %gs:8, %rbx
        mov %gs:16, %rcx
        movdqu %gs:24, %xmm1
	popf
	lea 0x1000(%rsp),%rsp
#endif
