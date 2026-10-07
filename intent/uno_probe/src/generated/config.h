#pragma once
namespace IntentConfig {
static const int VOCAB=154, EMBED=16, HIDDEN=64, LABELS=10, MAX_BYTES=96, MAX_TOKENS=16;
static const float THRESHOLD=0.9f, MARGIN=0.2f;
static const uint32_t MODEL_ID=0xf044b79eUL;
static const uint16_t PAD=0xFFFF;
static const char WORD_TEXT[] INTENT_STORAGE="a\0activate\0advance\0after\0ahead\0alarm\0all\0and\0arrive\0asked\0at\0back\0backward\0backwards\0beep\0beeper\0beeping\0before\0begin\0bit\0buzzer\0can\0car\0cat\0clean\0cleaning\0completely\0could\0deactivate\0dinner\0disable\0disengage\0do\0down\0drive\0driver\0driving\0emergency\0enable\0end\0engage\0everything\0extinguish\0for\0forward\0forwards\0front\0full\0functions\0gently\0glass\0go\0halt\0happens\0headlamps\0headlights\0honk\0honking\0horn\0i\0if\0immediately\0in\0is\0it\0just\0keep\0kindly\0lamp\0lamps\0light\0lighting\0lights\0like\0likes\0little\0low\0make\0making\0me\0milk\0moment\0motion\0motor\0move\0moving\0mute\0need\0never\0noise\0not\0nothing\0now\0off\0on\0only\0outputs\0permission\0piezo\0play\0please\0power\0proceed\0quiet\0rains\0rear\0reverse\0right\0roll\0run\0said\0send\0shut\0silence\0slowly\0sound\0sounding\0speed\0start\0stop\0straight\0sweep\0sweeping\0switch\0systems\0take\0the\0to\0travel\0turn\0up\0vehicle\0waiting\0want\0weather\0what\0when\0whether\0while\0whole\0why\0will\0window\0windscreen\0windshield\0wipe\0wiper\0wipers\0wiping\0without\0wonder\0would\0yesterday\0you\0";
static const uint16_t WORD_OFFSETS[154] INTENT_STORAGE={0,2,11,19,25,31,37,41,45,52,58,61,66,75,85,90,97,105,112,118,122,129,133,137,141,147,156,167,173,184,191,199,209,212,217,223,230,238,248,255,259,266,277,288,292,300,309,315,320,330,337,343,346,351,359,369,380,385,393,398,400,403,415,418,421,424,429,434,441,446,452,458,467,474,479,485,492,496,501,508,511,516,523,530,536,541,548,553,558,564,570,574,582,586,590,593,598,606,617,623,628,635,641,649,655,661,666,674,680,685,689,694,699,704,712,719,725,734,740,746,751,760,766,775,782,790,795,799,802,809,814,817,825,833,838,846,851,856,864,870,876,880,885,892,903,914,919,925,932,939,947,954,960,970};
static const char LABEL_TEXT[] INTENT_STORAGE="FORWARD\0REVERSE\0STOP\0LIGHT_ON\0LIGHT_OFF\0WIPER_ON\0WIPER_OFF\0UNKNOWN\0HORN_ON\0HORN_OFF\0";
static const uint16_t LABEL_OFFSETS[10] INTENT_STORAGE={0,8,16,21,30,40,49,59,67,75};
static const char GUARD_TEXT[] INTENT_STORAGE="after\0and\0are\0because\0before\0how\0if\0is\0likes\0maybe\0might\0never\0no\0not\0only\0or\0said\0should\0then\0unless\0was\0what\0when\0where\0whether\0while\0why\0without\0wonder\0yesterday\0";
static const uint16_t GUARD_OFFSETS[30] INTENT_STORAGE={0,6,10,14,22,29,33,36,39,45,51,57,63,66,70,75,78,83,90,95,102,106,111,116,122,130,136,140,148,155};
static const int GUARD_COUNT=30;
}
