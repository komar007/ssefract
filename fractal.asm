global compute_point:function

section .data
align 16

mask dq 0x7fffffffffffffff
     dq 0x7fffffffffffffff

mone dq -1.0

section .text

extern  _GLOBAL_OFFSET_TABLE_
; Macro used to store the _GLOBAL_OFFSET_TABLE_ address in ebx
%macro  get_GOT 0
	call    %%getgot
%%getgot:
	pop     ebx
	add     ebx,_GLOBAL_OFFSET_TABLE_+$$-%%getgot wrt ..gotpc
%endmacro

; Iterate burningship pixel and return number of iterations and absolute value
; of complex number after iterating
; assumes:
; 	lower xmm7: -1.0
; 	xmm6      : sign bitmask used to count abs
; input:
; 	eax       : max number of iterations
; 	xmm0      : complex number c
; 	lower xmm5: squared bailout radius
; output:
; 	ecx       : number of iterations
; 	lower xmm0: final squared absolute value
iterate:
	; copy input complex to xmm1
	movapd xmm1, xmm0
	; xmm0: iterated complex z (initialize to 0 + 0i)
	pxor xmm0, xmm0
	; ecx: loop counter
	mov ecx, 0
loop_beg:
	cmp ecx, eax
	je bailout
	andpd xmm0, xmm6
	movapd xmm2, xmm0
	mulpd xmm2, xmm0 ; xmm2: a^2 and b^2
	movhlps xmm3, xmm2 ; b^2 to lower xmm3
	addsd xmm3, xmm2 ; lower xmm3: a^2+b^2
	comisd xmm5, xmm3 ; if greater than bailout radius...
	jc bailout
	movapd xmm3, xmm2 ; copy squares
	movhlps xmm3, xmm0 ; b to lower xmm3
	movlhps xmm0, xmm7 ; -1 to higher xmm0
	mulpd xmm3, xmm0 ; lower xmm3: ab, higher xmm3: -b^2
	movapd xmm0, xmm3
	movlhps xmm0, xmm2
	addpd xmm3, xmm0
	movhlps xmm0, xmm3
	movlhps xmm0, xmm3
	addpd xmm0, xmm1
	add ecx, 1
	jmp loop_beg
bailout:
	; return squared absolute value to lower xmm0
	movq xmm0, xmm3
	ret

; Computes fuzzy set belonging coefficient
; input:
; 	lower xmm0: absolute value of complex point
; 	lower xmm5: bailout radius N
; 	ecx       : number of iterations
; output:
; 	st        : output value v
compute_v:
	; two local doubles, one integer, 32 bytes total for alignment
	enter 0x20, 0
	; move abs, N and number of iterations to stack memory
	movq qword [ebp-20], xmm0
	movq qword [ebp-12], xmm5
	mov dword [ebp-4], ecx
	fild dword [ebp-4]
	fld1
	fld1
	fld qword [ebp-20]
	fyl2x
	fld1
	fld qword [ebp-12]
	fyl2x
	fdiv
	fyl2x
	fsub
	leave
	ret

; Performs at most k iterations. Returns number of iteratation after which the
; value exceeded bailout radius
; arguments:
;	c - pointer to input complex value
;	N - pointer to bailout radius
;	k - number of iterations
;	v - pointer to double where the v value will be stored
compute_point:
	; local vars:
	; 	qword [ebp-8]  : xmm5 (N)
	; 	qword [ebp-16] : xmm0 (absolute value)
	; 	dword [ebp-20] : number of iterations
	enter 20, 0
	get_GOT
	; lower xmm7: -1
	lea eax, [ebx+mone wrt ..gotoff]
	movq xmm7, [eax]
	; xmm6: sign bit mask
	lea eax, [ebx+mask wrt ..gotoff]
	movupd xmm6, [eax]
	; xmm0 - c (= c + di)
	mov eax, [ebp+8]
	movupd xmm0, [eax]
	; lower xmm5: N (bailout radius)
	mov eax, [ebp+12]
	movq xmm5, [eax]
	; N^2
	movq xmm4, xmm5
	mulsd xmm5, xmm4
	; eax: max number of iterations
	mov eax, [ebp+16]
	call iterate
	call compute_v
	; move calculated value to v argument
	mov ebx, [ebp+20]
	fstp qword [ebx]
	; return whether point belongs to set
	sub eax, ecx
	leave
	ret
