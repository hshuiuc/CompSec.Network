.global your_asm_fn
.section .text

your_asm_fn:

push	%ebp
mov	%esp,%ebp
push $mp
push $sec
push $0xacce55ed
call practice

leave
ret

.data
sec:	.long 0xc105ed
mp:		.string "coolfood"