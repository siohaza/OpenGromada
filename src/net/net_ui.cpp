#include "net/net_ui.h"

#include "game/filedata.h"
#include "game/game_descriptor.h"
#include "game/input_as.h"
#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "net/net_client.h"
#include "net/net_protocol.h"
#include "platform/keycodes.h"
#include "platform/portable_config.h"
#include "platform/render.h"
#include "platform/timing.h"
#include "sprite/sprite.h"
#include "sprite/stext.h"
#include "ui/mouse.h"
#include "ui/ui_scaling.h"
#include "util/myerror.h"
#include "util/string.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum NETUI_ITEM {
	NETUI_ITEM_ADDRESS,
	NETUI_ITEM_NAME,
	NETUI_ITEM_PORT,
	NETUI_ITEM_CONNECT,
	NETUI_ITEM_HOST,
	NETUI_ITEM_DISCONNECT,
	NETUI_ITEM_COUNT
};

enum {
	NETUI_GLYPH = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE,
	NETUI_PAD = 8,
	NETUI_ENTRY_X = 8,
	NETUI_ENTRY_Y = 254,
	NETUI_ENTRY_W = 176,
	NETUI_ENTRY_H = 26,
	NETUI_MENU_BUTTON_VID = 705,
	NETUI_MENU_FONT_VID = 3,
	NETUI_SPRCLASS_FRAME = 10,
	NETUI_SPRCLASS_TEXT = 19,
	NETUI_MENU_ROW_UNITS = 30,
	NETUI_ENTRY_HALF_W = 88,
	NETUI_ENTRY_ABOVE = 14,
	NETUI_ENTRY_BELOW = 12,
	NETUI_ACT_GET_BEHAVE = 94,
	NETUI_ACT_SET_BEHAVE = 95,
	NETUI_ACT_SET_TEXT = 120,
	NETUI_TEXT_SOURCE_MASK = 0x70,
	NETUI_LINE = 12,
	NETUI_ROW_MARGIN = 2,
	NETUI_PANEL_W = 468,
	NETUI_ROW_TITLE = NETUI_PAD,
	NETUI_ROW_FIELDS = 28,
	NETUI_ROW_BUTTONS = 70,
	NETUI_ROW_STATUS = 88,
	NETUI_ROW_ROSTER_TITLE = 106,
	NETUI_ROW_ROSTER = 118,
	NETUI_HELP_GAP = 6,
	NETUI_INNER_CHARS = (NETUI_PANEL_W - 2 * NETUI_PAD) / NETUI_GLYPH,
	NETUI_LABEL_CHARS = 9,
	NETUI_BUTTON_GAP_CHARS = 3,
	NETUI_ELLIPSIS_CHARS = 3,
	NETUI_MAP_LEAF_CHARS = 22,
	NETUI_MAP_SUFFIX_CHARS = 4,
	NETUI_PORT_LEN = 6,
	NETUI_NOTICE_LEN = 64,
	NETUI_STATUS_LEN = 160,
	NETUI_LINE_LEN = 128,
	NETUI_ASCII_FIRST = 0x20,
	NETUI_ASCII_LAST = 0x7e,
	NETUI_TOAST_MS = 5000,
	NETUI_TOAST_TOP = 14,
	NETUI_OVERHEAD_LIFT = 80,
	NETUI_OVERHEAD_HALF_WIDTH = 30,
	NETUI_OVERHEAD_TROUGH = 3,
	NETUI_OVERHEAD_NAME_RISE = 14,
	NETUI_POINTER_ARM = 4,
	NETUI_MOUSE_BUTTON_BITS = 32,
	NETUI_EFFECT_PIXEL_APPEAR = 6,
	NETUI_EFFECT_LINE_SHIFT = 7
};

constexpr unsigned int NETUI_GRAPH_FRAME_OPEN = 0x10000u;
constexpr unsigned int NETUI_GRAPH_ALPHA_BLEND = 0x4000u;
constexpr unsigned int NETUI_MAP_MENU_MODE = 0x10u;

constexpr unsigned int NETUI_ARGB_BACKGROUND = 0xff101010u;
constexpr unsigned int NETUI_ARGB_BORDER = 0xffc8c800u;
constexpr unsigned int NETUI_ARGB_FOCUS_BAR = 0xff38381cu;
constexpr unsigned int NETUI_ARGB_POINTER = 0xffffffffu;
constexpr const char* NETUI_DEFAULT_PANEL_KEY = "F2";
constexpr const char* NETUI_ENTRY_LABEL = "MULTIPLAYER";
constexpr unsigned int NETUI_ARGB_ENTRY_BORDER = 0xff820000u;
constexpr unsigned int NETUI_ARGB_ENTRY_FILL = 0xff1c0000u;
constexpr unsigned int NETUI_ARGB_ENTRY_DIM = 0x5e1c0000u;
constexpr unsigned int NETUI_ARGB_ENTRY_FILL_HOVER = 0xff400808u;
constexpr unsigned int NETUI_ARGB_ENTRY_TEXT = 0xffad4842u;
constexpr unsigned int NETUI_ARGB_ENTRY_TEXT_HOVER = 0xffff7060u;
constexpr unsigned int NETUI_ARGB_TITLE = 0xffffff00u;
constexpr unsigned int NETUI_ARGB_FOCUSED = 0xffffff00u;
constexpr unsigned int NETUI_ARGB_ENABLED = 0xffffffffu;
constexpr unsigned int NETUI_ARGB_DISABLED = 0xff808080u;
constexpr unsigned int NETUI_ARGB_STATUS = 0xff80ff80u;
constexpr unsigned int NETUI_ARGB_NOTICE = 0xffff8080u;
constexpr unsigned int NETUI_ARGB_HELP = 0xff00c000u;
constexpr unsigned int NETUI_ARGB_HEALTH_FRAME = 0xff3c3c34u;
constexpr unsigned int NETUI_ARGB_HEALTH_TROUGH = 0xff140000u;
constexpr unsigned int NETUI_ARGB_HEALTH_FILL = 0xffc82808u;

static const char* const NETUI_PATH_ADDRESS = "save://common/NetServerAddress";
static const char* const NETUI_PATH_NAME = "save://common/NetPlayerName";
static const char* const NETUI_PATH_PORT = "save://common/NetHostPort";
static const char* const NETUI_PATH_PROFILE_NAME = "save://common/PlayerName";
static const char* const NETUI_DEFAULT_ADDRESS = "127.0.0.1";
static const char* const NETUI_DEFAULT_NAME = "Player";
static const char* const NETUI_HELP_TEXT = "Tab next | Enter select | F3 disconnect | Esc close";

struct NETUI_RECT {
	float m_x0;
	float m_y0;
	float m_x1;
	float m_y1;
};

struct NETUI_LAYOUT {
	int m_k;
	float m_x;
	float m_y;
	float m_w;
	float m_h;
};

struct NETUI_TEXT {
	size_t m_length;
	char m_text[NETUI_LINE_LEN];
};

struct NETUI_ROSTER_ROW {
	unsigned int m_argb;
	NETUI_TEXT m_line;
};

struct NETUI_ROSTER {
	int m_count;
	NETUI_ROSTER_ROW m_rows[NET_MAX_PLAYERS];
};

struct NETUI_OVERHEAD {
	float m_sx;
	float m_sy;
	int m_hp;
	int m_maxHp;
	char m_name[NET_NAME_LEN];
};

struct NETUI_OVERHEAD_FRAME {
	int m_count;
	float m_frameW;
	float m_frameH;
	NETUI_OVERHEAD m_items[NET_MAX_PLAYERS];
};

struct NETUI_PANEL {
	int m_open;
	int m_loaded;
	int m_focus;
	int m_autoClose;
	int m_toastActive;
	unsigned int m_toastTime;
	unsigned int m_mouseSwallowed;
	int m_mainMenuShown;
	char m_address[NET_ADDRESS_LEN];
	char m_name[NET_NAME_LEN];
	char m_port[NETUI_PORT_LEN];
	char m_notice[NETUI_NOTICE_LEN];
	char m_toast[NETUI_STATUS_LEN];
};

namespace
{
NETUI_PANEL g_ui;
}

namespace
{
NETUI_OVERHEAD_FRAME g_frame;
}

#ifdef OPENGROMADA_NET_TESTHOOKS

static void TraceOpen()
{
	MYERROR::Log(::Error, "NETUI: open");
}

static void TraceClosed()
{
	MYERROR::Log(::Error, "NETUI: closed");
	MYERROR::Log(::Error, "NETUI: held %x", Map ? (unsigned int) Map->m_input.m_button : 0u);
}

static void TraceConnect(const char* p_address)
{
	MYERROR::Log(::Error, "NETUI: connect %s", p_address);
}

static void TraceHost(int p_port)
{
	MYERROR::Log(::Error, "NETUI: host %d", p_port);
}

static void TraceDisconnect()
{
	MYERROR::Log(::Error, "NETUI: disconnect");
}

#else

static void TraceOpen()
{
}

static void TraceClosed()
{
}

static void TraceConnect(const char*)
{
}

static void TraceHost(int)
{
}

static void TraceDisconnect()
{
}

#endif

static int IsPrintableAscii(unsigned char p_c)
{
	return p_c >= NETUI_ASCII_FIRST && p_c <= NETUI_ASCII_LAST;
}

static void TextReset(NETUI_TEXT* p_text)
{
	p_text->m_length = 0;
	p_text->m_text[0] = 0;
}

static void TextAppend(NETUI_TEXT* p_text, const char* p_source)
{
	for (const char* c = p_source ? p_source : ""; *c && p_text->m_length + 1 < sizeof(p_text->m_text); ++c) {
		p_text->m_text[p_text->m_length++] = IsPrintableAscii((unsigned char) *c) ? *c : '?';
	}
	p_text->m_text[p_text->m_length] = 0;
}

static void TextAppendInt(NETUI_TEXT* p_text, int p_value)
{
	char digits[16];
	snprintf(digits, sizeof(digits), "%d", p_value);
	TextAppend(p_text, digits);
}

static void TextPadTo(NETUI_TEXT* p_text, size_t p_column)
{
	while (p_text->m_length < p_column && p_text->m_length + 1 < sizeof(p_text->m_text)) {
		p_text->m_text[p_text->m_length++] = ' ';
	}
	p_text->m_text[p_text->m_length] = 0;
}

static void TextAppendTail(NETUI_TEXT* p_text, const char* p_source, size_t p_maxChars)
{
	const size_t length = strlen(p_source);
	if (length <= p_maxChars) {
		TextAppend(p_text, p_source);
		return;
	}
	if (p_maxChars <= NETUI_ELLIPSIS_CHARS) {
		TextAppend(p_text, p_source + length - p_maxChars);
		return;
	}
	TextAppend(p_text, "...");
	TextAppend(p_text, p_source + length - (p_maxChars - NETUI_ELLIPSIS_CHARS));
}

static void TextEllipsize(NETUI_TEXT* p_text, size_t p_maxChars)
{
	if (p_text->m_length <= p_maxChars) {
		return;
	}
	p_text->m_length = p_maxChars;
	p_text->m_text[p_maxChars] = 0;
	for (size_t i = 0; i < NETUI_ELLIPSIS_CHARS && i < p_maxChars; ++i) {
		p_text->m_text[p_maxChars - 1 - i] = '.';
	}
}

static const char* FirstNonEmpty(const char* p_preferred, const char* p_fallback)
{
	return p_preferred && p_preferred[0] ? p_preferred : p_fallback;
}

static int IsField(int p_item)
{
	return p_item >= NETUI_ITEM_ADDRESS && p_item <= NETUI_ITEM_PORT;
}

static char* FieldText(int p_item)
{
	switch (p_item) {
	case NETUI_ITEM_ADDRESS:
		return g_ui.m_address;
	case NETUI_ITEM_NAME:
		return g_ui.m_name;
	default:
		return g_ui.m_port;
	}
}

static size_t FieldCapacity(int p_item)
{
	switch (p_item) {
	case NETUI_ITEM_ADDRESS:
		return sizeof(g_ui.m_address);
	case NETUI_ITEM_NAME:
		return sizeof(g_ui.m_name);
	default:
		return sizeof(g_ui.m_port);
	}
}

static int FieldAccepts(int p_item, unsigned char p_c, size_t p_length)
{
	const int letter = (p_c >= 'a' && p_c <= 'z') || (p_c >= 'A' && p_c <= 'Z');
	const int digit = p_c >= '0' && p_c <= '9';
	switch (p_item) {
	case NETUI_ITEM_ADDRESS:
		return letter || digit || p_c == '.' || p_c == '-' || p_c == ':' || p_c == '[' || p_c == ']' || p_c == '%' ||
			   p_c == '_';
	case NETUI_ITEM_NAME:
		return IsPrintableAscii(p_c) && (p_c != ' ' || p_length > 0);
	default:
		return digit;
	}
}

static void AppendFiltered(int p_item, const char* p_source)
{
	char* field = FieldText(p_item);
	const size_t capacity = FieldCapacity(p_item);
	size_t length = strlen(field);
	for (const char* c = p_source ? p_source : ""; *c; ++c) {
		if (length + 1 < capacity && FieldAccepts(p_item, (unsigned char) *c, length)) {
			field[length++] = *c;
		}
	}
	field[length] = 0;
}

static int ItemEnabled(int p_item)
{
	const int idle = Net_UiState() == NET_UI_IDLE;
	return p_item == NETUI_ITEM_DISCONNECT ? !idle : idle;
}

static void NormalizeFocus()
{
	for (int step = 0; step < NETUI_ITEM_COUNT && !ItemEnabled(g_ui.m_focus); ++step) {
		g_ui.m_focus = (g_ui.m_focus + 1) % NETUI_ITEM_COUNT;
	}
}

static void MoveFocus(int p_direction)
{
	int item = g_ui.m_focus;
	for (int step = 0; step < NETUI_ITEM_COUNT; ++step) {
		item = (item + p_direction + NETUI_ITEM_COUNT) % NETUI_ITEM_COUNT;
		if (ItemEnabled(item)) {
			g_ui.m_focus = item;
			return;
		}
	}
}

static int FocusedFieldEditable()
{
	return IsField(g_ui.m_focus) && ItemEnabled(g_ui.m_focus);
}

static void EraseLast()
{
	if (!FocusedFieldEditable()) {
		return;
	}
	char* field = FieldText(g_ui.m_focus);
	const size_t length = strlen(field);
	if (length) {
		field[length - 1] = 0;
	}
}

static void ClearFocusedField()
{
	if (FocusedFieldEditable()) {
		FieldText(g_ui.m_focus)[0] = 0;
	}
}

static void PasteClipboard()
{
	if (!FocusedFieldEditable()) {
		return;
	}
	char* clipboard = SDL_GetClipboardText();
	if (clipboard) {
		AppendFiltered(g_ui.m_focus, clipboard);
		SDL_free(clipboard);
	}
}

static void SetNotice(const char* p_notice)
{
	snprintf(g_ui.m_notice, sizeof(g_ui.m_notice), "%s", p_notice);
}

static void LoadFieldsOnce()
{
	if (g_ui.m_loaded) {
		return;
	}
	g_ui.m_loaded = 1;
	AppendFiltered(NETUI_ITEM_ADDRESS, FileData_Load(NETUI_PATH_ADDRESS, NETUI_DEFAULT_ADDRESS));
	if (!g_ui.m_address[0]) {
		AppendFiltered(NETUI_ITEM_ADDRESS, NETUI_DEFAULT_ADDRESS);
	}
	AppendFiltered(NETUI_ITEM_NAME, FileData_Load(NETUI_PATH_NAME, ""));
	if (!g_ui.m_name[0]) {
		AppendFiltered(NETUI_ITEM_NAME, FileData_Load(NETUI_PATH_PROFILE_NAME, NETUI_DEFAULT_NAME));
	}
	if (!g_ui.m_name[0]) {
		AppendFiltered(NETUI_ITEM_NAME, NETUI_DEFAULT_NAME);
	}
	AppendFiltered(NETUI_ITEM_PORT, FileData_Load(NETUI_PATH_PORT, ""));
	int port = 0;
	if (!NetParsePort(g_ui.m_port, &port)) {
		snprintf(g_ui.m_port, sizeof(g_ui.m_port), "%d", (int) NET_DEFAULT_PORT);
	}
}

static void PersistField(const char* p_path, const char* p_value)
{
	if (strcmp(FileData_Load(p_path, ""), p_value) != 0) {
		FileData_Save(p_path, p_value);
	}
}

static void FinishName()
{
	size_t length = strlen(g_ui.m_name);
	while (length && g_ui.m_name[length - 1] == ' ') {
		g_ui.m_name[--length] = 0;
	}
	if (!length) {
		AppendFiltered(NETUI_ITEM_NAME, NETUI_DEFAULT_NAME);
	}
}

static void OpenPanel()
{
	LoadFieldsOnce();
	g_ui.m_open = 1;
	g_ui.m_notice[0] = 0;
	NormalizeFocus();
	TraceOpen();
}

static void ClosePanel()
{
	if (!g_ui.m_open) {
		return;
	}
	g_ui.m_open = 0;
	g_ui.m_autoClose = 0;
	TraceClosed();
}

static void ActivateConnect()
{
	if (Net_UiState() != NET_UI_IDLE) {
		return;
	}
	char host[NET_ADDRESS_LEN];
	int port = 0;
	if (!NetParseAddress(g_ui.m_address, host, sizeof(host), &port)) {
		SetNotice("invalid address");
		return;
	}
	FinishName();
	PersistField(NETUI_PATH_ADDRESS, g_ui.m_address);
	PersistField(NETUI_PATH_NAME, g_ui.m_name);
	TraceConnect(g_ui.m_address);
	g_ui.m_autoClose = Net_Connect(g_ui.m_address, g_ui.m_name) != 0;
}

static void ActivateHost()
{
	if (Net_UiState() != NET_UI_IDLE) {
		return;
	}
	int port = 0;
	if (!NetParsePort(g_ui.m_port, &port)) {
		SetNotice("invalid port");
		return;
	}
	FinishName();
	PersistField(NETUI_PATH_NAME, g_ui.m_name);
	PersistField(NETUI_PATH_PORT, g_ui.m_port);
	TraceHost(port);
	g_ui.m_autoClose = Net_Host(port, g_ui.m_name) != 0;
}

static void ActivateDisconnect()
{
	if (Net_UiState() == NET_UI_IDLE) {
		return;
	}
	g_ui.m_autoClose = 0;
	TraceDisconnect();
	Net_Disconnect();
}

static void ActivateItem(int p_item)
{
	switch (p_item) {
	case NETUI_ITEM_ADDRESS:
	case NETUI_ITEM_NAME:
	case NETUI_ITEM_CONNECT:
		ActivateConnect();
		break;
	case NETUI_ITEM_PORT:
	case NETUI_ITEM_HOST:
		ActivateHost();
		break;
	case NETUI_ITEM_DISCONNECT:
		ActivateDisconnect();
		break;
	default:
		break;
	}
}

static const char* ButtonLabel(int p_item)
{
	switch (p_item) {
	case NETUI_ITEM_CONNECT:
		return "[ Connect ]";
	case NETUI_ITEM_HOST:
		return "[ Host ]";
	default:
		if (Net_UiState() == NET_UI_BUSY) {
			return "[ Cancel ]";
		}
		return Net_Hosting() ? "[ Stop hosting ]" : "[ Disconnect ]";
	}
}

static int ButtonColumn(int p_item)
{
	int column = 0;
	for (int item = NETUI_ITEM_CONNECT; item < p_item; ++item) {
		column += (int) strlen(ButtonLabel(item)) + NETUI_BUTTON_GAP_CHARS;
	}
	return column;
}

static int FieldRow(int p_item)
{
	return NETUI_ROW_FIELDS + NETUI_LINE * (p_item - NETUI_ITEM_ADDRESS);
}

static int PanelHeight(int p_rosterRows)
{
	return NETUI_ROW_ROSTER + p_rosterRows * NETUI_LINE + NETUI_HELP_GAP + NETUI_GLYPH + NETUI_PAD;
}

static int UiScale(const GRAPH_CORE* p_core)
{
	const float scale = UI_SCALING::NormalizeDrawScale((float) p_core->m_uiScale * p_core->m_uiPresentationScale);
	return UI_SCALING::NormalizeScale((int) (scale + 0.5f));
}

static float SnapToScale(float p_value, int p_k)
{
	const float scale = (float) p_k;
	return floorf(p_value / scale) * scale;
}

static float SnapInsideView(float p_value, float p_viewMin, int p_k)
{
	const float snapped = SnapToScale(p_value, p_k);
	return snapped < p_viewMin ? snapped + (float) p_k : snapped;
}

static int ComputeLayout(const GRAPH_CORE* p_core, int p_rosterRows, NETUI_LAYOUT* p_out)
{
	const float viewW = p_core->m_viewXMax - p_core->m_viewXMin;
	const float viewH = p_core->m_viewYMax - p_core->m_viewYMin;
	if (!(viewW > 0.0f) || !(viewH > 0.0f)) {
		return 0;
	}
	const float room = (float) (2 * NETUI_PAD);
	const int fullHeight = PanelHeight(NET_MAX_PLAYERS);
	int k = UiScale(p_core);
	while (k > 1 && ((float) (NETUI_PANEL_W * k) > viewW - room || (float) (fullHeight * k) > viewH - room)) {
		--k;
	}
	const float width = (float) (NETUI_PANEL_W * k);
	float offsetX = (viewW - width) * 0.5f;
	float offsetY = (viewH - (float) (fullHeight * k)) / 3.0f;
	if (offsetX < 0.0f) {
		offsetX = 0.0f;
	}
	if (offsetY < 0.0f) {
		offsetY = 0.0f;
	}
	p_out->m_k = k;
	p_out->m_w = width;
	p_out->m_h = (float) (PanelHeight(p_rosterRows) * k);
	p_out->m_x = SnapInsideView(p_core->m_viewXMin + offsetX, p_core->m_viewXMin, k);
	p_out->m_y = SnapInsideView(p_core->m_viewYMin + offsetY, p_core->m_viewYMin, k);
	return 1;
}

static NETUI_RECT PanelRect(const NETUI_LAYOUT& p_layout)
{
	const NETUI_RECT rect = {p_layout.m_x, p_layout.m_y, p_layout.m_x + p_layout.m_w, p_layout.m_y + p_layout.m_h};
	return rect;
}

static NETUI_RECT ItemRect(const NETUI_LAYOUT& p_layout, int p_item)
{
	int row = NETUI_ROW_BUTTONS;
	int left = NETUI_PAD;
	int right = NETUI_PANEL_W - NETUI_PAD;
	if (IsField(p_item)) {
		row = FieldRow(p_item);
	}
	else {
		left = NETUI_PAD + ButtonColumn(p_item) * NETUI_GLYPH;
		right = left + (int) strlen(ButtonLabel(p_item)) * NETUI_GLYPH;
	}
	const float k = (float) p_layout.m_k;
	const NETUI_RECT rect = {
		p_layout.m_x + (float) (left - NETUI_ROW_MARGIN) * k,
		p_layout.m_y + (float) (row - NETUI_ROW_MARGIN) * k,
		p_layout.m_x + (float) (right + NETUI_ROW_MARGIN) * k,
		p_layout.m_y + (float) (row + NETUI_GLYPH + NETUI_ROW_MARGIN) * k
	};
	return rect;
}

static int RectContains(const NETUI_RECT& p_rect, float p_x, float p_y)
{
	return p_x >= p_rect.m_x0 && p_x < p_rect.m_x1 && p_y >= p_rect.m_y0 && p_y < p_rect.m_y1;
}

static int RectsIntersect(const NETUI_RECT& p_a, const NETUI_RECT& p_b)
{
	return p_a.m_x0 < p_b.m_x1 && p_b.m_x0 < p_a.m_x1 && p_a.m_y0 < p_b.m_y1 && p_b.m_y0 < p_a.m_y1;
}

static NETUI_RECT ViewRect(const GRAPH_CORE* p_core)
{
	const NETUI_RECT rect = {p_core->m_viewXMin, p_core->m_viewYMin, p_core->m_viewXMax, p_core->m_viewYMax};
	return rect;
}

static int CountRoster()
{
	int count = 0;
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		NET_ROSTER_INFO info = {};
		if (Net_RosterInfo(i, &info)) {
			++count;
		}
	}
	return count;
}

static int OpenPanelLayout(NETUI_LAYOUT* p_out)
{
	const GRAPH_CORE* core = Graph;
	return g_ui.m_open && core && ComputeLayout(core, CountRoster(), p_out);
}

static int PointInOpenPanel(float p_x, float p_y)
{
	NETUI_LAYOUT layout;
	return OpenPanelLayout(&layout) && RectContains(PanelRect(layout), p_x, p_y);
}

static unsigned int MouseButtonBit(unsigned int p_button)
{
	return p_button < NETUI_MOUSE_BUTTON_BITS ? 1u << p_button : 0u;
}

static void ClickItemAt(const NETUI_LAYOUT& p_layout, float p_x, float p_y)
{
	for (int item = 0; item < NETUI_ITEM_COUNT; ++item) {
		if (!ItemEnabled(item) || !RectContains(ItemRect(p_layout, item), p_x, p_y)) {
			continue;
		}
		g_ui.m_focus = item;
		if (!IsField(item)) {
			ActivateItem(item);
		}
		return;
	}
}

static int PanelKey()
{
	static int key;
	if (!key) {
		const char* configured = PortableConfig_GetString("net", "PanelKey");
		key = INPUT_AS::GetKeyByName(STRING(configured && *configured ? configured : NETUI_DEFAULT_PANEL_KEY));
		if (key <= 0) {
			key = INPUT_AS::GetKeyByName(STRING(NETUI_DEFAULT_PANEL_KEY));
		}
	}
	return key;
}

static int IsPanelKey(const SDL_KeyboardEvent& p_key)
{
	return Platform_KeyToVirtualKey((unsigned int) p_key.key) == PanelKey();
}

static int OnKeyDown(const SDL_KeyboardEvent& p_key)
{
	const int repeat = p_key.repeat ? 1 : 0;
	if (!g_ui.m_open) {
		if (!IsPanelKey(p_key)) {
			return 0;
		}
		if (!repeat) {
			OpenPanel();
		}
		return 1;
	}
	if (!repeat) {
		g_ui.m_notice[0] = 0;
	}
	NormalizeFocus();
	if (IsPanelKey(p_key)) {
		if (!repeat) {
			ClosePanel();
		}
		return 1;
	}
	switch (p_key.key) {
	case SDLK_ESCAPE:
		if (!repeat) {
			ClosePanel();
		}
		break;
	case SDLK_TAB:
		MoveFocus((p_key.mod & SDL_KMOD_SHIFT) ? -1 : 1);
		break;
	case SDLK_DOWN:
		MoveFocus(1);
		break;
	case SDLK_UP:
		MoveFocus(-1);
		break;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		if (!repeat) {
			ActivateItem(g_ui.m_focus);
		}
		break;
	case SDLK_F3:
		if (!repeat) {
			ActivateDisconnect();
		}
		break;
	case SDLK_BACKSPACE:
		EraseLast();
		break;
	case SDLK_DELETE:
		if (!repeat) {
			ClearFocusedField();
		}
		break;
	case SDLK_V:
		if (!repeat && (p_key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI))) {
			PasteClipboard();
		}
		break;
	case SDLK_INSERT:
		if (!repeat && (p_key.mod & SDL_KMOD_SHIFT)) {
			PasteClipboard();
		}
		break;
	default:
		break;
	}
	return 1;
}

static int OnTextInput(const char* p_text)
{
	if (!g_ui.m_open) {
		return 0;
	}
	NormalizeFocus();
	if (FocusedFieldEditable()) {
		AppendFiltered(g_ui.m_focus, p_text);
	}
	return 1;
}

struct NETUI_MENU_STACK {
	SPRITE* m_topButton;
	SPRITE* m_topLabel;
	SPRITE* m_ownLabel;
	float m_spacing;
};

static int MenuEntryShown()
{
	return g_ui.m_mainMenuShown && Map && Map->m_menu.m_n > 0 && !Map->m_gameplayMap;
}

static float MenuScreenY(const SPRITE* p_sprite)
{
	return p_sprite->m_y - p_sprite->m_z - Map->m_shiftY;
}

static int FindMenuStack(NETUI_MENU_STACK* p_out)
{
	memset(p_out, 0, sizeof(*p_out));
	float topY = 0.0f;
	float nextY = 0.0f;
	int buttons = 0;
	for (int i = 0; Map && i < Map->m_menu.m_n; ++i) {
		SPRITE* item = (SPRITE*) Map->m_menu.m_data[i];
		if (!item || !item->m_vid) {
			continue;
		}
		const int sprClass = (int) item->m_vid->m_sprClass;
		const int vid = item->m_vid->m_idx;
		const float y = MenuScreenY(item);
		if (sprClass == NETUI_SPRCLASS_FRAME && vid == NETUI_MENU_BUTTON_VID) {
			if (!buttons || y < topY) {
				nextY = topY;
				topY = y;
				p_out->m_topButton = item;
			}
			else if (buttons == 1 || y < nextY) {
				nextY = y;
			}
			++buttons;
		}
		else if (sprClass == NETUI_SPRCLASS_TEXT && vid == NETUI_MENU_FONT_VID) {
			const char* text = ((STEXT*) item)->m_text.m_str;
			if (text && !strcmp(text, NETUI_ENTRY_LABEL)) {
				p_out->m_ownLabel = item;
			}
			else if (!p_out->m_topLabel || y < MenuScreenY(p_out->m_topLabel)) {
				p_out->m_topLabel = item;
			}
		}
	}
	p_out->m_spacing = buttons >= 2 ? nextY - topY : 0.0f;
	return p_out->m_topButton && p_out->m_topLabel && p_out->m_spacing > 0.0f;
}

static NETUI_RECT MenuEntryRect(const GRAPH_CORE* p_core, int)
{
	NETUI_MENU_STACK stack;
	if (FindMenuStack(&stack)) {
		const float unit = stack.m_spacing / (float) NETUI_MENU_ROW_UNITS;
		const float centerX = stack.m_topButton->m_x - Map->m_shiftX;
		const float anchorY = MenuScreenY(stack.m_topButton) - stack.m_spacing;
		const NETUI_RECT rect = {
			floorf(centerX - (float) NETUI_ENTRY_HALF_W * unit),
			floorf(anchorY - (float) NETUI_ENTRY_ABOVE * unit),
			floorf(centerX + (float) NETUI_ENTRY_HALF_W * unit),
			floorf(anchorY + (float) NETUI_ENTRY_BELOW * unit)
		};
		return rect;
	}
	const float scale = UI_SCALING::NormalizeDrawScale((float) p_core->m_uiScale * p_core->m_uiPresentationScale);
	const float originX = floorf(((float) p_core->m_width - (float) GameDesc->m_uiBaseWidth * scale) * 0.5f);
	const float originY = floorf(((float) p_core->m_height - (float) GameDesc->m_uiBaseHeight * scale) * 0.5f);
	const float x0 = originX + (float) NETUI_ENTRY_X * scale;
	const float y0 = originY + (float) NETUI_ENTRY_Y * scale;
	const NETUI_RECT rect = {x0, y0, x0 + (float) NETUI_ENTRY_W * scale, y0 + (float) NETUI_ENTRY_H * scale};
	return rect;
}

static void EnsureMenuEntryLabel()
{
	NETUI_MENU_STACK stack;
	if (!MenuEntryShown() || !Map->VidExists(NETUI_MENU_FONT_VID) || !FindMenuStack(&stack) || stack.m_ownLabel) {
		return;
	}
	SPRITE* model = stack.m_topLabel;
	SPRITE* label = Map->CreateSprite(
		Map->Vid(NETUI_MENU_FONT_VID),
		model->m_x,
		model->m_y - stack.m_spacing,
		model->m_z,
		ANGLE(0),
		0
	);
	if (!label) {
		return;
	}
	label->SetUIScriptLayout(((GRAPH_CORE*) Graph)->m_uiScale, model->UIAnchorX(), model->UIAnchorY());
	const decomp_intptr behave = model->Action(NETUI_ACT_GET_BEHAVE, 0, 0, 0) & ~(decomp_intptr) NETUI_TEXT_SOURCE_MASK;
	label->Action(NETUI_ACT_SET_BEHAVE, behave, 0, 0);
	STRING text(NETUI_ENTRY_LABEL);
	label->Action(NETUI_ACT_SET_TEXT, (decomp_intptr) &text, 0, 0);
}

static void RedrawMenuEntryLabel()
{
	NETUI_MENU_STACK stack;
	FindMenuStack(&stack);
	if (stack.m_ownLabel) {
		stack.m_ownLabel->Draw();
	}
}

static int MenuEntryHasSpriteLabel()
{
	NETUI_MENU_STACK stack;
	FindMenuStack(&stack);
	return stack.m_ownLabel != 0;
}

static int PointOnMenuEntry(float p_x, float p_y)
{
	const GRAPH_CORE* core = Graph;
	return core && !g_ui.m_open && MenuEntryShown() && RectContains(MenuEntryRect(core, UiScale(core)), p_x, p_y);
}

static int OnMouseDown(const SDL_MouseButtonEvent& p_button)
{
	const unsigned int bit = MouseButtonBit(p_button.button);
	g_ui.m_mouseSwallowed &= ~bit;
	if (p_button.button == SDL_BUTTON_LEFT && PointOnMenuEntry(p_button.x, p_button.y)) {
		g_ui.m_mouseSwallowed |= bit;
		OpenPanel();
		return 1;
	}
	NETUI_LAYOUT layout;
	if (!OpenPanelLayout(&layout) || !RectContains(PanelRect(layout), p_button.x, p_button.y)) {
		return 0;
	}
	g_ui.m_mouseSwallowed |= bit;
	if (p_button.button == SDL_BUTTON_LEFT) {
		g_ui.m_notice[0] = 0;
		NormalizeFocus();
		ClickItemAt(layout, p_button.x, p_button.y);
	}
	return 1;
}

static int OnMouseUp(const SDL_MouseButtonEvent& p_button)
{
	const unsigned int bit = MouseButtonBit(p_button.button);
	if (!(g_ui.m_mouseSwallowed & bit)) {
		return 0;
	}
	g_ui.m_mouseSwallowed &= ~bit;
	return 1;
}

#ifdef OPENGROMADA_NET_TESTHOOKS

enum NETUI_TEST_KIND {
	NETUI_TEST_TAP,
	NETUI_TEST_KEY_DOWN,
	NETUI_TEST_KEY_UP,
	NETUI_TEST_TEXT,
	NETUI_TEST_MOUSE_DOWN,
	NETUI_TEST_MOUSE_UP
};

enum {
	NETUI_TEST_STEP_MAX = 64,
	NETUI_TEST_SOURCE_LEN = 1024
};

struct NETUI_TEST_KEY_NAME {
	const char* m_token;
	unsigned int m_key;
};

struct NETUI_TEST_MOUSE_NAME {
	const char* m_token;
	int m_kind;
	int m_insidePanel;
};

struct NETUI_TEST_STEP {
	unsigned int m_when;
	int m_kind;
	unsigned int m_key;
	const char* m_text;
	int m_insidePanel;
	int m_fired;
};

struct NETUI_TEST {
	int m_parsed;
	int m_count;
	unsigned int m_start;
	char m_source[NETUI_TEST_SOURCE_LEN];
	NETUI_TEST_STEP m_steps[NETUI_TEST_STEP_MAX];
};

static const NETUI_TEST_KEY_NAME NETUI_TEST_KEY_NAMES[] = {
	{"F2", SDLK_F2},
	{"F3", SDLK_F3},
	{"TAB", SDLK_TAB},
	{"ENTER", SDLK_RETURN},
	{"ESC", SDLK_ESCAPE},
	{"BKSP", SDLK_BACKSPACE},
	{"DEL", SDLK_DELETE},
	{"UP", SDLK_UP},
	{"DOWN", SDLK_DOWN}
};

static const NETUI_TEST_MOUSE_NAME NETUI_TEST_MOUSE_NAMES[] = {
	{"MDIN", NETUI_TEST_MOUSE_DOWN, 1},
	{"MUIN", NETUI_TEST_MOUSE_UP, 1},
	{"MDOUT", NETUI_TEST_MOUSE_DOWN, 0},
	{"MUOUT", NETUI_TEST_MOUSE_UP, 0},
	{"MDENTRY", NETUI_TEST_MOUSE_DOWN, 2},
	{"MUENTRY", NETUI_TEST_MOUSE_UP, 2}
};

namespace
{
NETUI_TEST g_test;
}

static int ParseTestToken(const char* p_token, NETUI_TEST_STEP* p_step)
{
	if (!strncmp(p_token, "T=", 2)) {
		p_step->m_kind = NETUI_TEST_TEXT;
		p_step->m_text = p_token + 2;
		return 1;
	}
	if (!strncmp(p_token, "D=", 2) || !strncmp(p_token, "U=", 2)) {
		p_step->m_kind = p_token[0] == 'D' ? NETUI_TEST_KEY_DOWN : NETUI_TEST_KEY_UP;
		p_step->m_key = (unsigned int) strtoul(p_token + 2, 0, 0);
		return 1;
	}
	for (const NETUI_TEST_MOUSE_NAME& name : NETUI_TEST_MOUSE_NAMES) {
		if (!strcmp(p_token, name.m_token)) {
			p_step->m_kind = name.m_kind;
			p_step->m_insidePanel = name.m_insidePanel;
			return 1;
		}
	}
	for (const NETUI_TEST_KEY_NAME& name : NETUI_TEST_KEY_NAMES) {
		if (!strcmp(p_token, name.m_token)) {
			p_step->m_kind = NETUI_TEST_TAP;
			p_step->m_key = name.m_key;
			return 1;
		}
	}
	return 0;
}

static void ParseTestSchedule(unsigned int p_now)
{
	g_test.m_parsed = 1;
	g_test.m_start = p_now;
	const char* schedule = SDL_getenv("ALIEN_NET_UIKEYS");
	if (!schedule) {
		return;
	}
	SDL_strlcpy(g_test.m_source, schedule, sizeof(g_test.m_source));
	char* cursor = g_test.m_source;
	while (cursor && *cursor && g_test.m_count < NETUI_TEST_STEP_MAX) {
		char* comma = strchr(cursor, ',');
		if (comma) {
			*comma = 0;
		}
		char* colon = strchr(cursor, ':');
		if (colon) {
			NETUI_TEST_STEP step = {};
			step.m_when = (unsigned int) strtoul(cursor, 0, 10);
			if (ParseTestToken(colon + 1, &step)) {
				g_test.m_steps[g_test.m_count++] = step;
			}
		}
		cursor = comma ? comma + 1 : 0;
	}
}

static void PushTestKey(unsigned int p_key, int p_down)
{
	SDL_Event event;
	SDL_zero(event);
	event.type = p_down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
	event.key.key = (SDL_Keycode) p_key;
	event.key.down = p_down != 0;
	SDL_PushEvent(&event);
}

static void PushTestText(const char* p_text)
{
	SDL_Event event;
	SDL_zero(event);
	event.type = SDL_EVENT_TEXT_INPUT;
	event.text.text = p_text;
	SDL_PushEvent(&event);
}

static void PushTestMouse(int p_insidePanel, int p_down)
{
	const GRAPH_CORE* core = Graph;
	NETUI_LAYOUT layout;
	if (!core || !ComputeLayout(core, CountRoster(), &layout)) {
		return;
	}
	const float k = (float) layout.m_k;
	float frameX = core->m_viewXMin + 1.0f;
	float frameY = core->m_viewYMax - 2.0f;
	if (p_insidePanel == 2) {
		const NETUI_RECT entry = MenuEntryRect(core, UiScale(core));
		frameX = (entry.m_x0 + entry.m_x1) * 0.5f;
		frameY = (entry.m_y0 + entry.m_y1) * 0.5f;
		MYERROR::Log(
			::Error,
			"NETUI: entry shown=%d spriteLabel=%d",
			MenuEntryShown() && !g_ui.m_open,
			MenuEntryHasSpriteLabel()
		);
	}
	else if (p_insidePanel) {
		frameX = layout.m_x + layout.m_w - (float) NETUI_PAD * k;
		frameY = layout.m_y + (float) (NETUI_ROW_TITLE + NETUI_GLYPH / 2) * k;
	}
	float windowX = frameX;
	float windowY = frameY;
	SDL_Renderer* renderer = Platform_RenderRenderer();
	SDL_Window* window = renderer ? SDL_GetRenderWindow(renderer) : 0;
	SDL_Event event;
	SDL_zero(event);
	if (window && SDL_RenderCoordinatesToWindow(renderer, frameX, frameY, &windowX, &windowY)) {
		event.button.windowID = SDL_GetWindowID(window);
	}
	else {
		windowX = frameX;
		windowY = frameY;
	}
	event.type = p_down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
	event.button.button = SDL_BUTTON_LEFT;
	event.button.down = p_down != 0;
	event.button.clicks = 1;
	event.button.x = windowX;
	event.button.y = windowY;
	SDL_PushEvent(&event);
}

static void FireTestStep(const NETUI_TEST_STEP& p_step)
{
	switch (p_step.m_kind) {
	case NETUI_TEST_TAP:
		PushTestKey(p_step.m_key, 1);
		PushTestKey(p_step.m_key, 0);
		break;
	case NETUI_TEST_KEY_DOWN:
		PushTestKey(p_step.m_key, 1);
		break;
	case NETUI_TEST_KEY_UP:
		PushTestKey(p_step.m_key, 0);
		break;
	case NETUI_TEST_TEXT:
		PushTestText(p_step.m_text);
		break;
	case NETUI_TEST_MOUSE_DOWN:
		PushTestMouse(p_step.m_insidePanel, 1);
		break;
	case NETUI_TEST_MOUSE_UP:
		PushTestMouse(p_step.m_insidePanel, 0);
		break;
	default:
		break;
	}
}

static void PumpTestSchedule(unsigned int p_now)
{
	if (!g_test.m_parsed) {
		ParseTestSchedule(p_now);
	}
	const unsigned int elapsed = NetElapsedMs(p_now, g_test.m_start);
	for (int i = 0; i < g_test.m_count; ++i) {
		NETUI_TEST_STEP& step = g_test.m_steps[i];
		if (!step.m_fired && elapsed >= step.m_when) {
			step.m_fired = 1;
			FireTestStep(step);
		}
	}
}

#else

static void PumpTestSchedule(unsigned int)
{
}

#endif

static int MapShowsMenu()
{
	return (Map->m_flag & NETUI_MAP_MENU_MODE) || Map->m_menuFrameActive;
}

static void StartToast(unsigned int p_now, const char* p_status)
{
	g_ui.m_toastActive = p_status[0] != 0;
	g_ui.m_toastTime = p_now;
}

static void TrackStatus(unsigned int p_now)
{
	const char* status = FirstNonEmpty(Net_Status(), "");
	if (strncmp(status, g_ui.m_toast, sizeof(g_ui.m_toast) - 1) != 0) {
		SDL_strlcpy(g_ui.m_toast, status, sizeof(g_ui.m_toast));
		g_ui.m_notice[0] = 0;
		StartToast(p_now, status);
	}
	if (g_ui.m_toastActive && NetElapsedMs(p_now, g_ui.m_toastTime) >= NETUI_TOAST_MS) {
		g_ui.m_toastActive = 0;
	}
}

static void TrackSession(unsigned int p_now)
{
	const int state = Net_UiState();
	if (g_ui.m_autoClose) {
		if (state == NET_UI_ONLINE) {
			ClosePanel();
		}
		else if (state == NET_UI_IDLE) {
			g_ui.m_autoClose = 0;
		}
	}
	if (g_ui.m_open) {
		NormalizeFocus();
	}
	TrackStatus(p_now);
}

static void CaptureOverheads()
{
	g_frame.m_count = 0;
	const GRAPH_CORE* core = Graph;
	if (!core || !Map || !Net_Playing() || MapShowsMenu()) {
		return;
	}
	g_frame.m_frameW = core->m_width;
	g_frame.m_frameH = core->m_height;
	for (int i = 0; i < NET_MAX_PLAYERS && g_frame.m_count < NET_MAX_PLAYERS; ++i) {
		NET_REMOTE_INFO info = {};
		if (!Net_RemoteInfo(i, &info)) {
			continue;
		}
		if (!NetFloatFinite(info.m_x) || !NetFloatFinite(info.m_y) || !NetFloatFinite(info.m_z)) {
			continue;
		}
		NETUI_OVERHEAD& overhead = g_frame.m_items[g_frame.m_count++];
		overhead.m_sx = info.m_x - Map->m_shiftX;
		overhead.m_sy = info.m_y - info.m_z - Map->m_shiftY - (float) NETUI_OVERHEAD_LIFT;
		overhead.m_hp = info.m_hp;
		overhead.m_maxHp = info.m_maxHp;
		NETUI_TEXT name;
		TextReset(&name);
		TextAppend(&name, info.m_name);
		SDL_strlcpy(overhead.m_name, name.m_text, sizeof(overhead.m_name));
	}
}

static int FrameWillPresent(const GRAPH_CORE* p_core)
{
	return (p_core->m_flags & NETUI_GRAPH_FRAME_OPEN) && !p_core->m_movieActive &&
		   !p_core->m_effectStart[NETUI_EFFECT_PIXEL_APPEAR] && !p_core->m_effectStart[NETUI_EFFECT_LINE_SHIFT];
}

static int OverheadsCurrent(const GRAPH_CORE* p_core)
{
	return g_frame.m_count > 0 && Map && Net_Playing() && !MapShowsMenu() && g_frame.m_frameW == p_core->m_width &&
		   g_frame.m_frameH == p_core->m_height;
}

static void FillBar(GRAPH_CORE* p_core, float p_x0, float p_y0, float p_x1, float p_y1, unsigned int p_argb)
{
	const NETUI_RECT rect = {floorf(p_x0), floorf(p_y0), floorf(p_x1), floorf(p_y1)};
	if (rect.m_x1 <= rect.m_x0 || rect.m_y1 <= rect.m_y0 || !RectsIntersect(rect, ViewRect(p_core))) {
		return;
	}
	p_core->Bar(rect.m_x0, rect.m_y0, rect.m_x1, rect.m_y1, COLOR((int) p_argb));
}

static void EmitText(const GRAPH_CORE* p_core, float p_x, float p_y, int p_k, unsigned int p_argb, const char* p_text)
{
	if (!p_text[0]) {
		return;
	}
	const float x = SnapInsideView(p_x, p_core->m_viewXMin, p_k);
	const float y = SnapInsideView(p_y, p_core->m_viewYMin, p_k);
	const float width = (float) strlen(p_text) * (float) (NETUI_GLYPH * p_k);
	const float height = (float) (NETUI_GLYPH * p_k);
	if (x < p_core->m_viewXMin || y < p_core->m_viewYMin || x + width > p_core->m_viewXMax ||
		y + height > p_core->m_viewYMax) {
		return;
	}
	Platform_RenderDebugText(x, y, p_argb, p_text, NETUI_GLYPH * p_k);
}

static float OverheadNameWidth(const NETUI_OVERHEAD& p_overhead, int p_k)
{
	return (float) strlen(p_overhead.m_name) * (float) (NETUI_GLYPH * p_k);
}

static float OverheadNameLeft(const GRAPH_CORE* p_core, const NETUI_OVERHEAD& p_overhead, int p_k)
{
	const float width = OverheadNameWidth(p_overhead, p_k);
	float left = p_overhead.m_sx - width * 0.5f;
	if (left + width > p_core->m_viewXMax) {
		left = p_core->m_viewXMax - width;
	}
	if (left < p_core->m_viewXMin) {
		left = p_core->m_viewXMin;
	}
	return left;
}

static NETUI_RECT OverheadBarBounds(const NETUI_OVERHEAD& p_overhead, int p_k)
{
	const float k = (float) p_k;
	const float half = (float) NETUI_OVERHEAD_HALF_WIDTH + k;
	const NETUI_RECT rect = {
		p_overhead.m_sx - half,
		p_overhead.m_sy - k,
		p_overhead.m_sx + half,
		p_overhead.m_sy + (float) (NETUI_OVERHEAD_TROUGH + 1) * k
	};
	return rect;
}

static NETUI_RECT OverheadNameBounds(const GRAPH_CORE* p_core, const NETUI_OVERHEAD& p_overhead, int p_k)
{
	const float left = OverheadNameLeft(p_core, p_overhead, p_k);
	const float top = p_overhead.m_sy - (float) (NETUI_OVERHEAD_NAME_RISE * p_k);
	const NETUI_RECT rect = {left, top, left + OverheadNameWidth(p_overhead, p_k), top + (float) (NETUI_GLYPH * p_k)};
	return rect;
}

static int OverheadShown(const GRAPH_CORE* p_core, const NETUI_OVERHEAD& p_overhead, int p_k, const NETUI_RECT* p_panel)
{
	const NETUI_RECT bar = OverheadBarBounds(p_overhead, p_k);
	if (!RectsIntersect(bar, ViewRect(p_core))) {
		return 0;
	}
	if (!p_panel) {
		return 1;
	}
	return !RectsIntersect(bar, *p_panel) && !RectsIntersect(OverheadNameBounds(p_core, p_overhead, p_k), *p_panel);
}

static void DrawOverheadBars(GRAPH_CORE* p_core, int p_k, const NETUI_RECT* p_panel)
{
	const float k = (float) p_k;
	for (int i = 0; i < g_frame.m_count; ++i) {
		const NETUI_OVERHEAD& overhead = g_frame.m_items[i];
		if (!OverheadShown(p_core, overhead, p_k, p_panel)) {
			continue;
		}
		const float left = overhead.m_sx - (float) NETUI_OVERHEAD_HALF_WIDTH;
		const float right = overhead.m_sx + (float) NETUI_OVERHEAD_HALF_WIDTH;
		const float top = overhead.m_sy;
		const float bottom = overhead.m_sy + (float) NETUI_OVERHEAD_TROUGH * k;
		float fraction = overhead.m_maxHp > 0 ? (float) overhead.m_hp / (float) overhead.m_maxHp : 0.0f;
		if (fraction > 1.0f) {
			fraction = 1.0f;
		}
		FillBar(p_core, left - k, top - k, right + k, bottom + k, NETUI_ARGB_HEALTH_FRAME);
		FillBar(p_core, left, top, right, bottom, NETUI_ARGB_HEALTH_TROUGH);
		if (fraction > 0.0f) {
			FillBar(p_core, left, top, left + (right - left) * fraction, bottom, NETUI_ARGB_HEALTH_FILL);
		}
	}
}

static void DrawOverheadNames(const GRAPH_CORE* p_core, int p_k, const NETUI_RECT* p_panel)
{
	const float k = (float) p_k;
	for (int i = 0; i < g_frame.m_count; ++i) {
		const NETUI_OVERHEAD& overhead = g_frame.m_items[i];
		if (!OverheadShown(p_core, overhead, p_k, p_panel)) {
			continue;
		}
		EmitText(
			p_core,
			OverheadNameLeft(p_core, overhead, p_k),
			overhead.m_sy - (float) NETUI_OVERHEAD_NAME_RISE * k,
			p_k,
			NETUI_ARGB_ENABLED,
			overhead.m_name
		);
	}
}

static int SoftwareCursorShown()
{
	return Mouse && !Mouse->m_unk0x70;
}

static void DrawPointerMarker(GRAPH_CORE* p_core, const NETUI_LAYOUT& p_layout)
{
	if (!Map || !SoftwareCursorShown()) {
		return;
	}
	const float x = floorf(Map->m_input.m_x);
	const float y = floorf(Map->m_input.m_y);
	if (!RectContains(PanelRect(p_layout), x, y)) {
		return;
	}
	const float k = (float) p_layout.m_k;
	const float arm = (float) NETUI_POINTER_ARM * k;
	FillBar(p_core, x - arm, y, x + arm + k, y + k, NETUI_ARGB_POINTER);
	FillBar(p_core, x, y - arm, x + k, y + arm + k, NETUI_ARGB_POINTER);
}

static void DrawPanelBars(GRAPH_CORE* p_core, const NETUI_LAYOUT& p_layout)
{
	const float k = (float) p_layout.m_k;
	const NETUI_RECT panel = PanelRect(p_layout);
	FillBar(p_core, panel.m_x0, panel.m_y0, panel.m_x1, panel.m_y1, NETUI_ARGB_BACKGROUND);
	FillBar(p_core, panel.m_x0, panel.m_y0, panel.m_x1, panel.m_y0 + k, NETUI_ARGB_BORDER);
	FillBar(p_core, panel.m_x0, panel.m_y1 - k, panel.m_x1, panel.m_y1, NETUI_ARGB_BORDER);
	FillBar(p_core, panel.m_x0, panel.m_y0 + k, panel.m_x0 + k, panel.m_y1 - k, NETUI_ARGB_BORDER);
	FillBar(p_core, panel.m_x1 - k, panel.m_y0 + k, panel.m_x1, panel.m_y1 - k, NETUI_ARGB_BORDER);
	if (ItemEnabled(g_ui.m_focus)) {
		const NETUI_RECT focus = ItemRect(p_layout, g_ui.m_focus);
		FillBar(p_core, focus.m_x0, focus.m_y0, focus.m_x1, focus.m_y1, NETUI_ARGB_FOCUS_BAR);
	}
	DrawPointerMarker(p_core, p_layout);
}

static void PanelText(
	const GRAPH_CORE* p_core,
	const NETUI_LAYOUT& p_layout,
	int p_column,
	int p_row,
	unsigned int p_argb,
	const char* p_text
)
{
	const float k = (float) p_layout.m_k;
	EmitText(
		p_core,
		p_layout.m_x + (float) (NETUI_PAD + p_column * NETUI_GLYPH) * k,
		p_layout.m_y + (float) p_row * k,
		p_layout.m_k,
		p_argb,
		p_text
	);
}

static unsigned int ItemColor(int p_item)
{
	if (!ItemEnabled(p_item)) {
		return NETUI_ARGB_DISABLED;
	}
	return g_ui.m_focus == p_item ? NETUI_ARGB_FOCUSED : NETUI_ARGB_ENABLED;
}

static void DrawFieldRow(
	const GRAPH_CORE* p_core,
	const NETUI_LAYOUT& p_layout,
	int p_item,
	const char* p_label,
	const char* p_value
)
{
	const int caret = ItemEnabled(p_item) && g_ui.m_focus == p_item;
	NETUI_TEXT line;
	TextReset(&line);
	TextAppend(&line, p_label);
	TextPadTo(&line, NETUI_LABEL_CHARS);
	TextAppendTail(&line, p_value, NETUI_INNER_CHARS - NETUI_LABEL_CHARS - 1);
	if (caret) {
		TextAppend(&line, "_");
	}
	PanelText(p_core, p_layout, 0, FieldRow(p_item), ItemColor(p_item), line.m_text);
}

static void MapLeaf(NETUI_TEXT* p_out, const char* p_map)
{
	const char* leaf = p_map ? p_map : "";
	for (const char* c = leaf; *c; ++c) {
		if (*c == '\\' || *c == '/') {
			leaf = c + 1;
		}
	}
	TextReset(p_out);
	TextAppend(p_out, leaf);
	if (p_out->m_length > NETUI_MAP_SUFFIX_CHARS &&
		!SDL_strcasecmp(p_out->m_text + p_out->m_length - NETUI_MAP_SUFFIX_CHARS, ".map")) {
		p_out->m_length -= NETUI_MAP_SUFFIX_CHARS;
		p_out->m_text[p_out->m_length] = 0;
	}
	TextEllipsize(p_out, NETUI_MAP_LEAF_CHARS);
}

static void BuildRoster(NETUI_ROSTER* p_out)
{
	p_out->m_count = 0;
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		NET_ROSTER_INFO info = {};
		if (!Net_RosterInfo(i, &info)) {
			continue;
		}
		const int here = info.m_local || info.m_sameMap;
		const int away = info.m_away && !info.m_local;
		NETUI_TEXT leaf;
		MapLeaf(&leaf, info.m_map);
		NETUI_ROSTER_ROW& row = p_out->m_rows[p_out->m_count++];
		row.m_argb = here ? NETUI_ARGB_ENABLED : NETUI_ARGB_DISABLED;
		TextReset(&row.m_line);
		TextAppend(&row.m_line, info.m_local ? "* " : "  ");
		TextAppend(&row.m_line, info.m_name);
		TextPadTo(&row.m_line, NET_NAME_LEN + 2);
		if (away) {
			TextAppend(&row.m_line, "away");
		}
		else {
			TextAppend(&row.m_line, leaf.m_text);
			if (!here) {
				TextAppend(&row.m_line, leaf.m_length ? "  on another map" : "on another map");
			}
		}
		TextEllipsize(&row.m_line, NETUI_INNER_CHARS);
	}
}

static void DrawPanelText(const GRAPH_CORE* p_core, const NETUI_LAYOUT& p_layout, const NETUI_ROSTER& p_roster)
{
	const int idle = Net_UiState() == NET_UI_IDLE;
	PanelText(p_core, p_layout, 0, NETUI_ROW_TITLE, NETUI_ARGB_TITLE, "MULTIPLAYER");
	DrawFieldRow(
		p_core,
		p_layout,
		NETUI_ITEM_ADDRESS,
		"Address:",
		idle ? g_ui.m_address : FirstNonEmpty(Net_ActiveAddress(), g_ui.m_address)
	);
	DrawFieldRow(
		p_core,
		p_layout,
		NETUI_ITEM_NAME,
		"Name:",
		idle ? g_ui.m_name : FirstNonEmpty(Net_ActiveName(), g_ui.m_name)
	);
	DrawFieldRow(p_core, p_layout, NETUI_ITEM_PORT, "Port:", g_ui.m_port);
	for (int item = NETUI_ITEM_CONNECT; item < NETUI_ITEM_COUNT; ++item) {
		PanelText(p_core, p_layout, ButtonColumn(item), NETUI_ROW_BUTTONS, ItemColor(item), ButtonLabel(item));
	}

	NETUI_TEXT line;
	TextReset(&line);
	TextAppend(&line, "Status: ");
	TextAppend(&line, g_ui.m_notice[0] ? g_ui.m_notice : FirstNonEmpty(Net_Status(), "not connected"));
	TextEllipsize(&line, NETUI_INNER_CHARS);
	PanelText(
		p_core,
		p_layout,
		0,
		NETUI_ROW_STATUS,
		g_ui.m_notice[0] ? NETUI_ARGB_NOTICE : NETUI_ARGB_STATUS,
		line.m_text
	);

	TextReset(&line);
	if (p_roster.m_count) {
		TextAppend(&line, "Players (");
		TextAppendInt(&line, p_roster.m_count);
		TextAppend(&line, "):");
	}
	else {
		TextAppend(&line, "Players: none");
	}
	PanelText(p_core, p_layout, 0, NETUI_ROW_ROSTER_TITLE, NETUI_ARGB_TITLE, line.m_text);
	for (int i = 0; i < p_roster.m_count; ++i) {
		const NETUI_ROSTER_ROW& row = p_roster.m_rows[i];
		PanelText(p_core, p_layout, 0, NETUI_ROW_ROSTER + i * NETUI_LINE, row.m_argb, row.m_line.m_text);
	}
	PanelText(
		p_core,
		p_layout,
		0,
		NETUI_ROW_ROSTER + p_roster.m_count * NETUI_LINE + NETUI_HELP_GAP,
		NETUI_ARGB_HELP,
		NETUI_HELP_TEXT
	);
}

static void DrawToast(const GRAPH_CORE* p_core, int p_k)
{
	const float k = (float) p_k;
	const float x = SnapInsideView(p_core->m_viewXMin + (float) NETUI_PAD * k, p_core->m_viewXMin, p_k);
	const float y = SnapInsideView(p_core->m_viewYMin + (float) NETUI_TOAST_TOP * k, p_core->m_viewYMin, p_k);
	const float room = (p_core->m_viewXMax - x) / ((float) NETUI_GLYPH * k);
	if (!(room >= 1.0f)) {
		return;
	}
	NETUI_TEXT line;
	TextReset(&line);
	TextAppend(&line, g_ui.m_toast);
	TextEllipsize(&line, (size_t) room);
	EmitText(p_core, x, y, p_k, NETUI_ARGB_STATUS, line.m_text);
}

int NetUi_ProcessEvent(const SDL_Event& p_event)
{
	if (!Net_Available()) {
		return 0;
	}
	switch (p_event.type) {
	case SDL_EVENT_KEY_DOWN:
		return OnKeyDown(p_event.key);
	case SDL_EVENT_TEXT_INPUT:
		return OnTextInput(p_event.text.text);
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		return OnMouseDown(p_event.button);
	case SDL_EVENT_MOUSE_BUTTON_UP:
		return OnMouseUp(p_event.button);
	case SDL_EVENT_MOUSE_WHEEL:
		return PointInOpenPanel(p_event.wheel.mouse_x, p_event.wheel.mouse_y);
	default:
		return 0;
	}
}

void NetUi_OnMenuLoaded(const char* p_name)
{
	const char* leaf = p_name ? p_name : "";
	for (const char* c = leaf; *c; ++c) {
		if (*c == '\\' || *c == '/') {
			leaf = c + 1;
		}
	}
	g_ui.m_mainMenuShown = !SDL_strcasecmp(leaf, "mainmenu.men");
}

static int PointerOnMenuEntry(const NETUI_RECT& p_rect)
{
	return Map && RectContains(p_rect, floorf(Map->m_input.m_x), floorf(Map->m_input.m_y));
}

static void DrawMenuEntryBars(GRAPH_CORE* p_core, int p_k)
{
	const NETUI_RECT rect = MenuEntryRect(p_core, p_k);
	const float k = (float) p_k;
	const unsigned int fill = PointerOnMenuEntry(rect) ? NETUI_ARGB_ENTRY_FILL_HOVER : NETUI_ARGB_ENTRY_FILL;
	FillBar(p_core, rect.m_x0, rect.m_y0, rect.m_x1, rect.m_y1, fill);
	FillBar(p_core, rect.m_x0, rect.m_y0, rect.m_x1, rect.m_y0 + k, NETUI_ARGB_ENTRY_BORDER);
	FillBar(p_core, rect.m_x0, rect.m_y1 - k, rect.m_x1, rect.m_y1, NETUI_ARGB_ENTRY_BORDER);
	FillBar(p_core, rect.m_x0, rect.m_y0 + k, rect.m_x0 + k, rect.m_y1 - k, NETUI_ARGB_ENTRY_BORDER);
	FillBar(p_core, rect.m_x1 - k, rect.m_y0 + k, rect.m_x1, rect.m_y1 - k, NETUI_ARGB_ENTRY_BORDER);
}

static void DimIdleMenuEntry(GRAPH_CORE* p_core, int p_k)
{
	const NETUI_RECT rect = MenuEntryRect(p_core, p_k);
	if (PointerOnMenuEntry(rect) || !MenuEntryHasSpriteLabel()) {
		return;
	}
	const float k = (float) p_k;
	const RENDER_STATE savedState = p_core->m_state;
	const unsigned int savedBlend = p_core->m_flags & NETUI_GRAPH_ALPHA_BLEND;
	FillBar(p_core, rect.m_x0 + k, rect.m_y0 + k, rect.m_x1 - k, rect.m_y1 - k, NETUI_ARGB_ENTRY_DIM);
	p_core->m_state = savedState;
	p_core->m_flags = (p_core->m_flags & ~NETUI_GRAPH_ALPHA_BLEND) | savedBlend;
}

static void DrawMenuEntryText(const GRAPH_CORE* p_core, int p_k)
{
	if (MenuEntryHasSpriteLabel()) {
		return;
	}
	const NETUI_RECT rect = MenuEntryRect(p_core, p_k);
	const float glyph = (float) (NETUI_GLYPH * p_k);
	const float x = rect.m_x0 + ((rect.m_x1 - rect.m_x0) - (float) strlen(NETUI_ENTRY_LABEL) * glyph) * 0.5f;
	const float y = rect.m_y0 + ((rect.m_y1 - rect.m_y0) - glyph) * 0.5f;
	const unsigned int color = PointerOnMenuEntry(rect) ? NETUI_ARGB_ENTRY_TEXT_HOVER : NETUI_ARGB_ENTRY_TEXT;
	EmitText(p_core, x, y, p_k, color, NETUI_ENTRY_LABEL);
}

void NetUi_Pump()
{
	if (!Net_Available()) {
		return;
	}
	const unsigned int now = Platform_Ticks();
	PumpTestSchedule(now);
	TrackSession(now);
	EnsureMenuEntryLabel();
	CaptureOverheads();
}

void NetUi_Draw()
{
	if (!Net_Available()) {
		return;
	}
	GRAPH_CORE* core = Graph;
	if (!core || !FrameWillPresent(core)) {
		return;
	}

	NETUI_ROSTER roster;
	roster.m_count = 0;
	NETUI_LAYOUT layout = {};
	int panelShown = 0;
	if (g_ui.m_open) {
		BuildRoster(&roster);
		panelShown = ComputeLayout(core, roster.m_count, &layout);
	}
	const NETUI_RECT panelRect = PanelRect(layout);
	const NETUI_RECT* panel = panelShown ? &panelRect : 0;
	const int overheadsShown = OverheadsCurrent(core);
	const int toastShown = !g_ui.m_open && g_ui.m_toastActive;
	const int entryShown = MenuEntryShown();
	if (!panelShown && !overheadsShown && !toastShown && !entryShown) {
		return;
	}

	const int hudScale = UiScale(core);
	if (panelShown || overheadsShown || entryShown) {
		const RENDER_STATE savedState = core->m_state;
		const unsigned int savedBlend = core->m_flags & NETUI_GRAPH_ALPHA_BLEND;
		if (overheadsShown) {
			DrawOverheadBars(core, hudScale, panel);
		}
		if (entryShown) {
			DrawMenuEntryBars(core, hudScale);
		}
		if (panelShown) {
			DrawPanelBars(core, layout);
		}
		core->m_state = savedState;
		core->m_flags = (core->m_flags & ~NETUI_GRAPH_ALPHA_BLEND) | savedBlend;
		if (entryShown) {
			RedrawMenuEntryLabel();
			DimIdleMenuEntry(core, hudScale);
		}
	}

	if (panelShown) {
		DrawPanelText(core, layout, roster);
	}
	if (entryShown) {
		DrawMenuEntryText(core, hudScale);
	}
	if (toastShown) {
		DrawToast(core, hudScale);
	}
	if (overheadsShown) {
		DrawOverheadNames(core, hudScale, panel);
	}
}
