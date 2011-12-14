%include "c32.mac"

global iter:function

section .data

mask dq 0x7fffffffffffffff
     dq 0x7fffffffffffffff

mone dq -1.0

section .text

extern  _GLOBAL_OFFSET_TABLE_

%macro  get_GOT 0
	call    %%getgot
%%getgot:
	pop     ebx
	add     ebx,_GLOBAL_OFFSET_TABLE_+$$-%%getgot wrt ..gotpc
%endmacro

; Performs at most k iterations. Returns number of iteratation after which the
; value exceeded bailout radius
; arguments:
;	c - pointer to input complex value
;	N - pointer to bailout radius
;	k - number of iterations
;	abs - pointer to double where the absolute value will be stored
proc iter
	get_GOT
	; lower xmm7: -1
	lea eax, [ebx+mone wrt ..gotoff]
	movq xmm7, [eax]
	; xmm6: sign bit mask
	lea eax, [ebx+mask wrt ..gotoff]
	movapd xmm6, [eax]
	; xmm0 - z (= a + bi) = 0 + 0i
	pxor xmm0, xmm0
	; xmm1 - c (= c + di)
	mov eax, [ebp+8]
	movapd xmm1, [eax]
	; lower xmm5: N (bailout radius)
	mov eax, [ebp+12]
	movq xmm5, [eax]
	; eax: number of iterations
	mov eax, [ebp+16]
	mov ecx, 0
loop_beg:
	cmp ecx, eax
	je bailout
	andpd xmm0, xmm6
	movapd xmm2, xmm0
	mulpd xmm2, xmm0 ; xmm2: a^2 and b^2
	movhlps xmm3, xmm2 ; b^2 to lower xmm3
	addsd xmm3, xmm2 ; lower xmm3: a^2+b^2
	sqrtsd xmm3, xmm3 ; lower xmm3: abs z
	comisd xmm5, xmm3 ; if greater than bailout radius...
	jc bailout
	movaps xmm3, xmm2 ; copy squares
	movhlps xmm3, xmm0 ; b to lower xmm3
	movlhps xmm0, xmm7 ; -1 to higher xmm0
	mulpd xmm3, xmm0 ; lower xmm3: ab, higher xmm3: -b^2
	movaps xmm0, xmm3
	movlhps xmm0, xmm2
	addpd xmm3, xmm0
	movhlps xmm0, xmm3
	movlhps xmm0, xmm3
	addpd xmm0, xmm1
	inc ecx
	jmp loop_beg
bailout:
	mov eax, [ebp+20]
	movq [eax], xmm3 ; store abs in third argument
	; return number of iterations
	mov eax, ecx
	leave
	ret
