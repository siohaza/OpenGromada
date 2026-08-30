#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/picture_font.h"
#include "gfx/picture_makevid.h"
#include "platform/paths.h"
#include "util/myerror.h"
#include "video/vid_font.h"
#include "video/vid_hardware.h"
#include "video/vid_hardware_z.h"
#include "video/vid_light.h"
#include "video/vid_software.h"
#include "video/vid_software16.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// STUB: ALIEN 0x411b50
VID* MAP::CreateVid(RESOURCE* p_res, int p_idx)
{
	VID* vid = 0;
	STRING name;
	VID scratch;
	name.Read_res(p_res);
	scratch.m_dotFrameCount = 32000;
	long paramsPos = ftell(p_res->m_file);
	scratch.LoadParameters(p_res);

	STRING fname;
	fname.Read_res(p_res);
	fname = fname.ToLower();

	p_res->m_state = 2;
	fseek(p_res->m_file, paramsPos, SEEK_SET);

	int i;
	for (i = 0; i < m_noVid; ++i) {
		if (!VidExists(i)) {
			continue;
		}
		VID* other = m_vids[i];
		if (strcmp(other->m_fname, fname.m_str)) {
			continue;
		}
		if (scratch.m_sprClass == 8) {
			if (other->m_sprClass != 8) {
				continue;
			}
		}
		else if (other->m_sprClass == 8) {
			continue;
		}
		if (!(other->m_pixelFlag16 & 0x20) &&
			(other->m_colorSub != scratch.m_colorSub || other->m_colorAdd != scratch.m_colorAdd)) {
			continue;
		}
		if (!(other->m_pixelFlag16 & 0x20) && ((scratch.m_flag ^ other->m_flag) & 0x800)) {
			continue;
		}
		vid = other->CreateMirror();
		vid->m_idx = p_idx;
		vid->SetName(name.m_str);
		vid->SetFileName(fname.m_str);
		vid->LoadParameters(p_res);
		break;
	}
	if (i < m_noVid) {
		vid->SetLayer();
		return vid;
	}

	int isFont = strstr(
					 fname.m_str,
					 // STRING: ALIEN 0x482a68
					 ".fon"
				 ) ||
				 strstr(
					 fname.m_str,
					 // STRING: ALIEN 0x482a60
					 ".ttf"
				 );
	int isPicture = strstr(
						fname,
						// STRING: ALIEN 0x47f738
						".tga"
					) ||
					strstr(
						fname,
						// STRING: ALIEN 0x482a58
						".bmp"
					) ||
					strstr(
						fname,
						// STRING: ALIEN 0x482a50
						".flc"
					);
	int removeTemp = 0;

	RESOURCE vidFile;
	if (isFont) {
		vid = new VID_FONT;
	}
	else {
		if (isPicture) {
			PICTURE_MAKEVID* converter = scratch.m_sprClass == 19 ? new PICTURE_FONT : new PICTURE_MAKEVID;
			if (converter->Load(STRING(fname.m_str), STRING(empty_str), STRING(empty_str))) {
				MYERROR::Log(
					::Error,
					// STRING: ALIEN 0x482a28
					"LOAD::Can't open file %s",
					fname.m_str
				);
			}
			else {
				STRING temp;
				FTempFile(
					&temp.m_str,
					// STRING: ALIEN 0x482a44
					"c:\\tmp",
					// STRING: ALIEN 0x482a4c
					"vid"
				);
				fname = temp;
				removeTemp = 1;
				converter->MakeVid(0, STRING(fname.m_str));
			}
			delete converter;
		}
		{
			STRING dir;
			STRING resName(p_res->m_name);
			_strlwr(resName.m_str);
			if (strstr(
					resName,
					// STRING: ALIEN 0x482a20
					".map"
				) &&
				strstr(
					resName,
					// STRING: ALIEN 0x482a1c
					":\\"
				)) {
				STRING sub;
				fname.BeforeLast((char**) &sub, "\\");
				_strlwr(sub.m_str);
				STRING tmp;
				resName.BeforeLast((char**) &tmp, "\\");
				resName = tmp;
				resName.BeforeLast((char**) &dir, sub);
				resName = dir;
			}
			else {
				resName = empty_str;
			}
			STRING path = resName + fname;
			if (vidFile.OpenForRead(path, 0x20444956 /* 'VID ' */)) {
				MYERROR::Log(::Error, "LOAD::Can't open file %s", fname.m_str);
				// Remove the converted temporary before the early return.
				if (removeTemp) {
					Platform_Remove(fname.m_str);
				}
				return vid;
			}
			if (vidFile.GoBegin(0x44414548 /* 'HEAD' */)) {
				MYERROR::Log(
					::Error,
					// STRING: ALIEN 0x4829f4
					"!!!ERROR!!!VID '%s': Load() not HEAD ",
					fname.m_str
				);
			}
			vidFile.Read(&scratch.m_pixelFlag16, 2);
		}
		// Unused hardware-only VID_MESH assets fall back to ordinary sprites.
		if (scratch.m_pixelFlag16 & 0x80) {
			vid = new VID_LIGHT;
		}
		else if (scratch.m_pixelFlag16 & 0x20) {
			if ((scratch.m_pixelFlag16 & 2) && (scratch.m_pixelFlag16 & 4)) {
				vid = new VID_HARDWARE_Z;
			}
			else {
				vid = new VID_HARDWARE;
			}
		}
		else if (scratch.m_sprClass == 8) {
			vid = new VID_SOFTWARE16;
		}
		else if (((GRAPH_CORE*) Graph)->m_flags & 2) {
			vid = new VID_SOFTWARE;
		}
		else {
			vid = new VID_SOFTWARE16;
		}
		vid->m_pixelFlag16 = scratch.m_pixelFlag16;
	}

	vid->m_idx = p_idx;
	vid->SetName(name.m_str);
	vid->SetFileName(fname.m_str);
	vidFile.Read(&vid->m_defaultAniPeriod, 2);
	vidFile.Read(&vid->m_dotFrameCount, 2);
	vidFile.Read(&vid->m_unk0x2f6, 2);
	vidFile.Read(&vid->m_messageLineHeight, 2);
	vid->LoadParameters(p_res);
	vid->Load(&vidFile);
	vidFile.Close();
	if (removeTemp) {
		Platform_Remove(fname.m_str);
	}
	vid->SetLayer();
	return vid;
}
