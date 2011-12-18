global compute_point:function
global generate:function

section .data
align 16

; bit mask used to count absolute values of 2 doubles in xmm
mask dq 0x7fffffffffffffff
     dq 0x7fffffffffffffff

; floating point minus one
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
; st = ecx - log2(log(abs)/log(N)) = ecx - log2(log2(abs^2)/log2(N^2))
; input:
; 	lower xmm0: squared absolute value of complex point
; 	lower xmm5: squared bailout radius N
; 	ecx       : number of iterations
; local vars:
; 	dword [ebp-4] : number of iterations
; 	qword [ebp-12]: xmm5 (N)
; 	qword [ebp-20]: xmm0 (absolute value)
; output:
; 	st        : output value v
compute_v:
	; two local doubles, one integer, 32 bytes total for alignment
	enter 0x20, 0
	; move abs, N and number of iterations to stack memory
	movq qword [ebp-20], xmm0
	movq qword [ebp-12], xmm5
	mov dword [ebp-4], ecx
	; do the magic
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

; Loads -1 and abs bit mask to lower xmm7 and xmm6, respectively
load_static_data:
	get_GOT
	; lower xmm7: -1
	lea eax, [ebx+mone wrt ..gotoff]
	movq xmm7, [eax]
	; xmm6: sign bit mask
	lea eax, [ebx+mask wrt ..gotoff]
	movupd xmm6, [eax]
	ret

; Performs at most k iterations. Returns number of iteratation after which the
; value exceeded bailout radius
; arguments:
;	c - input complex value
;	N - bailout radius
;	k - number of iterations
;	v - pointer to double where the v value will be stored
compute_point:
	enter 0, 0
	call load_static_data
	; xmm0 - c (= c + di)
	movupd xmm0, [ebp+8]
	; lower xmm5: N (bailout radius)
	movq xmm5, [ebp+24]
	; N^2
	movq xmm4, xmm5
	mulsd xmm5, xmm4
	; eax: max number of iterations
	mov eax, [ebp+32] ; k
	call compute_point_impl
	; move calculated value to v argument
	mov ebx, [ebp+36] ; v
	fstp qword [ebx]
	leave
	ret

; Calculates fuzzy belonging coefficient for one complex point
; assumes:
; 	lower xmm7: -1.0
; 	xmm6      : sign bitmask used to count abs
; input:
; 	xmm0      : input complex value
; 	lower xmm5: squared bailout radius
; 	eax       : max number of iterations
; output:
; 	st        : computed output value
;       eax       : 0: point in set; != 0: point not in set
compute_point_impl:
	call iterate ; puts number of iterations in ecx
	; compute how many iterations to the end
	sub eax, ecx
	call compute_v ;leaves v in st
	ret

generate:
	enter 0, 0
	; load max_x and max_y
	movupd xmm0, [ebp+64]
	; load min_x and min_y
	movupd xmm1, [ebp+48]
	; subtract mins from maxs, ranges in xmm0
	subpd xmm0, xmm1
	; load width and height
	cvtdq2pd xmm1, [ebp+12]
	; divide by width and height, steps in xmm0
	divpd xmm0, xmm1
	; edx: pointer to buffer
	mov edx, [ebp+8] ; buf
	; buf += frame_y*width + frame_x;
	mov eax, [ebp+24] ; frame_y
	mov ebx, [ebp+12] ; width
	imul eax, ebx
	add eax, [ebp+20] ; frame_x
	add edx, eax
	; ebx: row counter
	mov ebx, 0
rows_loop:
	cmp ebx, [ebp+32] ; frame_h
	je rows_loop_end
	; ---
	mov ecx, 0
cols_loop:
	cmp ecx, [ebp+28] ; frame_w
	je cols_loop_end

	mov byte [edx+ecx], 0

	add ecx, 1
	jmp cols_loop
cols_loop_end:
	; move buffer one line down
	add edx, [ebp+12] ; width
	; ---
	add ebx, 1
	jmp rows_loop
rows_loop_end:
	leave
	ret
