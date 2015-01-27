.text
.globl system_call

system_call:
	#push all regs
	#validate eax syscall values
	#-good? use as index into jump table-->call code for syscall
	#-ret from call-->cleanup, ret control to program
	
	pushl %eax
	
#	cld
	pushw %fs
	pushw %es
	pushw %ds
	pushl %eax
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
#	movl $(0x0018), %esi  #kernel_ds
#	movl %esi, %ds
#	movl %esi, %es
	
	cmpl $1, %eax
	jl invalid_syscall
	cmpl $10, %eax
	jg invalid_syscall

	call *syscall_jump(,%eax,4)
	movl %eax, 24(%esp)
	
ret_from_syscall:
	addl $4, %esp
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popl %eax
	popw %ds
	popw %es
	popw %fs
	addl $4, %esp
	iret

invalid_syscall:
	movl $(-1), 24(%esp)
	jmp ret_from_syscall

syscall_jump:
	.long 0, halt, execute, read, write, open, close

.long 0
.extern syscall_halt, .long syscall_halt
.extern syscall_execute, .long syscall_execute
.extern syscall_read, .long syscall_read
.extern syscall_write, .long syscall_write
.extern syscall_open, .long syscall_open
.extern syscall_close, .long syscall_close

halt:
	pushl %ebx
	call syscall_halt
	addl $4, %esp
	jmp ret_from_syscall

execute:
	pushl %ebx
	call syscall_execute
	addl $4, %esp
	jmp ret_from_syscall

read:
	pushl %edx
	pushl %ecx
	pushl %ebx
	call syscall_read
	addl $12, %esp
	jmp ret_from_syscall

write:
	pushl %edx
	pushl %ecx
	pushl %ebx
	call syscall_write
	addl $12, %esp
	jmp ret_from_syscall

open:
	pushl %ebx
	call syscall_open
	addl $4, %esp
	jmp ret_from_syscall

close:
	pushl %ebx
	call syscall_close
	addl $4, %esp
	jmp ret_from_syscall
