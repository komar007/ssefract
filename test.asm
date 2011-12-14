%include "c32.mac"

global iter:function

proc iter
	mov eax, [ebp+16]
	movq xmm7, [eax] ; lower xmm7: -1
	mov eax, [ebp+20]
	movapd xmm6, [eax] ; xmm6 - masks
	; xmm0 - z (= a + bi)
	; xmm1 - c (= c + di)
	mov eax, [ebp+8]
	movapd xmm0, [eax]
	mov eax, [ebp+12]
	movapd xmm1, [eax]

	andpd xmm0, xmm6
	movapd xmm2, xmm0
	mulpd xmm2, xmm0 ; xmm2 - a^2 and b^2
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

	mov eax, [ebp+8]
	movapd [eax], xmm0
	leave
	ret
