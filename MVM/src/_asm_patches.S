.global the_patch_assembly;

.section .text;

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
        mov %rdx, %gs:0
        mov %rbx, %gs:8
        mov %rcx, %gs:16
        mov %rdx,%rcx
        and $0xfffffffffffff000,%rcx
        and $0xfff,%rdx
        test $0x7,%rdx
        jz second_qword
        and $0x000000000000fff8,%rdx
        shr $0x3, %rdx
        mov %rdx, %rbx
        and $0xf, %bx
        shr $0x4, %rdx
        bts %bx, 8192(%rcx, %rdx, 2)
        jc next_qword
        shl $4, %rdx
        add %rbx, %rdx
        mov (%rcx, %rdx, 8), %rbx
        mov %rbx, 4096(%rcx, %rdx, 8)
next_qword:
        mov %gs:0, %rdx
        and $0xFF8, %rdx
        add $8, %rdx
        cmp $4096, %rdx
        jge end_checkpoint
second_qword:
        shr $3, %rdx
        mov %rdx, %rbx
        and $15, %rbx
        shr $4, %rdx
        bts %bx, 8192(%rcx, %rdx, 2)
        jc end_checkpoint
        shl $4, %rdx
        add %rbx, %rdx
        mov (%rcx, %rdx, 8), %rbx
        mov %rbx, 4096(%rcx, %rdx, 8)
end_checkpoint:
        mov %gs:0, %rdx
        mov %gs:8, %rbx
        mov %gs:16, %rcx
	popf
	lea 0x1000(%rsp),%rsp