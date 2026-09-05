#include "video/vid_mesh.h"

#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/sprite.h"
#include "util/resource.h"
#include "video/vid_exdata.h"

#include <cmath>
#include <limits>
#include <vector>

struct VID_MESH::FRAME_SET {
	struct FRAME {
		std::unique_ptr<TEXTURE> m_texture;
		short m_left;
		short m_top;
		short m_right;
		short m_bottom;
	};
	std::vector<FRAME> m_frames;
};

VID_MESH::VID_MESH() : m_reportedUnsupported(false)
{
}

VID_MESH::VID_MESH(VID_MESH& p_other) : m_frames(p_other.m_frames), m_reportedUnsupported(false)
{
	m_weaponPtr = p_other.m_weaponPtr;
	p_other.m_weaponPtr = this;
	m_layer = p_other.m_layer;
	m_pixelFlag16 = p_other.m_pixelFlag16;
	m_defaultAniPeriod = p_other.m_defaultAniPeriod;
	m_dotFrameCount = p_other.m_dotFrameCount;
	m_unk0x2f6 = p_other.m_unk0x2f6;
	m_messageLineHeight = p_other.m_messageLineHeight;
}

VID_MESH::~VID_MESH() = default;

VID* VID_MESH::CreateMirror()
{
	return new VID_MESH(*this);
}

void* VID_MESH::ScalarDeletingDestructor(unsigned int p_flags)
{
	VID_MESH* result = this;
	this->~VID_MESH();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

void VID_MESH::SetLayer()
{

	m_layer = 8;
}

void VID_MESH::Load(RESOURCE* p_res)
{





	unsigned int format = 0;
	if (p_res->ReadWords(&format, 4) || !p_res->RequireEnd()) {
		return;
	}
	if (m_pixelFlag16 != 0x1012 || format != D3DFMT_A4R4G4B4 || !(m_flag & 0x8000)) {
		p_res->Fail("unsupported legacy mesh pixel format or world-space mesh");
		return;
	}
	if (m_dotFrameCount <= 0 || p_res->GoBegin(0x41544144  )) {
		p_res->Fail("missing legacy mesh frames");
		return;
	}
	if (p_res->m_noSubRes != m_dotFrameCount) {
		p_res->Fail("legacy mesh frame count differs from HEAD");
		return;
	}
	std::shared_ptr<FRAME_SET> decoded = std::make_shared<FRAME_SET>();
	for (int frame = 0; frame < m_dotFrameCount; ++frame) {
		unsigned short width = 0;
		unsigned short height = 0;
		if (p_res->ReadWords(&width, 2, 2) || p_res->ReadWords(&height, 2, 2)) {
			return;
		}
		const size_t pixelBytes = size_t(width) * height * 2;
		if (!width || !height || width > g_textureMaxWidth || height > g_textureMaxHeight || p_res->Remaining() < 48 ||
			pixelBytes > size_t(p_res->Remaining() - 48)) {
			p_res->Fail("invalid or truncated legacy mesh texture dimensions");
			return;
		}
		FRAME_SET::FRAME output = {};
		output.m_texture.reset(new TEXTURE(width, height, D3DFMT_A4R4G4B4, 24));
		if (!output.m_texture->m_data) {
			p_res->Fail("cannot allocate legacy mesh texture");
			return;
		}
		if (p_res->ReadWords(output.m_texture->m_data, int(pixelBytes), 2)) {
			return;
		}
		unsigned int vertexCount = 0;
		unsigned int indexCount = 0;
		if (p_res->ReadWords(&vertexCount, 4) || p_res->ReadWords(&indexCount, 4)) {
			return;
		}
		if (vertexCount != 4 || indexCount != 0) {
			p_res->Fail("unsupported legacy mesh topology (requires four-vertex strip without indices)");
			return;
		}
		short vertices[4][5] = {};
		if (p_res->ReadWords(vertices, sizeof(vertices), 2) || !p_res->RequireEnd()) {
			return;
		}
		output.m_left = vertices[0][0];
		output.m_top = vertices[0][1];
		output.m_right = vertices[3][0];
		output.m_bottom = vertices[3][1];
		for (int vertex = 0; vertex < 4; ++vertex) {
			const int x = vertex & 1 ? output.m_right : output.m_left;
			const int y = vertex & 2 ? output.m_bottom : output.m_top;
			if (vertices[vertex][0] != x || vertices[vertex][1] != y || vertices[vertex][2] != 1024 ||
				vertices[vertex][3] != (vertex & 1 ? width : 0) || vertices[vertex][4] != (vertex & 2 ? height : 0) ||
				output.m_left >= output.m_right || output.m_top >= output.m_bottom) {
				p_res->Fail("unsupported legacy mesh geometry (requires flat rectangular full-texture strip)");
				return;
			}
		}
		decoded->m_frames.push_back(std::move(output));
		if (frame + 1 < m_dotFrameCount && p_res->GoNextSub(0x41544144)) {
			p_res->Fail("missing legacy mesh frame record");
			return;
		}
	}
	m_frames = std::move(decoded);
	SetLayer();
}

int VID_MESH::Draw(SPRITE* p_sprite)
{
	if (!m_frames || !Graph || !Map || !p_sprite || p_sprite->m_noCadr < 0 ||
		size_t(p_sprite->m_noCadr) >= m_frames->m_frames.size()) {
		return 0;
	}
	if (m_unk0x47c & 0x40) {
		return 0;
	}
	if (!(m_flag & 0x8000)) {
		if (!m_reportedUnsupported) {
			Error(5, "unsupported world-space legacy mesh", p_sprite->m_noCadr);
			m_reportedUnsupported = true;
		}
		return 0;
	}
	const FRAME_SET::FRAME& frame = m_frames->m_frames[p_sprite->m_noCadr];
	double scaleX = double(m_gammaR) * p_sprite->UIDrawScale();
	double scaleY = double(m_gammaG) * p_sprite->UIDrawScale();
	double x = double(p_sprite->m_x) - Map->m_shiftX;
	double y = double(p_sprite->m_y) - p_sprite->m_z - Map->m_shiftY;
	if (m_unk0x47c & 6) {
		if (!m_exData || !p_sprite->m_exData) {
			return 0;
		}
		float time = p_sprite->m_exData->m_unk0x1c;
		if (!std::isfinite(time) || time < 0.0f) {
			return 0;
		}
		auto interpolate = [time](const float* p_values) {
			if (time >= 7.0f) {
				return p_values[7];
			}
			int index = int(time);
			return (p_values[index + 1] - p_values[index]) * (time - index) + p_values[index];
		};
		if (m_unk0x47c & 2) {
			scaleX *= interpolate(m_exData->m_unk0xe4);
			scaleY *= interpolate(m_exData->m_unk0x104);
		}
		if (m_unk0x47c & 4) {
			x += interpolate(m_exData->m_unk0x144);
			y += interpolate(m_exData->m_unk0x164) - interpolate(m_exData->m_unk0x184);
		}
	}


	const double bounds[] =
		{x + frame.m_left * scaleX, y + frame.m_top * scaleY, x + frame.m_right * scaleX, y + frame.m_bottom * scaleY};
	for (double value : bounds) {
		if (!std::isfinite(value) || value < std::numeric_limits<int>::min() / 4 ||
			value > std::numeric_limits<int>::max() / 4) {
			return 0;
		}
	}
	const int dst[] = {int(bounds[0]), int(bounds[1]), int(bounds[2]), int(bounds[3])};
	const int src[] = {0, 0, frame.m_texture->m_width, frame.m_texture->m_height};
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;


	graph->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
	graph->SetAlphaBlend(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
	GAMMA color;
	color.Add(GAMMA(GAMMA::RAW_COPY, m_colorSub, m_colorAdd), GAMMA(GAMMA::RAW_COPY, p_sprite->GetGamma()));
	GAMMA withScreen;
	const GAMMA* drawGamma = &color;
	if (!(m_flag & 0x800)) {
		withScreen.Add(color, GAMMA(GAMMA::RAW_COPY, graph->m_gammaSet.m_a, graph->m_gammaSet.m_b));
		drawGamma = &withScreen;
	}
	frame.m_texture->Draw_z(0.99999988f, 0, dst, src, drawGamma);
	return 0;
}
