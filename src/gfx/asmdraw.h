#ifndef ASMDRAW_H
#define ASMDRAW_H

class COLOR;

void AsmDraw32(unsigned char* p_src, short* p_zbuf, int* p_dest, int p_count, short p_z, const int* p_palette);
void AsmDrawWithAlpha32(
	unsigned char* p_src,
	unsigned short* p_zbuf,
	COLOR* p_dest,
	int p_count,
	unsigned short p_z,
	const int* p_palette
);
void AsmDrawWithAlpha16(
	unsigned char* p_src,
	unsigned short* p_zbuf,
	unsigned short* p_dest,
	int p_count,
	unsigned short p_z,
	const int* p_palette
);
void AsmDrawAlphaWithZ(const void* p_zdelta, const void* p_src, short* p_zdst, short* p_dst, int p_count, int p_baseZ);
void AsmDrawLightWithZ(const void* p_zdelta, const void* p_src, short* p_zdst, short* p_dst, int p_count, int p_baseZ);

#endif
