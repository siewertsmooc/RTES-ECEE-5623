/* Hand written ARM assembly to show potential advantages of custom
   assembly code authoring over relying solely on a compiler.

   This code computes the greatest common factor of two numbers and
   computes the least common multiple of two numbers using the greatest
   common factor.

*/
	.text
	.align	2
	.global	gcfa
	.type	gcfa, %function
gcfa:
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	sub	sp, sp, #8
	str	r0, [fp, #-16]
	str	r1, [fp, #-20]

	b	WHLCHK

LOOPBDY:
	ldr	r2, [fp, #-16] /* r2=a */
	ldr	r3, [fp, #-20] /* r3=b */
	cmp	r2, r3
	ble	ELSBR

	ldr	r3, [fp, #-16] /* r3=a */
	ldr	r2, [fp, #-20] /* r2=b */
	rsb	r3, r2, r3     /* r3=r3-r2 */
	str	r3, [fp, #-16] /* r3=a (a=a-b) */
	b	WHLCHK

ELSBR:
	ldr	r3, [fp, #-20]
	ldr	r2, [fp, #-16]
	rsb	r3, r2, r3
	str	r3, [fp, #-20]

WHLCHK:
	ldr	r2, [fp, #-16] /* r2=a */
	ldr	r3, [fp, #-20] /* r3=b */
	cmp	r2, r3         /* is a == b? */
	bne	LOOPBDY

	ldr	r3, [fp, #-16] /* r3=a */
	mov	r0, r3         /* return value is a */

        /* return a */
	sub	sp, fp, #12
	ldmfd	sp, {fp, sp, lr}
	bx	lr


	.text
	.align	2
	.global	gcfa1
	.type	gcfa1, %function
gcfa1:
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	sub	sp, sp, #8
gcfa1r:
	str	r0, [fp, #-16]
	str	r1, [fp, #-20]

        cmp	r0, r1
	beq	END
	blt	LESS
	sub	r0, r0, r1
	b	gcfa1r

LESS:	
	sub	r1, r1, r0 
	b	gcfa1r

END:
        sub     sp, fp, #12
        ldmfd   sp, {fp, sp, lr}
        bx      lr


	.text
	.align	2
	.global	gcfa2
	.type	gcfa2, %function
gcfa2:
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	sub	sp, sp, #8
gcfa2r:
	str	r0, [fp, #-16]
	str	r1, [fp, #-20]

	cmp	r0, r1
	subgt	r0, r0, r1
	sublt	r1, r1, r0
	bne	gcfa2r

END2:
        sub     sp, fp, #12
        ldmfd   sp, {fp, sp, lr}
        bx      lr


	.text
	.align	2
	.global	divide
	.type	divide, %function
divide:
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        sub     sp, sp, #20
        str     r0, [fp, #-16]
        str     r1, [fp, #-20]
        ldr     r3, [fp, #-16]
        str     r3, [fp, #-8]
        mov     r3, #0
        str     r3, [fp, #-12]
        b       .L7
.L8:
        ldr     r2, [fp, #-8]
        ldr     r3, [fp, #-20]
        sub     r3, r2, r3
        str     r3, [fp, #-8]
        ldr     r3, [fp, #-12]
        add     r3, r3, #1
        str     r3, [fp, #-12]
.L7:
        ldr     r2, [fp, #-8]
        ldr     r3, [fp, #-20]
        cmp     r2, r3
        bge     .L8
        ldr     r3, [fp, #-12]
        mov     r0, r3
        add     sp, fp, #0
        ldr     fp, [sp], #4
        bx      lr


	.text
	.align	2
	.global	remain
	.type	remain, %function
remain:
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        sub     sp, sp, #20
        str     r0, [fp, #-16]
        str     r1, [fp, #-20]
        ldr     r3, [fp, #-16]
        str     r3, [fp, #-8]
        b       .L11
.L12:
        ldr     r2, [fp, #-8]
        ldr     r3, [fp, #-20]
        sub     r3, r2, r3
        str     r3, [fp, #-8]
.L11:
        ldr     r2, [fp, #-8]
        ldr     r3, [fp, #-20]
        cmp     r2, r3
        bge     .L12
        ldr     r3, [fp, #-8]
        mov     r0, r3
        add     sp, fp, #0
        ldr     fp, [sp], #4
        bx      lr


	.text
	.align	2
	.global	lcma
	.type	lcma, %function
lcma:
        push    {r4, fp, lr}
        add     fp, sp, #8
        sub     sp, sp, #20
        str     r0, [fp, #-24]
        str     r1, [fp, #-28]
        ldr     r3, [fp, #-24]
        ldr     r2, [fp, #-28]
        mul     r4, r2, r3
        ldr     r1, [fp, #-28]
        ldr     r0, [fp, #-24]

        bl      gcfa

        mov     r3, r0
        mov     r1, r3
        mov     r0, r4

        bl      __aeabi_idiv

        mov     r3, r0
        str     r3, [fp, #-16]
        ldr     r3, [fp, #-16]
        mov     r0, r3
        sub     sp, fp, #8
        pop     {r4, fp, pc}

