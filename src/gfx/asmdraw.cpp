#define DECOMP_COLOR_COPY_OUT_OF_LINE
#define DECOMP_COLOR_RGB16_CTOR_OUT_OF_LINE

#include "util/decomp.h"

#include "gfx/color.h"

#pragma data_seg(".data")
__declspec(allocate(".data")) short AsmDrawData[12] = {
	0, 0, 0, 0,
	(short) 0x8001, (short) 0x8001, (short) 0x8001, (short) 0x8001,
	0, 0, 0, 0
};

__declspec(allocate(".data")) int* AsmDrawPalette = 0;
#pragma data_seg()

// FUNCTION: ALIEN 0x4165c0
void AsmDraw32(unsigned char* p_src, short* p_zbuf, int* p_dest, int p_count)
{
	__asm {
		mov edi, p_dest
		mov ebx, p_zbuf
		mov ecx, AsmDrawPalette
		mov eax, dword ptr AsmDrawData
	again:
		cmp ax, [ebx]
		jle skip
		mov [ebx], ax
		mov edx, p_src
		movzx edx, byte ptr [edx]
		mov edx, [ecx+edx*4]
		mov [edi], edx
	skip:
		add edi, 4
		add ebx, 2
		inc p_src
		dec p_count
		jg again
	}
}

// FUNCTION: ALIEN 0x416600
void AsmDrawWithAlpha32(unsigned char* p_src, unsigned short* p_zbuf, COLOR* p_dest, int p_count)
{
	for (int i = 0; i < p_count; ++i) {
		if ((unsigned short) AsmDrawData[0] >= p_zbuf[i]) {
			COLOR* color = (COLOR*) &AsmDrawPalette[p_src[i]];
			p_dest[i].AlphaAdd(*color, (unsigned int) color->m_value >> 24);
		}
	}
}

// FUNCTION: ALIEN 0x416740
void AsmDrawWithAlpha16(unsigned char* p_src, unsigned short* p_zbuf, unsigned short* p_dest,
	int p_count)
{
	for (int i = 0; i < p_count; ++i) {
		if ((unsigned short) AsmDrawData[0] >= p_zbuf[i]) {
			COLOR* color = (COLOR*) &AsmDrawPalette[p_src[i]];
			unsigned int c = (unsigned int) COLOR(&p_dest[i])
				.AlphaAdd(*color, (unsigned int) color->m_value >> 24).m_value;
			p_dest[i] = (unsigned short) (((c >> 3) & 0x1f)
				| (RGB16_rMask & (c >> (16 - RGB16_rShift)))
				| (RGB16_gMask & (c >> (8 - RGB16_gShift))));
		}
	}
}

// FUNCTION: ALIEN 0x41af80
__declspec(naked) void AsmDrawAlphaWithZ(short*, short*, short*, short*, int)
{
	__asm {
		mov eax, [esp+0x14]
		dec eax
		js done
		push ebx
		mov ebx, [esp+0x14]
		push ebp
		push esi
		inc eax
		push edi
		mov edi, [esp+0x18]
		mov [esp+0x24], eax
	again:
		mov eax, [esp+0x14]
		movsx esi, word ptr [eax]
		movsx eax, word ptr AsmDrawData
		mov edx, [esp+0x1c]
		movsx edx, word ptr [edx]
		lea ecx, [eax+esi]
		cmp ecx, edx
		jge notclear
		mov word ptr [ebx], 0
		jmp advance
	notclear:
		lea ebp, [edx+0x7f]
		cmp ecx, ebp
		jle blend
		mov ax, [edi]
		mov [ebx], ax
		jmp advance
	blend:
		xor ecx, ecx
		mov cx, [edi]
		sub eax, edx
		add eax, esi
		mov edx, ecx
		and edx, 0xffff
		imul eax, edx
		sar eax, 7
		cmp eax, 0xffff
		jle noclamp
		mov eax, 0xf000
		jmp merge
	noclamp:
		and eax, 0xf000
	merge:
		and ecx, 0xfff
		add ecx, eax
		mov [ebx], cx
	advance:
		mov ebp, [esp+0x14]
		mov esi, [esp+0x1c]
		mov eax, 2
		add ebp, eax
		add esi, eax
		add ebx, eax
		add edi, eax
		mov eax, [esp+0x24]
		dec eax
		mov [esp+0x14], ebp
		mov [esp+0x1c], esi
		mov [esp+0x24], eax
		jnz again
		pop edi
		pop esi
		pop ebp
		pop ebx
	done:
		ret
	}
}

// FUNCTION: ALIEN 0x41b040
__declspec(naked) void AsmDrawLightWithZ(short*, short*, short*, short*, int)
{
	__asm {
		mov eax, [esp+0x14]
		dec eax
		js done
		push ebx
		mov ebx, [esp+0xc]
		push ebp
		push esi
		inc eax
		push edi
		mov edi, [esp+0x20]
		mov [esp+0x24], eax
	again:
		mov eax, [esp+0x14]
		movsx edx, word ptr [eax]
		movsx eax, word ptr AsmDrawData
		mov ecx, [esp+0x1c]
		movsx ecx, word ptr [ecx]
		lea esi, [eax+edx]
		cmp esi, ecx
		jge notdark
		mov word ptr [edi], 0
		jmp advance
	notdark:
		lea ebp, [ecx+0x7f]
		cmp esi, ebp
		jle blend
		mov dx, [ebx]
		or dx, 0xf000
		mov [edi], dx
		jmp advance
	blend:
		sub eax, ecx
		add eax, edx
		cdq
		and edx, 7
		add eax, edx
		sar eax, 3
		shl eax, 0xc
		or ax, [ebx]
		mov [edi], ax
	advance:
		mov ebp, [esp+0x14]
		mov esi, [esp+0x1c]
		mov eax, 2
		add ebp, eax
		add esi, eax
		add edi, eax
		add ebx, eax
		mov eax, [esp+0x24]
		dec eax
		mov [esp+0x14], ebp
		mov [esp+0x1c], esi
		mov [esp+0x24], eax
		jnz again
		pop edi
		pop esi
		pop ebp
		pop ebx
	done:
		ret
	}
}
