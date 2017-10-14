.global your_asm_fn
.section .text

your_asm_fn:

push	%ebp
mov	%esp,%ebp
push $mp
push $0xc105ed
push $0xacce55ed
call practice

leave
ret

.data
mp: .long 0xc001f00d