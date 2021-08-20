	.file	"lcmc.c"
	.text
.globl gcfa
	.type	gcfa, @function
gcfa:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	jmp	.L2
.L4:
	movl	-4(%rbp), %eax
	cmpl	-8(%rbp), %eax
	jle	.L3
	movl	-8(%rbp), %eax
	subl	%eax, -4(%rbp)
	jmp	.L2
.L3:
	movl	-4(%rbp), %eax
	subl	%eax, -8(%rbp)
.L2:
	movl	-4(%rbp), %eax
	cmpl	-8(%rbp), %eax
	jne	.L4
	movl	-4(%rbp), %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	gcfa, .-gcfa
.globl divide
	.type	divide, @function
divide:
.LFB1:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -20(%rbp)
	movl	%esi, -24(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	$0, -4(%rbp)
	jmp	.L7
.L8:
	movl	-24(%rbp), %eax
	subl	%eax, -8(%rbp)
	addl	$1, -4(%rbp)
.L7:
	movl	-8(%rbp), %eax
	cmpl	-24(%rbp), %eax
	jge	.L8
	movl	-4(%rbp), %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	divide, .-divide
.globl remain
	.type	remain, @function
remain:
.LFB2:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -20(%rbp)
	movl	%esi, -24(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	.L11
.L12:
	movl	-24(%rbp), %eax
	subl	%eax, -4(%rbp)
.L11:
	movl	-4(%rbp), %eax
	cmpl	-24(%rbp), %eax
	jge	.L12
	movl	-4(%rbp), %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	remain, .-remain
.globl lcma
	.type	lcma, @function
lcma:
.LFB3:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$24, %rsp
	movl	%edi, -20(%rbp)
	movl	%esi, -24(%rbp)
	movl	-24(%rbp), %edx
	movl	-20(%rbp), %eax
	movl	%edx, %esi
	movl	%eax, %edi
	call	gcfa
	movl	%eax, %edx
	movl	-20(%rbp), %eax
	imull	-24(%rbp), %eax
	movl	%edx, %esi
	movl	%eax, %edi
	call	divide
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	lcma, .-lcma
	.ident	"GCC: (GNU) 4.4.7 20120313 (Red Hat 4.4.7-23)"
	.section	.note.GNU-stack,"",@progbits
