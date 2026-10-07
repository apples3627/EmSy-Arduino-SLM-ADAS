#pragma once
namespace IntentConfig {
static const int VOCAB=96, EMBED=12, HIDDEN=48, LABELS=10, MAX_BYTES=96, MAX_TOKENS=16;
static const float THRESHOLD=0.9f, MARGIN=0.2f;
static const uint32_t MODEL_ID=0x54d239ecUL;
static const uint16_t PAD=0xFFFF;
static const char WORD_TEXT[] INTENT_STORAGE="a\0ahead\0all\0arrive\0back\0backward\0backwards\0beep\0beeping\0begin\0buzzer\0can\0car\0clean\0could\0disable\0do\0does\0down\0drive\0enable\0end\0everything\0extinguish\0for\0forward\0functions\0glass\0go\0halt\0headlights\0hello\0honk\0honking\0horn\0how\0i\0if\0in\0is\0it\0lamps\0learn\0light\0lights\0like\0make\0me\0mean\0moment\0move\0moving\0mute\0not\0nothing\0now\0off\0on\0outputs\0please\0proceed\0rains\0red\0reverse\0right\0robot\0roll\0shut\0silence\0slowly\0sound\0start\0stop\0straight\0sweep\0switch\0the\0this\0to\0turn\0up\0was\0weather\0what\0when\0whether\0whole\0windshield\0wipe\0wiper\0wipers\0wiping\0wonder\0work\0would\0you\0";
static const uint16_t WORD_OFFSETS[96] INTENT_STORAGE={0,2,8,12,19,24,33,43,48,56,62,69,73,77,83,89,97,100,105,110,116,123,127,138,149,153,161,171,177,180,185,196,202,207,215,220,224,226,229,232,235,238,244,250,256,263,268,273,276,281,288,293,300,305,309,317,321,325,328,336,343,351,357,361,369,375,381,386,391,399,406,412,418,423,432,438,445,449,454,457,462,465,469,477,482,487,495,501,512,517,523,530,537,544,549,555};
static const char LABEL_TEXT[] INTENT_STORAGE="FORWARD\0REVERSE\0STOP\0LIGHT_ON\0LIGHT_OFF\0WIPER_ON\0WIPER_OFF\0UNKNOWN\0HORN_ON\0HORN_OFF\0";
static const uint16_t LABEL_OFFSETS[10] INTENT_STORAGE={0,8,16,21,30,40,49,59,67,75};
static const char GUARD_TEXT[] INTENT_STORAGE="and\0are\0how\0if\0is\0like\0likes\0maybe\0never\0no\0not\0or\0said\0then\0unless\0was\0what\0when\0where\0whether\0why\0wonder\0yesterday\0";
static const uint16_t GUARD_OFFSETS[23] INTENT_STORAGE={0,4,8,12,15,18,23,29,35,41,44,48,51,56,61,68,72,77,82,88,96,100,107};
static const int GUARD_COUNT=23;
}
