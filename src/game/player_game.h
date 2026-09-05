#ifndef PLAYER_GAME_H
#define PLAYER_GAME_H

#include "game/message.h"
#include "game/player.h"

class ENGINE;
class DEPO;




class PLAYER_GAME final : public PLAYER {
public:
	PLAYER_GAME(int p_control, int p_army);
	~PLAYER_GAME() override;
	void DeletePointerToSprite(SPRITE* p_sprite) override;
	void Release() override;
	void SetFlagman(SPRITE* p_sprite) override;
	void Control(INPUT_AS* p_input) override;
	void StateBarOn() override;
	void StateBarOff() override;
	void PutMessage(const STRING& p_msg, float p_x, float p_y) override;
	void AddPointerToSprite(SPRITE* p_sprite) override;
	void RefreshUILayout() override;
	unsigned int SetCleverAttack(int p_on) override;

private:
	unsigned int m_controlFlags = 2;
	int m_hudMode = 6;
	int m_queueIndex = -1;
	SPRITE_LIST m_selection;
	MESSAGE m_message;
	PTR_SPRITE m_slots[10];
	bool m_releasing = false;

	SPRITE* Hud(int p_index) const;
	SPRITE* AddHud(int p_vid, float p_x, float p_y, float p_z);
	void SetHudMode(int p_mode);
	void UpdateHud();
	void Keyboard(INPUT_AS* p_input);
	void Toolbar(int p_index, unsigned int p_key);
	void ContextOrders(INPUT_AS* p_input, int p_cursorBase);
	void AssignSlot(SPRITE* p_sprite, unsigned int p_slot);
	bool CanSelect(SPRITE* p_sprite) const;
	bool CanBuild(VID* p_vid) const;
	bool SetIcon(int p_index, VID* p_vid, SPRITE* p_sprite = nullptr);
};

#endif
