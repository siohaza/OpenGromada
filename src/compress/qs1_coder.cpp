#include "compress/qs1_coder.h"

#include "compress/r_coder.h"

// FUNCTION: ALIEN 0x425a20
void QS1_CODER::Reset()
{
	QSMODEL* p = m_models;
	int n = 256;
	do {
		p->Reset(0);
		++p;
		--n;
	} while (n);
}

// STUB: ALIEN 0x425a40
int QS1_CODER::Write(const void* p_buf, int p_size, FILE* p_file)
{
	const unsigned char* buffer = (const unsigned char*) p_buf;
	int prevSym = 0;
	unsigned int half = ((unsigned int) p_size + 1) >> 1;
	unsigned int i = 0;
	R_CODER rc;
	rc.m_file = 0;
	if (!p_file || !p_size)
		return 0;
	if ((unsigned int) p_size < 10)
		return fwrite(buffer, 1, p_size, p_file);

	rc.start_encoding(0, 0, (int) p_file);
	int syFreq;
	int ltFreq;
	int interleaved = 0;
	while (i < (unsigned int) p_size) {
		int sym;
		if (m_mode == 2)
			sym = (i < half) ? buffer[interleaved] : buffer[interleaved - 2 * half + 1];
		else
			sym = buffer[i];
		QSMODEL* model = &m_models[prevSym];
		model->GetFreq(sym, &syFreq, &ltFreq);
		rc.EncodeShift(syFreq, ltFreq, 12);
		model->Update(sym);
		++i;
		interleaved += 2;
		prevSym = sym;
	}
	m_models[prevSym].GetFreq(256, &syFreq, &ltFreq);
	rc.EncodeShift(syFreq, ltFreq, 12);
	return rc.EndEncoding();
}

// FUNCTION: ALIEN 0x425b90
int QS1_CODER::Read(void* p_buffer, int p_count, FILE* p_stream)
{
	int done = 0;
	int syFreq;
	unsigned int half = ((unsigned int) p_count + 1) >> 1;
	int prevSym = 0;
	R_CODER rc;
	rc.m_file = 0;
	if (!p_stream || !p_count)
		return 0;
	if ((unsigned int) p_count < 10)
		return fread(p_buffer, 1, p_count, p_stream);
	if (rc.StartDecoding(p_stream) < 0)
		return 0;
	int sym;
	int ltFreq;
	while (1) {
		ltFreq = rc.DecodeCulShift(12);
		sym = m_models[prevSym].GetSym(ltFreq);
		if (sym == 256)
			break;
		if ((unsigned int) done >= (unsigned int) p_count) {
			fprintf(stderr,
					// STRING: ALIEN 0x483694
					"!!!ERROR!!! decode");
			break;
		}
		if (m_mode == 2) {
			if ((unsigned int) done < half)
				((char*) p_buffer)[2 * done++] = sym;
			else
				((char*) p_buffer)[2 * done++ - 2 * half + 1] = sym;
		}
		else {
			((char*) p_buffer)[done++] = sym;
		}
		m_models[prevSym].GetFreq(sym, &syFreq, &ltFreq);
		rc.DecodeUpdate(syFreq, ltFreq, 0x1000);
		m_models[prevSym].Update(sym);
		prevSym = sym;
	}
	m_models[prevSym].GetFreq(256, &syFreq, &ltFreq);
	rc.DecodeUpdate(syFreq, ltFreq, 0x1000);
	while (rc.m_range <= 0x800000) {
		rc.m_low = (rc.m_low << 8) | ((rc.m_byte << 7) & 0xff);
		rc.m_byte = fgetc((FILE*) rc.m_file);
		rc.m_low |= rc.m_byte >> 1;
		rc.m_range <<= 8;
	}
	return done;
}
