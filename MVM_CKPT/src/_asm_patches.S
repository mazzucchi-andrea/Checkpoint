.global the_patch_assembly;
.global ckpt_assembly;

.section .text;

// ASM_PREAMBLE
the_patch_assembly:
	lea -0x1000(%rsp),%rsp //this is for the stack red zone
	pushf
        pushq %rdi
        pushq %rsi
        pushq %rdx
        pushq %rcx
        pushq %rax
        pushq %r8
        pushq %r9
        pushq %r10
        pushq %r11
        pushq %rbx
        pushq %rbp
        pushq %r12
        pushq %r13
        pushq %r14
        pushq %r15
	movq %rsp,%rsi
        movabs $0x00000000ff,%rdi
     	call the_patch 
       	popq %r15
        popq %r14
        popq %r13
        popq %r12
        popq %rbp
        popq %rbx
        popq %r11
        popq %r10
        popq %r9
        popq %r8
        popq %rax //index 11
        popq %rcx
        popq %rdx //index 13
        popq %rsi
        popq %rdi
	popf
	lea 0x1000(%rsp),%rsp

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
        shl $4, %rax
check_last:
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
#elif MOD == 256
	lea -0x1000(%rsp),%rsp //this is for the stack red zone
	pushf
        mov %rax,%rcx
        and $0xffffffffffc00000, %rcx
        and $0x3fffff, %rax
        test $15,%rax
        jz second_qword
        and $0x7fffe0,%rax
        shr $4, %rax
        mov %rax, %rbx
        and $15, %rbx
        shr $5, %rax
        bts %bx, (2 *ALLOCATOR_AREA_SIZE)(%rcx, %rax, 2)
        jc next_qword
        shl $4, %rax
        add %rbx, %rax
        shl $5, %rax
        vmovdqu (%rcx, %rax,), %ymm1
        vmovdqu %ymm1, ALLOCATOR_AREA_SIZE(%rcx, %rax,)
        jmp check_last
next_qword:
        shl $4, %rax
        add %rbx, %rax
        shl $5, %rax
check_last:
        add $32, %rax
        cmp $ALLOCATOR_AREA_SIZE, %rax
        jge end_checkpoint
second_qword:
        shr $4, %rax
        mov %rax, %rbx
        and $15, %rbx
        shr $5, %rax
        bts %bx, (2 * ALLOCATOR_AREA_SIZE)(%rcx, %rax, 2)
        jc end_checkpoint
        shl $4, %rax
        add %rbx, %rax
        shl $5, %rax
        vmovdqu (%rcx, %rax,), %ymm1
        vmovdqu %ymm1, ALLOCATOR_AREA_SIZE(%rcx, %rax,)
end_checkpoint:
        mov %gs:0, %rax
        mov %gs:8, %rbx
        mov %gs:16, %rcx
        vmovdqu %gs:24, %ymm1
	popf
	lea 0x1000(%rsp),%rsp
#endif