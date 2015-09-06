#include "input.h"

namespace Blocks
{

const char* gKeyName[] = {
"UNDEF",
"ESC",
"TAB",
"CAPS LOCK",
"Left shift",
"Right shift",
"Left ctrl",
"Right control",
"Left alt",
"Right alt",
"Left win",
"Right win",
"Space",
"Backspace",
"Enter",
"~",
"/",
"\\",
"|",
"[",
"]",
",",
".",
";",
"'",
"-",
"=",
"F1",
"F2",
"F3",
"F4",
"F5",
"F6",
"F7",
"F8",
"F9",
"F10",
"F11",
"F12",
"PrtScr",
"Scroll Lock",
"Pause",
"Insert",
"Del",
"Home",
"End",
"Page Up",
"Page Down",
"0",
"1",
"2",
"3",
"4",
"5",
"6",
"7",
"8",
"9",
"Unused",
"Unused",
"Unused",
"Unused",
"Unused",
"Unused",
"Unused",
"A",
"B",
"C",
"D",
"E",
"F",
"G",
"H",
"I",
"J",
"K",
"L",
"M",
"N",
"O",
"P",
"Q",
"R",
"S",
"T",
"U",
"V",
"W",
"X",
"Y",
"Z",
"Up",
"Down",
"Left",
"Right",
"Shift",
"Ctrl",
"Alt",
"Win",
"Volume+",
"Volue-",
"Mute",
"Num Lock",
"Num 1",
"Num 2",
"Num 3",
"Num 4",
"Num 5",
"Num 6",
"Num 7",
"Num 8",
"Num 9",
"Num 10",
"Num /",
"Num *",
"Num +",
"Num -",
"Num Enter",
"Num .",
"Num Separator",
"LMB",
"RMB",
"MMB",
"MB4",
"MB5",
"Scroll Up",
"Scroll Down"
};

const char* GetKeyName( KEY key )
{
	return gKeyName[ ( uint8 )key ];
}

// https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
KEY gKeyValuesForVKeys[] = {
KEY::UNDEF,		
KEY::LMB,		/* VK_LBUTTON 		0x01  */
KEY::RMB,		/* VK_RBUTTON 		0x02 */
KEY::UNDEF,		/* VK_CANCEL 		0x03 Control-break processing */
KEY::MMB,		/* VK_MBUTTON 		0x04 Middle mouse button (three-button mouse) */
KEY::MB4,		/* VK_XBUTTON1 		0x05 X1 mouse button */
KEY::MB5,		/* VK_XBUTTON2 		0x06 X2 mouse button */
KEY::UNDEF,		/* - 				0x07 Undefined */
KEY::BACKSPACE,	/* VK_BACK 			0x08 BACKSPACE key */
KEY::TAB,		/* VK_TAB 			0x09 TAB key */
KEY::UNDEF,		/* - 				0x0A Reserved */
KEY::UNDEF,		/* - 				0x0B Reserved */
KEY::UNDEF,		/* VK_CLEAR 		0x0C CLEAR key */
KEY::ENTER,		/* VK_RETURN 		0x0D ENTER key */
KEY::UNDEF,		/* - 				0x0E Undefined */
KEY::UNDEF,		/* - 				0x0F Undefined */
KEY::SHIFT,		/* VK_SHIFT 		0x10 SHIFT key */
KEY::CTRL,		/* VK_CONTROL 		0x11 CTRL key */
KEY::ALT,		/* VK_MENU 			0x12 ALT key */
KEY::PAUSE,		/* VK_PAUSE 		0x13 PAUSE key */
KEY::CAPS,		/* VK_CAPITAL 		0x14 CAPS LOCK key */
KEY::UNDEF,		/* VK_KANA 			0x15 IME Kana mode VK_HANGUEL 		0x15 IME Hanguel mode (maintained for compatibility; use VK_HANGUL) VK_HANGUL 		0x15 IME Hangul mode */
KEY::UNDEF,		/* - 				0x16 Undefined */
KEY::UNDEF,		/* VK_JUNJA 		0x17 IME Junja mode */
KEY::UNDEF,		/* VK_FINAL 		0x18 IME final mode */
KEY::UNDEF,		/* VK_HANJA 		0x19 IME Hanja mode VK_KANJI 		0x19 IME Kanji mode */
KEY::UNDEF,		/* - 				0x1A Undefined */
KEY::ESC,		/* VK_ESCAPE 		0x1B ESC key */
KEY::UNDEF,		/* VK_CONVERT 		0x1C IME convert */
KEY::UNDEF,		/* VK_NONCONVERT 	0x1D IME nonconvert */
KEY::UNDEF,		/* VK_ACCEPT 		0x1E IME accept */
KEY::UNDEF,		/* VK_MODECHANGE 	0x1F IME mode change request */
KEY::SPACE,		/* VK_SPACE 		0x20 SPACEBAR */
KEY::PAGEUP,		/* VK_PRIOR 		0x21 PAGE UP key */
KEY::PAGEDOWN,	/* VK_NEXT 			0x22 PAGE DOWN key */
KEY::END,		/* VK_END 			0x23 END key */
KEY::HOME,		/* VK_HOME 			0x24 HOME key */
KEY::LEFT,		/* VK_LEFT 			0x25 LEFT ARROW key */
KEY::UP,			/* VK_UP 			0x26 UP ARROW key */
KEY::RIGHT,		/* VK_RIGHT 		0x27 RIGHT ARROW key */
KEY::DOWN,		/* VK_DOWN 			0x28 DOWN ARROW key */
KEY::UNDEF,		/* VK_SELECT 		0x29 SELECT key */
KEY::UNDEF,		/* VK_PRINT 		0x2A PRINT key */
KEY::UNDEF,		/* VK_EXECUTE 		0x2B EXECUTE key */
KEY::PRTSCR,		/* VK_SNAPSHOT 		0x2C PRINT SCREEN key */
KEY::INSERT,		/* VK_INSERT 		0x2D INS key */
KEY::DEL,		/* VK_DELETE 		0x2E DEL key */
KEY::UNDEF,		/* VK_HELP 			0x2F HELP key */
KEY::ZERO,		/* '0'				0x30 0 key */
KEY::ONE,		/* '1'				0x31 1 key */
KEY::TWO,		/* '2'				0x32 2 key */
KEY::THREE,		/* '3'				0x33 3 key */
KEY::FOUR,		/* '4'				0x34 4 key */
KEY::FIVE,		/* '5'				0x35 5 key */
KEY::SIX,		/* '6'				0x36 6 key */
KEY::SEVEN,		/* '7'				0x37 7 key */
KEY::EIGHT,		/* '8'				0x38 8 key */
KEY::NINE,		/* '9'				0x39 9 key */
KEY::UNDEF,		/* - 				0x3A Undefined */
KEY::UNDEF,		/* - 				0x3B Undefined */
KEY::UNDEF,		/* - 				0x3C Undefined */
KEY::UNDEF,		/* - 				0x3D Undefined */
KEY::UNDEF,		/* - 				0x3E Undefined */
KEY::UNDEF,		/* - 				0x3F Undefined */
KEY::UNDEF,		/* - 				0x40 Undefined */
KEY::A,			/* 'A'				0x41 A key */
KEY::B,			/* 'B'				0x42 B key */
KEY::C,			/* 'C'				0x43 C key */
KEY::D,			/* 'D'				0x44 D key */
KEY::E,			/* 'E'				0x45 E key */
KEY::F,			/* 'F'				0x46 F key */
KEY::G,			/* 'G'				0x47 G key */
KEY::H,			/* 'H'				0x48 H key */
KEY::I,			/* 'I'				0x49 I key */
KEY::J,			/* 'J'				0x4A J key */
KEY::K,			/* 'K'				0x4B K key */
KEY::L,			/* 'L'				0x4C L key */
KEY::M,			/* 'M'				0x4D M key */
KEY::N,			/* 'N'				0x4E N key */
KEY::O,			/* 'O'				0x4F O key */
KEY::P,			/* 'P'				0x50 P key */
KEY::Q,			/* 'Q'				0x51 Q key */
KEY::R,			/* 'R'				0x52 R key */
KEY::S,			/* 'S'				0x53 S key */
KEY::T,			/* 'T'				0x54 T key */
KEY::U,			/* 'U'				0x55 U key */
KEY::V,			/* 'V'				0x56 V key */
KEY::W,			/* 'W'				0x57 W key */
KEY::X,			/* 'X'				0x58 X key */
KEY::Y,			/* 'Y'				0x59 Y key */
KEY::Z,			/* 'Z'				0x5A Z key */
KEY::LWIN,		/* VK_LWIN 			0x5B Left Windows key (Natural keyboard) */
KEY::RWIN,		/* VK_RWIN 			0x5C Right Windows key (Natural keyboard) */
KEY::UNDEF,		/* VK_APPS 			0x5D Applications key (Natural keyboard) */
KEY::UNDEF,		/* - 				0x5E Reserved */
KEY::UNDEF,		/* VK_SLEEP 		0x5F Computer Sleep key */
KEY::NUM_0,		/* VK_NUMPAD0 		0x60 Numeric keypad 0 key */
KEY::NUM_1,		/* VK_NUMPAD1 		0x61 Numeric keypad 1 key */
KEY::NUM_2,		/* VK_NUMPAD2 		0x62 Numeric keypad 2 key */
KEY::NUM_3,		/* VK_NUMPAD3 		0x63 Numeric keypad 3 key */
KEY::NUM_4,		/* VK_NUMPAD4 		0x64 Numeric keypad 4 key */
KEY::NUM_5,		/* VK_NUMPAD5 		0x65 Numeric keypad 5 key */
KEY::NUM_6,		/* VK_NUMPAD6 		0x66 Numeric keypad 6 key */
KEY::NUM_7,		/* VK_NUMPAD7 		0x67 Numeric keypad 7 key  */
KEY::NUM_8,		/* VK_NUMPAD8 		0x68 Numeric keypad 8 key */
KEY::NUM_9,		/* VK_NUMPAD9 		0x69 Numeric keypad 9 key */
KEY::NUM_MUL,		/* VK_MULTIPLY 		0x6A Multiply key */
KEY::NUM_ADD,		/* VK_ADD 			0x6B Add key */
KEY::NUM_SEPARATOR,	/* VK_SEPARATOR 	0x6C Separator key */
KEY::NUM_SUB,		/* VK_SUBTRACT 		0x6D Subtract key */
KEY::NUM_DECIMAL,	/* VK_DECIMAL 		0x6E Decimal key */
KEY::NUM_DIV,		/* VK_DIVIDE 		0x6F Divide key */
KEY::F1,			/* VK_F1 			0x70 F1 key */
KEY::F2,			/* VK_F2 			0x71 F2 key */
KEY::F3,			/* VK_F3 			0x72 F3 key */
KEY::F4,			/* VK_F4 			0x73 F4 key */
KEY::F5,			/* VK_F5 			0x74 F5 key */
KEY::F6,			/* VK_F6 			0x75 F6 key */
KEY::F7,			/* VK_F7 			0x76 F7 key */
KEY::F8,			/* VK_F8 			0x77 F8 key */
KEY::F9,			/* VK_F9 			0x78 F9 key */
KEY::F10,		/* VK_F10 			0x79 F10 key */
KEY::F11,		/* VK_F11 			0x7A F11 key */
KEY::F12,		/* VK_F12 			0x7B F12 key */
KEY::UNDEF,		/* VK_F13 			0x7C F13 key */
KEY::UNDEF,		/* VK_F14 			0x7D F14 key */
KEY::UNDEF,		/* VK_F15 			0x7E F15 key */
KEY::UNDEF,		/* VK_F16 			0x7F F16 key */
KEY::UNDEF,		/* VK_F17 			0x80 F17 key */
KEY::UNDEF,		/* VK_F18 			0x81 F18 key */
KEY::UNDEF,		/* VK_F19 			0x82 F19 key */
KEY::UNDEF,		/* VK_F20 			0x83 F20 key */
KEY::UNDEF,		/* VK_F21 			0x84 F21 key */
KEY::UNDEF,		/* VK_F22 			0x85 F22 key */
KEY::UNDEF,		/* VK_F23 			0x86 F23 key */
KEY::UNDEF,		/* VK_F24 			0x87 F24 key */
KEY::UNDEF,		/* - 				0x88 Unassigned */
KEY::UNDEF,		/* - 				0x89 Unassigned */
KEY::UNDEF,		/* - 				0x8A Unassigned */
KEY::UNDEF,		/* - 				0x8B Unassigned */
KEY::UNDEF,		/* - 				0x8C Unassigned */
KEY::UNDEF,		/* - 				0x8D Unassigned */
KEY::UNDEF,		/* - 				0x8E Unassigned */
KEY::UNDEF,		/* - 				0x8F Unassigned */
KEY::NUM,		/* VK_NUMLOCK 		0x90 NUM LOCK key */
KEY::SCROLL,		/* VK_SCROLL 		0x91 SCROLL LOCK key */
KEY::UNDEF,		/* 					0x92 OEM specific */
KEY::UNDEF,		/* 					0x93 OEM specific */
KEY::UNDEF,		/* 					0x94 OEM specific */
KEY::UNDEF,		/* 					0x95 OEM specific */
KEY::UNDEF,		/* 					0x96 OEM specific */
KEY::UNDEF,		/* - 				0x97 Unassigned */
KEY::UNDEF,		/* - 				0x98 Unassigned */
KEY::UNDEF,		/* - 				0x99 Unassigned */
KEY::UNDEF,		/* - 				0x9A Unassigned */
KEY::UNDEF,		/* - 				0x9B Unassigned */
KEY::UNDEF,		/* - 				0x9C Unassigned */
KEY::UNDEF,		/* - 				0x9D Unassigned */
KEY::UNDEF,		/* - 				0x9E Unassigned */
KEY::UNDEF,		/* - 				0x9F Unassigned */
KEY::LSHIFT,		/* VK_LSHIFT 		0xA0 Left SHIFT key */
KEY::RSHIFT,		/* VK_RSHIFT 		0xA1 Right SHIFT key */
KEY::LCTRL,		/* VK_LCONTROL 		0xA2 Left CONTROL key */
KEY::RCTRL,		/* VK_RCONTROL 		0xA3 Right CONTROL key */
KEY::LALT,		/* VK_LMENU 		0xA4 Left MENU key */
KEY::RALT,		/* VK_RMENU 		0xA5 Right MENU key */
KEY::UNDEF,		/* VK_BROWSER_BACK 	0xA6 Browser Back key */
KEY::UNDEF,		/* VK_BROWSER_FORWARD 0xA7 Browser Forward key */
KEY::UNDEF,		/* VK_BROWSER_REFRESH 0xA8 Browser Refresh key */
KEY::UNDEF,		/* VK_BROWSER_STOP 	0xA9 Browser Stop key */
KEY::UNDEF,		/* VK_BROWSER_SEARCH 0xAA Browser Search key */
KEY::UNDEF,		/* VK_BROWSER_FAVORITES 0xAB Browser Favorites key */
KEY::UNDEF,		/* VK_BROWSER_HOME	0xAC Browser Start and Home key */
KEY::VOL_MUTE,	/* VK_VOLUME_MUTE 	0xAD Volume Mute key */
KEY::VOL_UP,		/* VK_VOLUME_DOWN 	0xAE Volume Down key */
KEY::VOL_DOWN,	/* VK_VOLUME_UP 	0xAF Volume Up key */
KEY::UNDEF,		/* VK_MEDIA_NEXT_TRACK 0xB0 Next Track key */
KEY::UNDEF,		/* VK_MEDIA_PREV_TRACK 0xB1 Previous Track key */
KEY::UNDEF,		/* VK_MEDIA_STOP 	0xB2 Stop Media key */
KEY::UNDEF,		/* VK_MEDIA_PLAY_PAUSE 0xB3 Play/Pause Media key */
KEY::UNDEF,		/* VK_LAUNCH_MAIL 	0xB4 Start Mail key */
KEY::UNDEF,		/* VK_LAUNCH_MEDIA_SELECT 0xB5 Select Media key */
KEY::UNDEF,		/* VK_LAUNCH_APP1 	0xB6 Start Application 1 key */
KEY::UNDEF,		/* VK_LAUNCH_APP2 	0xB7 Start Application 2 key */
KEY::UNDEF,		/* - 				0xB8 Reserved */
KEY::UNDEF,		/* - 				0xB9 Reserved */
KEY::SEMICOLON,	/* VK_OEM_1 		0xBA Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ';:' key */
KEY::EQUAL,		/* VK_OEM_PLUS 		0xBB For any country/region, the '+' key */
KEY::COMMA,		/* VK_OEM_COMMA 	0xBC For any country/region, the ',' key */
KEY::HYPHEN,		/* VK_OEM_MINUS 	0xBD For any country/region, the '-' key */
KEY::PERIOD,		/* VK_OEM_PERIOD 	0xBE For any country/region, the '.' key */
KEY::SLASH,		/* VK_OEM_2 		0xBF Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '/?' key */
KEY::TILDE,		/* VK_OEM_3 		0xC0 Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '`~' key */
KEY::UNDEF,		/* - 				0xC1 Reserved */
KEY::UNDEF,		/* - 				0xC2 Reserved */
KEY::UNDEF,		/* - 				0xC3 Reserved */
KEY::UNDEF,		/* - 				0xC4 Reserved */
KEY::UNDEF,		/* - 				0xC5 Reserved */
KEY::UNDEF,		/* - 				0xC6 Reserved */
KEY::UNDEF,		/* - 				0xC7 Reserved */
KEY::UNDEF,		/* - 				0xC8 Reserved */
KEY::UNDEF,		/* - 				0xC9 Reserved */
KEY::UNDEF,		/* - 				0xCA Reserved */
KEY::UNDEF,		/* - 				0xCB Reserved */
KEY::UNDEF,		/* - 				0xCC Reserved */
KEY::UNDEF,		/* - 				0xCD Reserved */
KEY::UNDEF,		/* - 				0xCE Reserved */
KEY::UNDEF,		/* - 				0xCF Reserved */
KEY::UNDEF,		/* - 				0xD0 Reserved */
KEY::UNDEF,		/* - 				0xD1 Reserved */
KEY::UNDEF,		/* - 				0xD2 Reserved */
KEY::UNDEF,		/* - 				0xD3 Reserved */
KEY::UNDEF,		/* - 				0xD4 Reserved */
KEY::UNDEF,		/* - 				0xD5 Reserved */
KEY::UNDEF,		/* - 				0xD6 Reserved */
KEY::UNDEF,		/* - 				0xD7 Reserved */
KEY::UNDEF,		/* - 				0xD8 Unassigned */
KEY::UNDEF,		/* - 				0xD9 Unassigned */
KEY::UNDEF,		/* - 				0xDA Unassigned */
KEY::LBRACKET,	/* VK_OEM_4 		0xDB Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '[{' key */
KEY::BACKSLASH,	/* VK_OEM_5 		0xDC Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\|' key */
KEY::RBRACKET,	/* VK_OEM_6 		0xDD Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ']}' key */
KEY::QUOTE,		/* VK_OEM_7 		0xDE Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the 'single-quote/double-quote' key */
KEY::UNDEF,		/* VK_OEM_8 		0xDF Used for miscellaneous characters; it can vary by keyboard. */
KEY::UNDEF,		/* - 				0xE0 Reserved */
KEY::UNDEF,		/* 					0xE1 OEM specific */
KEY::UNDEF,		/* VK_OEM_102 		0xE2 Either the angle bracket key or the backslash key on the RT 102-key keyboard */
KEY::UNDEF,		/* 					0xE3 OEM specific VK_PROCESSKEY */
KEY::UNDEF,		/* 					0xE4 OEM specific VK_PROCESSKEY */
KEY::UNDEF,		/* 					0xE5 IME PROCESS key  */
KEY::UNDEF,		/* 					0xE6 OEM specific  */
KEY::UNDEF,		/* VK_PACKET 		0xE7 Used to pass Unicode characters as if they were keystrokes. The VK_PACKET key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in KEYBDINPUT, SendInput, WM_KEYDOWN, and WM_KEYUP */
KEY::UNDEF,		/* - 				0xE8 Unassigned */
KEY::UNDEF,		/* 					0xE9 OEM specific */
KEY::UNDEF,		/* 					0xEA OEM specific */
KEY::UNDEF,		/* 					0xEB OEM specific */
KEY::UNDEF,		/* 					0xEC OEM specific */
KEY::UNDEF,		/* 					0xED OEM specific */
KEY::UNDEF,		/* 					0xEF OEM specific */
KEY::UNDEF,		/* 					0xEA OEM specific */
KEY::UNDEF,		/* 					0xF0 OEM specific */
KEY::UNDEF,		/* 					0xF1 OEM specific */
KEY::UNDEF,		/* 					0xF2 OEM specific */
KEY::UNDEF,		/* 					0xF3 OEM specific */
KEY::UNDEF,		/* 					0xF4 OEM specific */
KEY::UNDEF,		/* 					0xF5 OEM specific */
KEY::UNDEF,		/* VK_ATTN 			0xF6 Attn key */
KEY::UNDEF,		/* VK_CRSEL 		0xF7 CrSel key */
KEY::UNDEF,		/* VK_EXSEL 		0xF8 ExSel key */
KEY::UNDEF,		/* VK_EREOF 		0xF9 Erase EOF key */
KEY::UNDEF,		/* VK_PLAY 			0xFA Play key */
KEY::UNDEF,		/* VK_ZOOM 			0xFB Zoom key */
KEY::UNDEF,		/* VK_NONAME 		0xFC Reserved */
KEY::UNDEF,		/* VK_PA1 			0xFD PA1 key */
KEY::UNDEF,		/* VK_OEM_CLEAR 	0xFE Clear key */
};


KEY VKeyToKey( unsigned short vKey )
{
	return gKeyValuesForVKeys[ vKey ];
}

} // namespace Blocks