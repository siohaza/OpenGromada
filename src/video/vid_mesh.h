#ifndef VID_MESH_H
#define VID_MESH_H

#include "video/vid.h"

#include <memory>



class VID_MESH : public VID {
public:
	VID_MESH();
	VID_MESH(VID_MESH& p_other);
	~VID_MESH();
	VID* CreateMirror() override;
	void* ScalarDeletingDestructor(unsigned int p_flags) override;
	void Load(RESOURCE* p_res) override;
	int Draw(SPRITE* p_sprite) override;
	void SetLayer() override;

private:
	struct FRAME_SET;
	std::shared_ptr<FRAME_SET> m_frames;
	bool m_reportedUnsupported;
};

#endif
