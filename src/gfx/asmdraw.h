#ifndef ASMDRAW_H
#define ASMDRAW_H

class COLOR;

// GLOBAL: ALIEN 0x482ac8
extern short AsmDrawData[12];
// GLOBAL: ALIEN 0x482ae0
extern int* AsmDrawPalette;

void AsmDraw32(unsigned char* p_src, short* p_zbuf, int* p_dest, int p_count);
void AsmDrawWithAlpha32(unsigned char* p_src, unsigned short* p_zbuf, COLOR* p_dest, int p_count);
void AsmDrawWithAlpha16(unsigned char* p_src, unsigned short* p_zbuf, unsigned short* p_dest,
	int p_count);
void AsmDrawAlphaWithZ(short*, short*, short*, short*, int);
void AsmDrawLightWithZ(short*, short*, short*, short*, int);

#endif
