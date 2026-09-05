#ifndef SPRITE_IDS_H
#define SPRITE_IDS_H

#include "util/stream.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>




class SPRITE_SAVE_IDS {
public:
	void Clear() { m_ids.clear(); }

	uint32_t Encode(const void* p_sprite)
	{
		if (!p_sprite) return 0;
		if (reinterpret_cast<uintptr_t>(p_sprite) == std::numeric_limits<uintptr_t>::max()) return UINT32_MAX;
		auto found = m_ids.find(p_sprite);
		if (found != m_ids.end()) return found->second;
		if (m_ids.size() >= (size_t) std::numeric_limits<int32_t>::max()) {
			throw std::length_error("too many sprite identities in one legacy save");
		}
		uint32_t id = (uint32_t) m_ids.size() + 1;
		m_ids.emplace(p_sprite, id);
		return id;
	}

	int Write(STREAM* p_stream, const void* p_sprite)
	{
		uint32_t id = Encode(p_sprite);
		unsigned char bytes[4] = {
			(unsigned char) id, (unsigned char) (id >> 8), (unsigned char) (id >> 16), (unsigned char) (id >> 24)
		};
		return p_stream->Write(bytes, sizeof(bytes));
	}

private:
	std::unordered_map<const void*, uint32_t> m_ids;
};

#endif
