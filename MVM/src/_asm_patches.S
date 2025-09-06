.global the_patch_assembly;

.section .text;

the_patch_assembly:
	lea -0x1000(%rsp),%rsp //this is for the stack red zone
	pushf
        mov %rax,%rcx
        and $0xffffffffffe00000, %rcx
        and $0x1fffff, %rax
        test $7,%rax
        jz second_qword
        and $0x3ffff8,%rax
        shr $3, %rax
        mov %rax, %rbx
        and $15, %rbx
        shr $4, %rax
        bts %bx, 0x400000(%rcx, %rax, 2)
        jc next_qword
        shl $4, %rax
        add %rbx, %rax
        mov (%rcx, %rax, 8), %rbx
        mov %rbx, 0x200000(%rcx, %rax, 8)
        jmp check_last
next_qword:
        shl $4, %rax
        add %rbx, %rax
check_last:
        shl $3, %rax
        add $8, %rax
        cmp $0x200000, %rax
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
        mov %rbx, 0x200000(%rcx, %rax, 8)
end_checkpoint:
        mov %gs:0, %rax
        mov %gs:8, %rbx
        mov %gs:16, %rcx
	popf
	lea 0x1000(%rsp),%rsp
