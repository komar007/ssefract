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
; 	xmm0      : complex number c (will not be modified internally)
; 	lower xmm5: squared bailout radius
; output:
; 	esi       : number of iterations
; 	lower xmm3: final squared absolute value
iterate:
	; xmm1: iterated complex z (initialize to 0 + 0i)
	pxor xmm1, xmm1
	; esi: loop counter
	mov esi, 0
loop_beg:
	cmp esi, eax
	je bailout
	andpd xmm1, xmm6
	movapd xmm2, xmm1
	mulpd xmm2, xmm1 ; xmm2: a^2 and b^2
	movhlps xmm3, xmm2 ; b^2 to lower xmm3
	addsd xmm3, xmm2 ; lower xmm3: a^2+b^2
	comisd xmm5, xmm3 ; if greater than bailout radius...
	jc bailout
	movapd xmm3, xmm2 ; copy squares
	movhlps xmm3, xmm1 ; b to lower xmm3
	movlhps xmm1, xmm7 ; -1 to higher xmm0
	mulpd xmm3, xmm1 ; lower xmm3: ab, higher xmm3: -b^2
	movapd xmm1, xmm3
	movlhps xmm1, xmm2
	addpd xmm3, xmm1
	movhlps xmm1, xmm3
	movlhps xmm1, xmm3
	addpd xmm1, xmm0
	add esi, 1
	jmp loop_beg
bailout:
	ret

; Computes fuzzy set belonging coefficient
; st = esi - log2(log(abs)/log(N)) = esi - log2(log2(abs^2)/log2(N^2))
; input:
; 	lower xmm3: squared absolute value of complex point
; 	lower xmm5: squared bailout radius N
; 	esi       : number of iterations
; local vars:
; 	dword [ebp-4] : number of iterations
; 	qword [ebp-12]: xmm5 (N)
; 	qword [ebp-20]: lower xmm3 (absolute value)
; output:
; 	st        : output value v
compute_v:
	; two local doubles, one integer, 32 bytes total for alignment
	enter 0x20, 0
	; move abs, N and number of iterations to stack memory
	movq qword [ebp-20], xmm3
	movq qword [ebp-12], xmm5
	mov dword [ebp-4], esi
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
	call iterate ; puts number of iterations in esi
	; compute how many iterations to the end
	sub eax, esi
	call compute_v ;leaves v in st
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

; local vars:
; 	dqword [esp]     : min_x, min_y
; 	qword  [esp+16]  : result for one point
generate:
	; 32 bytes for local data just to be sure nothing gets in the way
	; after aligning
	enter 32, 0
	; align stack pointer to 16 to have local vars aligned
	and esp, ~0xf
	; load xmm6 and xmm7 with static values
	call load_static_data
	; edx: pointer to buffer
	mov edx, [ebp+8] ; buf
	; buf += frame_y*width + frame_x;
	mov eax, [ebp+24] ; frame_y
	mov ebx, [ebp+12] ; width
	imul eax, ebx
	add eax, [ebp+20] ; frame_x
	add edx, eax
	; --------- begin count step_x, step_y
	; load max_x and max_y
	movupd xmm4, [ebp+64]
	; load min_x and min_y
	movupd xmm1, [ebp+48]
	; save min_x and min_y in aligned memory
	movapd [esp], xmm1
	; subtract mins from maxs, ranges in xmm4
	subpd xmm4, xmm1
	; load width and height
	cvtdq2pd xmm1, [ebp+12]
	; divide by width and height, steps in xmm4
	divpd xmm4, xmm1
	; /-------- end count step_x, step_y
	; load lower xmm5 with N^2 (bailout radius squared), save N in higher xmm5
	movq xmm5, [ebp+36] ; N
	movlhps xmm5, xmm5
	mulsd xmm5, xmm5
	; edi: max number of iterations
	mov edi, [ebp+44] ; iter
	; ebx: row counter
	mov ebx, 0
rows_loop:
	cmp ebx, [ebp+32] ; frame_h
	je rows_loop_end
	; ---
	; prepare c at the beginning of line:
	; c = (min_x + step_x*frame_x) + (min_y + step_y*frame_y)i
	; frame_y is changed for each iteration of rows_loop
	movapd xmm0, [esp] ; min_x, min_y
	movapd xmm1, xmm4 ; step_x, step_y
	cvtdq2pd xmm2, [ebp+20] ; frame_x, frame_y
	mulpd xmm1, xmm2
	addpd xmm0, xmm1
	; start outer column loop
	mov ecx, 0
cols_loop:
	cmp ecx, [ebp+28] ; frame_w
	je cols_loop_end
	; load max number of iterations to eax
	mov eax, edi
	call compute_point_impl ; leaves 0 in eax if in set, != 0 otherwise
	; store floating point result in local var
	fstp qword [esp+16] ; local var (result)
	mov [edx+ecx], eax
	; add step_x + 0i to c
	addsd xmm0, xmm4 ; xmm4: steps
	add ecx, 1
	jmp cols_loop
cols_loop_end:
	; move buffer one line down
	add edx, [ebp+12] ; width
	; increment frame_y - the line number
	add dword [ebp+24], 1 ; frame_y
	; ---
	add ebx, 1
	jmp rows_loop
rows_loop_end:
	leave
	ret

; Wrapper around compute_point_impl to use from C
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
