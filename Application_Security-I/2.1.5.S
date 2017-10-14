.global _main
.section .text

_main:

push	%ebp
mov	%esp,%ebp
mov $0x0b, %eax		#sys call number for sys_execve
mov $filename, %ebx
xor %ecx, %ecx 		# clears ecx; no *argv[]
xor %edx, %edx		# no *envp[]
int $0x80

leave
ret

.data
filename:	.string "/bin/sh"