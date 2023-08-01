#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "patch.h"//Fois patcher
#include "defs.h"//some addresses and constants
#include "names.h"//some constants
#include "chars.h"//font

#define BREATH_MANACOST 150
#define TP_MANACOST 100

bool game_started = false;

WORD m_screen_w = 640;
WORD m_screen_h = 480;
WORD m_minx = 176;
WORD m_miny = 16;
WORD m_maxx = 616;
WORD m_maxy = 456;
WORD m_added_height = m_screen_w - 480;
WORD m_added_width = m_screen_h - 640;
WORD m_align_y = 0;
WORD m_align_x = 0;

byte a_custom = 0;//custom game
byte reg[4] = { 0 };//region
int* unit[1610];//array for units
int* unitt[1610];//temp array for units
int units = 0;//number of units in array
byte ua[255] = { 255 };//attackers array
byte ut[255] = { 255 };//targets array     only unit ua[i] can deal damage to unit ut[i]
byte m_devotion[256] = { 255 };//units that have defence aura
bool first_step = false;//first step of trigger
bool ai_fixed = false;//ai fix
bool saveload_fixed = false;//saveload ai break bug fix
bool A_portal = false;//portals activated
bool A_autoheal = false;//paladins autoheal activated
bool A_rally = false;//rally point
bool A_tower = false;//tower control

char churc[] = "\x0\x0\xa\x0\xc0\x43\x44\x0\x90\xe6\x40\x0\x14\x21\x72\x1\x0\x0\x0\x0\x0\x0\x6b\x0\x10\x44\x44\x0\x90\xe6\x40\x0\x1\x23\x81\x1\x0\x0\x0\x0\x1\x0\x6e\x0\x10\x44\x44\x0\x90\xe6\x40\x0\x3\x24\x83\x1\x0\x0\x0\x0\x3\x0\x6d\x0\xf0\x40\x44\x0\xf0\x40\x44\x0\x0\x0\x6c\x1\x0\x0\x0\x0";

struct Vizs
{
    byte x = 0;
    byte y = 0;
    byte p = 0;
    byte s = 0;
};
Vizs vizs_areas[2000];
int vizs_n = 0;

bool can_rain = false;
#define RAINDROPS 1000//max amount
WORD raindrops_amount = RAINDROPS;
byte raindrops_density = 4;
byte raindrops_speed = 2;
byte raindrops_size = 8;
byte raindrops_size_x = 5;//cannot be more than raindrops_size
byte raindrops_size_y = 6;//cannot be more than raindrops_size
byte raindrops_align_x = raindrops_size - raindrops_size_x;
byte raindrops_align_y = raindrops_size - raindrops_size_y;
bool raindrops_snow = false;
#define THUNDER_GRADIENT  20
#define THUNDER_CHANGE  12
WORD raindrops_thunder = 0;
WORD raindrops_thunder_timer = 0;
WORD raindrops_thunder_gradient = THUNDER_GRADIENT;
#define RAINDROPS_COLOR  0xB
byte real_palette[256 * 4];

struct raindrop
{
    WORD x1 = 0;
    WORD y1 = 0;
    WORD x2 = 0;
    WORD y2 = 0;
    WORD l = 0;
};
raindrop raindrops[RAINDROPS];

#define my_min(x,y) (((x)<(y))?(x):(y))
#define my_max(x,y) (((x)>(y))?(x):(y))

void save_palette()
{
    char snp[] = "storm.dll";
    int ad = (int)GetModuleHandleA(snp);
    if (ad != 0)
    {
        ad += 0x39B14;
        for (int i = 0; i < 256 * 4; i++)
        {
            real_palette[i] = *(byte*)(ad + i);
        }
    }
}

void change_palette(bool a)
{
    char snp[] = "storm.dll";
    int ad = (int)GetModuleHandleA(snp);
    if (ad != 0)
    {
        ad += 0x39B14;
        for (int i = 0; i < 256 * 4; i++)
        {
            byte c = *(byte*)(ad + i);
            if (a)
            {
                if (c < (255 - THUNDER_CHANGE))c += THUNDER_CHANGE;
                else c = 255;
            }
            else
            {
                if (c > THUNDER_CHANGE)c -= THUNDER_CHANGE;
                else c = 0;
            }
            *(byte*)(ad + i) = c;
        }
    }
}

void reset_palette()
{
    char snp[] = "storm.dll";
    int ad = (int)GetModuleHandleA(snp);
    if (ad != 0)
    {
        ad += 0x39B14;
        for (int i = 0; i < 256 * 4; i++)
        {
            *(byte*)(ad + i) = real_palette[i];
        }
    }
}

//-----sound
void* def_name = NULL;
void* def_sound = NULL;
void* def_name_seq = NULL;
void* def_sound_seq = NULL;

char drak1_name[] = "RedMist\\speech\\ace\\d1.wav\x0";
void* drak1_sound = NULL;
char drak2_name[] = "RedMist\\speech\\ace\\d2.wav\x0";
void* drak2_sound = NULL;
char drak3_name[] = "RedMist\\speech\\ace\\d3.wav\x0";
void* drak3_sound = NULL;
char drak4_name[] = "RedMist\\speech\\ace\\d4.wav\x0";
void* drak4_sound = NULL;
char drak5_name[] = "RedMist\\speech\\ace\\d5.wav\x0";
void* drak5_sound = NULL;
char drak6_name[] = "RedMist\\speech\\ace\\d6.wav\x0";
void* drak6_sound = NULL;
char drak7_name[] = "RedMist\\speech\\ace\\d7.wav\x0";
void* drak7_sound = NULL;
char drak8_name[] = "RedMist\\speech\\ace\\d8.wav\x0";
void* drak8_sound = NULL;
char* drak_names[] = { drak1_name,drak2_name,drak3_name,drak4_name,drak5_name,drak6_name,drak7_name ,drak8_name };
void* drak_sounds[] = { drak1_sound,drak2_sound,drak3_sound,drak4_sound,drak5_sound,drak6_sound,drak7_sound,drak8_sound };

char hy1_name[] = "RedMist\\speech\\comander\\dnyessr1.wav\x0";
void* hy1_sound = NULL;
char hy2_name[] = "RedMist\\speech\\comander\\dnyessr2.wav\x0";
void* hy2_sound = NULL;
char hy3_name[] = "RedMist\\speech\\comander\\dnyessr3.wav\x0";
void* hy3_sound = NULL;
char hw1_name[] = "RedMist\\speech\\comander\\dnwhat1.wav\x0";
void* hw1_sound = NULL;
char hw2_name[] = "RedMist\\speech\\comander\\dnwhat2.wav\x0";
void* hw2_sound = NULL;
char hw3_name[] = "RedMist\\speech\\comander\\dnwhat3.wav\x0";
void* hw3_sound = NULL;
char hp1_name[] = "RedMist\\speech\\comander\\dnpisd1.wav\x0";
void* hp1_sound = NULL;
char hp2_name[] = "RedMist\\speech\\comander\\dnpisd2.wav\x0";
void* hp2_sound = NULL;
char hp3_name[] = "RedMist\\speech\\comander\\dnpisd3.wav\x0";
void* hp3_sound = NULL;
char* h_names[] = { hp1_name,hp2_name,hp3_name,hw1_name,hw2_name,hw3_name,hy1_name,hy2_name,hy3_name };
void* h_sounds[] = { hp1_sound,hp2_sound,hp3_sound,hw1_sound,hw2_sound,hw3_sound,hy1_sound,hy2_sound,hy3_sound };

char py1_name[] = "RedMist\\speech\\prince\\kayessr1.wav\x0";
void* py1_sound = NULL;
char py2_name[] = "RedMist\\speech\\prince\\kayessr2.wav\x0";
void* py2_sound = NULL;
char py3_name[] = "RedMist\\speech\\prince\\kayessr3.wav\x0";
void* py3_sound = NULL;
char pw1_name[] = "RedMist\\speech\\prince\\kawhat1.wav\x0";
void* pw1_sound = NULL;
char pw2_name[] = "RedMist\\speech\\prince\\kawhat2.wav\x0";
void* pw2_sound = NULL;
char pw3_name[] = "RedMist\\speech\\prince\\kawhat3.wav\x0";
void* pw3_sound = NULL;
char pp1_name[] = "RedMist\\speech\\prince\\kapissd1.wav\x0";
void* pp1_sound = NULL;
char pp2_name[] = "RedMist\\speech\\prince\\kapissd2.wav\x0";
void* pp2_sound = NULL;
char pp3_name[] = "RedMist\\speech\\prince\\kapissd3.wav\x0";
void* pp3_sound = NULL;
char* p_names[] = { pp1_name,pp2_name,pp3_name,pw1_name,pw2_name,pw3_name,py1_name,py2_name,py3_name };
void* p_sounds[] = { pp1_sound,pp2_sound,pp3_sound,pw1_sound,pw2_sound,pw3_sound,py1_sound,py2_sound,py3_sound };

char wy1_name[] = "RedMist\\speech\\worker\\wyessr1.wav\x0";
void* wy1_sound = NULL;
char wy2_name[] = "RedMist\\speech\\worker\\wyessr2.wav\x0";
void* wy2_sound = NULL;
char wy3_name[] = "RedMist\\speech\\worker\\wyessr3.wav\x0";
void* wy3_sound = NULL;
char wy4_name[] = "RedMist\\speech\\worker\\wyessr4.wav\x0";
void* wy4_sound = NULL;
char ww1_name[] = "RedMist\\speech\\worker\\wwhat1.wav\x0";
void* ww1_sound = NULL;
char ww2_name[] = "RedMist\\speech\\worker\\wwhat2.wav\x0";
void* ww2_sound = NULL;
char ww3_name[] = "RedMist\\speech\\worker\\wwhat3.wav\x0";
void* ww3_sound = NULL;
char ww4_name[] = "RedMist\\speech\\worker\\wwhat4.wav\x0";
void* ww4_sound = NULL;
char wp1_name[] = "RedMist\\speech\\worker\\wpissd1.wav\x0";
void* wp1_sound = NULL;
char wp2_name[] = "RedMist\\speech\\worker\\wpissd2.wav\x0";
void* wp2_sound = NULL;
char wp3_name[] = "RedMist\\speech\\worker\\wpissd3.wav\x0";
void* wp3_sound = NULL;
char wp4_name[] = "RedMist\\speech\\worker\\wpissd4.wav\x0";
void* wp4_sound = NULL;
char wp5_name[] = "RedMist\\speech\\worker\\wpissd5.wav\x0";
void* wp5_sound = NULL;
char wp6_name[] = "RedMist\\speech\\worker\\wpissd6.wav\x0";
void* wp6_sound = NULL;
char wp7_name[] = "RedMist\\speech\\worker\\wpissd7.wav\x0";
void* wp7_sound = NULL;
char wr_name[] = "RedMist\\speech\\worker\\wready.wav\x0";
void* wr_sound = NULL;
char wd_name[] = "RedMist\\speech\\worker\\wrkdon.wav\x0";
void* wd_sound = NULL;
char wdn_name[] = "RedMist\\speech\\human\\wrkdone.wav\x0";
void* wdn_sound = NULL;
char* w_names[] = { wr_name,wp1_name,wp2_name,wp3_name,wp4_name,wp5_name,wp6_name,wp7_name,ww1_name,ww2_name,ww3_name,ww4_name,wy1_name,wy2_name,wy3_name,wy4_name,wd_name,wdn_name };
void* w_sounds[] = { wr_sound,wp1_sound,wp2_sound,wp3_sound,wp4_sound,wp5_sound,wp6_sound,wp7_sound,ww1_sound,ww2_sound,ww3_sound,ww4_sound,wy1_sound,wy2_sound,wy3_sound,wy4_sound,wd_sound,wdn_sound };
bool w_sounds_e = true;

char huy1_name[] = "RedMist\\speech\\human\\Hyessir1.wav\x0";
void* huy1_sound = NULL;
char huy2_name[] = "RedMist\\speech\\human\\Hyessir2.wav\x0";
void* huy2_sound = NULL;
char huy3_name[] = "RedMist\\speech\\human\\Hyessir3.wav\x0";
void* huy3_sound = NULL;
char huy4_name[] = "RedMist\\speech\\human\\Hyessir4.wav\x0";
void* huy4_sound = NULL;
char huw1_name[] = "RedMist\\speech\\human\\Hwhat1.wav\x0";
void* huw1_sound = NULL;
char huw2_name[] = "RedMist\\speech\\human\\Hwhat2.wav\x0";
void* huw2_sound = NULL;
char huw3_name[] = "RedMist\\speech\\human\\Hwhat3.wav\x0";
void* huw3_sound = NULL;
char huw4_name[] = "RedMist\\speech\\human\\Hwhat4.wav\x0";
void* huw4_sound = NULL;
char huw5_name[] = "RedMist\\speech\\human\\Hwhat5.wav\x0";
void* huw5_sound = NULL;
char huw6_name[] = "RedMist\\speech\\human\\Hwhat6.wav\x0";
void* huw6_sound = NULL;
char hup1_name[] = "RedMist\\speech\\human\\Hpissed1.wav\x0";
void* hup1_sound = NULL;
char hup2_name[] = "RedMist\\speech\\human\\Hpissed2.wav\x0";
void* hup2_sound = NULL;
char hup3_name[] = "RedMist\\speech\\human\\Hpissed3.wav\x0";
void* hup3_sound = NULL;
char hup4_name[] = "RedMist\\speech\\human\\Hpissed4.wav\x0";
void* hup4_sound = NULL;
char hup5_name[] = "RedMist\\speech\\human\\Hpissed5.wav\x0";
void* hup5_sound = NULL;
char hup6_name[] = "RedMist\\speech\\human\\Hpissed6.wav\x0";
void* hup6_sound = NULL;
char hup7_name[] = "RedMist\\speech\\human\\Hpissed7.wav\x0";
void* hup7_sound = NULL;
char hur_name[] = "RedMist\\speech\\human\\Hready.wav\x0";
void* hur_sound = NULL;
char* hu_names[] = { huw1_name,huw2_name,huw3_name,huw4_name,huw5_name,huw6_name,hup1_name,hup2_name,hup3_name,hup4_name,hup5_name,hup6_name,hup7_name,hup7_name,huy1_name,huy2_name,huy3_name,huy4_name,huy4_name,huy4_name,hur_name };
void* hu_sounds[] = { huw1_sound,huw2_sound,huw3_sound,huw4_sound,huw5_sound,huw6_sound,hup1_sound,hup2_sound,hup3_sound,hup4_sound,hup5_sound,hup6_sound,hup7_sound,hup7_sound,huy1_sound,huy2_sound,huy3_sound,huy4_sound,huy4_sound,huy4_sound,hur_sound };
bool hu_sounds_e = true;

char hh1_name[] = "RedMist\\speech\\human\\help1.wav\x0";
void* hh1_sound = NULL;
bool hh1_sounds_e = true;
char hh2_name[] = "RedMist\\speech\\human\\help2.wav\x0";
void* hh2_sound = NULL;
bool hh2_sounds_e = true;

char ky1_name[] = "RedMist\\speech\\knight\\Knyessr1.wav\x0";
void* ky1_sound = NULL;
char ky2_name[] = "RedMist\\speech\\knight\\Knyessr2.wav\x0";
void* ky2_sound = NULL;
char ky3_name[] = "RedMist\\speech\\knight\\Knyessr3.wav\x0";
void* ky3_sound = NULL;
char ky4_name[] = "RedMist\\speech\\knight\\Knyessr4.wav\x0";
void* ky4_sound = NULL;
char kw1_name[] = "RedMist\\speech\\knight\\Knwhat1.wav\x0";
void* kw1_sound = NULL;
char kw2_name[] = "RedMist\\speech\\knight\\Knwhat2.wav\x0";
void* kw2_sound = NULL;
char kw3_name[] = "RedMist\\speech\\knight\\Knwhat3.wav\x0";
void* kw3_sound = NULL;
char kw4_name[] = "RedMist\\speech\\knight\\Knwhat4.wav\x0";
void* kw4_sound = NULL;
char kp1_name[] = "RedMist\\speech\\knight\\Knpissd1.wav\x0";
void* kp1_sound = NULL;
char kp2_name[] = "RedMist\\speech\\knight\\Knpissd2.wav\x0";
void* kp2_sound = NULL;
char kp3_name[] = "RedMist\\speech\\knight\\Knpissd3.wav\x0";
void* kp3_sound = NULL;
char kr_name[] = "RedMist\\speech\\knight\\Knready.wav\x0";
void* kr_sound = NULL;
char* k_names[] = { kp1_name,kp2_name,kp3_name,kr_name,kw1_name,kw2_name,kw3_name,kw4_name,ky1_name,ky2_name,ky3_name,ky4_name };
void* k_sounds[] = { kp1_sound,kp2_sound,kp3_sound,kr_sound,kw1_sound,kw2_sound,kw3_sound,kw4_sound,ky1_sound,ky2_sound,ky3_sound,ky4_sound };
bool k_sounds_e = true;

char pky1_name[] = "RedMist\\speech\\paladin\\Pkyessr1.wav\x0";
void* pky1_sound = NULL;
char pky2_name[] = "RedMist\\speech\\paladin\\Pkyessr2.wav\x0";
void* pky2_sound = NULL;
char pky3_name[] = "RedMist\\speech\\paladin\\Pkyessr3.wav\x0";
void* pky3_sound = NULL;
char pky4_name[] = "RedMist\\speech\\paladin\\Pkyessr4.wav\x0";
void* pky4_sound = NULL;
char pkw1_name[] = "RedMist\\speech\\paladin\\Pkwhat1.wav\x0";
void* pkw1_sound = NULL;
char pkw2_name[] = "RedMist\\speech\\paladin\\Pkwhat2.wav\x0";
void* pkw2_sound = NULL;
char pkw3_name[] = "RedMist\\speech\\paladin\\Pkwhat3.wav\x0";
void* pkw3_sound = NULL;
char pkw4_name[] = "RedMist\\speech\\paladin\\Pkwhat4.wav\x0";
void* pkw4_sound = NULL;
char pkp1_name[] = "RedMist\\speech\\paladin\\Pkpissd1.wav\x0";
void* pkp1_sound = NULL;
char pkp2_name[] = "RedMist\\speech\\paladin\\Pkpissd2.wav\x0";
void* pkp2_sound = NULL;
char pkp3_name[] = "RedMist\\speech\\paladin\\Pkpissd3.wav\x0";
void* pkp3_sound = NULL;
char pkr_name[] = "RedMist\\speech\\paladin\\Pkready.wav\x0";
void* pkr_sound = NULL;
char* pk_names[] = { pkp1_name,pkp2_name,pkp3_name,pkr_name,pkw1_name,pkw2_name,pkw3_name,pkw4_name,pky1_name,pky2_name,pky3_name,pky4_name };
void* pk_sounds[] = { pkp1_sound,pkp2_sound,pkp3_sound,pkr_sound,pkw1_sound,pkw2_sound,pkw3_sound,pkw4_sound,pky1_sound,pky2_sound,pky3_sound,pky4_sound };
bool pk_sounds_e = true;

char ey1_name[] = "RedMist\\speech\\elves\\Eyessir1.wav\x0";
void* ey1_sound = NULL;
char ey2_name[] = "RedMist\\speech\\elves\\Eyessir2.wav\x0";
void* ey2_sound = NULL;
char ey3_name[] = "RedMist\\speech\\elves\\Eyessir3.wav\x0";
void* ey3_sound = NULL;
char ey4_name[] = "RedMist\\speech\\elves\\Eyessir4.wav\x0";
void* ey4_sound = NULL;
char ew1_name[] = "RedMist\\speech\\elves\\Ewhat1.wav\x0";
void* ew1_sound = NULL;
char ew2_name[] = "RedMist\\speech\\elves\\Ewhat2.wav\x0";
void* ew2_sound = NULL;
char ew3_name[] = "RedMist\\speech\\elves\\Ewhat3.wav\x0";
void* ew3_sound = NULL;
char ew4_name[] = "RedMist\\speech\\elves\\Ewhat4.wav\x0";
void* ew4_sound = NULL;
char ep1_name[] = "RedMist\\speech\\elves\\Epissed1.wav\x0";
void* ep1_sound = NULL;
char ep2_name[] = "RedMist\\speech\\elves\\Epissed2.wav\x0";
void* ep2_sound = NULL;
char ep3_name[] = "RedMist\\speech\\elves\\Epissed3.wav\x0";
void* ep3_sound = NULL;
char er_name[] = "RedMist\\speech\\elves\\Eready.wav\x0";
void* er_sound = NULL;
char* e_names[] = { ep1_name,ep2_name,ep3_name,er_name,ew1_name,ew2_name,ew3_name,ew4_name,ey1_name,ey2_name,ey3_name,ey4_name };
void* e_sounds[] = { ep1_sound,ep2_sound,ep3_sound,er_sound,ew1_sound,ew2_sound,ew3_sound,ew4_sound,ey1_sound,ey2_sound,ey3_sound,ey4_sound };
bool e_sounds_e = true;

char wzy1_name[] = "RedMist\\speech\\wizard\\Wzyessr1.wav\x0";
void* wzy1_sound = NULL;
char wzy2_name[] = "RedMist\\speech\\wizard\\Wzyessr2.wav\x0";
void* wzy2_sound = NULL;
char wzy3_name[] = "RedMist\\speech\\wizard\\Wzyessr3.wav\x0";
void* wzy3_sound = NULL;
char wzw1_name[] = "RedMist\\speech\\wizard\\Wzwhat1.wav\x0";
void* wzw1_sound = NULL;
char wzw2_name[] = "RedMist\\speech\\wizard\\Wzwhat2.wav\x0";
void* wzw2_sound = NULL;
char wzw3_name[] = "RedMist\\speech\\wizard\\Wzwhat3.wav\x0";
void* wzw3_sound = NULL;
char wzp1_name[] = "RedMist\\speech\\wizard\\Wzpissd1.wav\x0";
void* wzp1_sound = NULL;
char wzp2_name[] = "RedMist\\speech\\wizard\\Wzpissd2.wav\x0";
void* wzp2_sound = NULL;
char wzp3_name[] = "RedMist\\speech\\wizard\\Wzpissd3.wav\x0";
void* wzp3_sound = NULL;
char wzr_name[] = "RedMist\\speech\\wizard\\Wzready.wav\x0";
void* wzr_sound = NULL;
char* wz_names[] = { wzp1_name,wzp2_name,wzp3_name,wzr_name,wzw1_name,wzw2_name,wzw3_name,wzy1_name,wzy2_name,wzy3_name };
void* wz_sounds[] = { wzp1_sound,wzp2_sound,wzp3_sound,wzr_sound,wzw1_sound,wzw2_sound,wzw3_sound,wzy1_sound,wzy2_sound,wzy3_sound };
bool wz_sounds_e = true;

char sy1_name[] = "RedMist\\speech\\ships\\Hshpyes1.wav\x0";
void* sy1_sound = NULL;
char sy2_name[] = "RedMist\\speech\\ships\\Hshpyes2.wav\x0";
void* sy2_sound = NULL;
char sy3_name[] = "RedMist\\speech\\ships\\Hshpyes3.wav\x0";
void* sy3_sound = NULL;
char sw1_name[] = "RedMist\\speech\\ships\\Hshpwht1.wav\x0";
void* sw1_sound = NULL;
char sw2_name[] = "RedMist\\speech\\ships\\Hshpwht2.wav\x0";
void* sw2_sound = NULL;
char sw3_name[] = "RedMist\\speech\\ships\\Hshpwht3.wav\x0";
void* sw3_sound = NULL;
char sp1_name[] = "RedMist\\speech\\ships\\Hshppis1.wav\x0";
void* sp1_sound = NULL;
char sp2_name[] = "RedMist\\speech\\ships\\Hshppis2.wav\x0";
void* sp2_sound = NULL;
char sp3_name[] = "RedMist\\speech\\ships\\Hshppis3.wav\x0";
void* sp3_sound = NULL;
char sr_name[] = "RedMist\\speech\\ships\\Hshpredy.wav\x0";
void* sr_sound = NULL;
char* s_names[] = { sp1_name,sp2_name,sp3_name,sr_name,sw1_name,sw2_name,sw3_name,sy1_name,sy2_name,sy3_name };
void* s_sounds[] = { sp1_sound,sp2_sound,sp3_sound,sr_sound,sw1_sound,sw2_sound,sw3_sound,sy1_sound,sy2_sound,sy3_sound };
bool s_sounds_e = true;

char dy1_name[] = "RedMist\\speech\\dwarf\\Dwyessr1.wav\x0";
void* dy1_sound = NULL;
char dy2_name[] = "RedMist\\speech\\dwarf\\Dwyessr2.wav\x0";
void* dy2_sound = NULL;
char dy3_name[] = "RedMist\\speech\\dwarf\\Dwyessr3.wav\x0";
void* dy3_sound = NULL;
char dy4_name[] = "RedMist\\speech\\dwarf\\Dwyessr4.wav\x0";
void* dy4_sound = NULL;
char dy5_name[] = "RedMist\\speech\\dwarf\\Dwyessr5.wav\x0";
void* dy5_sound = NULL;
char dw1_name[] = "RedMist\\speech\\dwarf\\Dwhat1.wav\x0";
void* dw1_sound = NULL;
char dw2_name[] = "RedMist\\speech\\dwarf\\Dwhat2.wav\x0";
void* dw2_sound = NULL;
char dp1_name[] = "RedMist\\speech\\dwarf\\Dwpissd1.wav\x0";
void* dp1_sound = NULL;
char dp2_name[] = "RedMist\\speech\\dwarf\\Dwpissd2.wav\x0";
void* dp2_sound = NULL;
char dp3_name[] = "RedMist\\speech\\dwarf\\Dwpissd3.wav\x0";
void* dp3_sound = NULL;
char dr_name[] = "RedMist\\speech\\dwarf\\Dwready.wav\x0";
void* dr_sound = NULL;
char* d_names[] = { dp1_name,dp2_name,dp3_name,dr_name,dw1_name,dw2_name,dy1_name,dy2_name,dy3_name,dy4_name,dy5_name };
void* d_sounds[] = { dp1_sound,dp2_sound,dp3_sound,dr_sound,dw1_sound,dw2_sound,dy1_sound,dy2_sound,dy3_sound,dy4_sound,dy5_sound };
bool d_sounds_e = true;

char gy1_name[] = "RedMist\\speech\\gnome\\Gnyessr1.wav\x0";
void* gy1_sound = NULL;
char gp1_name[] = "RedMist\\speech\\gnome\\Gnpissd1.wav\x0";
void* gp1_sound = NULL;
char gp2_name[] = "RedMist\\speech\\gnome\\Gnpissd2.wav\x0";
void* gp2_sound = NULL;
char gp3_name[] = "RedMist\\speech\\gnome\\Gnpissd3.wav\x0";
void* gp3_sound = NULL;
char gp4_name[] = "RedMist\\speech\\gnome\\Gnpissd4.wav\x0";
void* gp4_sound = NULL;
char gp5_name[] = "RedMist\\speech\\gnome\\Gnpissd5.wav\x0";
void* gp5_sound = NULL;
char gr_name[] = "RedMist\\speech\\gnome\\Gnready.wav\x0";
void* gr_sound = NULL;
char* g_names[] = { gp1_name,gp2_name,gp3_name,gp4_name,gp5_name,gr_name,gy1_name };
void* g_sounds[] = { gp1_sound,gp2_sound,gp3_sound,gp4_sound,gp5_sound,gr_sound,gy1_sound,};
bool g_sounds_e = true;

void sound_play_from_file(int oid, DWORD name, void* snd)//old id
{
    def_name = (void*)*(int*)(SOUNDS_FILES_LIST + 8 + 24 * oid);
    def_sound = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * oid);//save default
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)name);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)snd);
    ((void (*)(WORD))F_WAR2_SOUND_PLAY)(oid);//war2 sound play
    snd = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * oid);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)def_sound);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)def_name);//restore default
}
//-----sound

void show_message(byte time, char* text)
{
    ((void (*)(char*, int, int))F_MAP_MSG)(text, 15, time * 10);//original war2 show msg func
}

int get_val(int adress, int player)
{
    return (int)(*(WORD*)(adress + player * 2));//player*2 cause all vals is WORD
}

bool cmp_args(byte m, byte v, byte c)
{//compare bytes
    bool f = false;
    switch (m)
    {
    case CMP_EQ:f = (v == c); break;
    case CMP_NEQ:f = (v != c); break;
    case CMP_BIGGER_EQ:f = (v >= c); break;
    case CMP_SMALLER_EQ:f = (v <= c); break;
    case CMP_BIGGER:f = (v > c); break;
    case CMP_SMALLER:f = (v < c); break;
    default: f = false; break;
    }
    return f;
}

bool cmp_args2(byte m, WORD v, WORD c)
{//compare words
    bool f = false;
    switch (m)
    {
    case CMP_EQ:f = (v == c); break;
    case CMP_NEQ:f = (v != c); break;
    case CMP_BIGGER_EQ:f = (v >= c); break;
    case CMP_SMALLER_EQ:f = (v <= c); break;
    case CMP_BIGGER:f = (v > c); break;
    case CMP_SMALLER:f = (v < c); break;
    default: f = false; break;
    }
    return f;
}

bool cmp_args4(byte m, int v, int c)
{//comapre 4 bytes (for resources)
    bool f = false;
    switch (m)
    {
    case CMP_EQ:f = (v == c); break;
    case CMP_NEQ:f = (v != c); break;
    case CMP_BIGGER_EQ:f = (v >= c); break;
    case CMP_SMALLER_EQ:f = (v <= c); break;
    case CMP_BIGGER:f = (v > c); break;
    case CMP_SMALLER:f = (v < c); break;
    default: f = false; break;
    }
    return f;
}

void lose(bool t)
{
    game_started = false;
    if (t == true)
    {
        char buf[] = "\x0";//if need to show table
        PATCH_SET((char*)LOSE_SHOW_TABLE, buf);
    }
    else
    {
        char buf[] = "\x3b";
        PATCH_SET((char*)LOSE_SHOW_TABLE, buf);
    }
	if (!first_step)
	{
		char l[] = "\x2";
        PATCH_SET((char*)(ENDGAME_STATE + (*(byte*)LOCAL_PLAYER)), l);
		((void (*)())F_LOSE)();//original lose func
	}
	else
	{
		patch_setdword((DWORD*)VICTORY_FUNCTION, (DWORD)F_LOSE);
	}
}

void win(bool t)
{
    game_started = false;
    if (t == true)
    {
        char buf[] = "\xEB";//if need to show table
        PATCH_SET((char*)WIN_SHOW_TABLE, buf);
    }
    else
    {
        char buf[] = "\x74";
        PATCH_SET((char*)WIN_SHOW_TABLE, buf);
    }
	if (!first_step)
	{
		char l[] = "\x3";
        PATCH_SET((char*)(ENDGAME_STATE + (*(byte*)LOCAL_PLAYER)), l);
		((void (*)())F_WIN)();//original win func
	}
	else
	{
		patch_setdword((DWORD*)VICTORY_FUNCTION, (DWORD)F_WIN);
	}
}

void tile_remove_trees(int x, int y)
{
    ((void (*)(int, int))F_TILE_REMOVE_TREES)(x, y);
}

void tile_remove_rocks(int x, int y)
{
    ((void (*)(int, int))F_TILE_REMOVE_ROCKS)(x, y);
}

void tile_remove_walls(int x, int y)
{
    ((void (*)(int, int))F_TILE_REMOVE_WALLS)(x, y);
}

bool stat_byte(byte s)
{//chech if unit stat is 1 or 2 byte
    bool f = (s == S_DRAW_X) || (s == S_DRAW_Y) || (s == S_X) || (s == S_Y) || (s == S_HP)
        || (s == S_INVIZ) || (s == S_SHIELD) || (s == S_BLOOD) || (s == S_HASTE)
        || (s == S_AI_SPELLS) || (s == S_NEXT_FIRE)
        || (s == S_LAST_HARVEST_X) || (s == S_LAST_HARVEST_Y)
        || (s == S_BUILD_PROGRES) || (s == S_BUILD_PROGRES_TOTAL)
        || (s == S_RESOURCES) || (s == S_ORDER_X) || (s == S_ORDER_Y)
        || (s == S_RETARGET_X1) || (s == S_RETARGET_Y1) || (s == S_RETARGET_X2) || (s == S_RETARGET_Y2);
    return !f;
}

bool cmp_stat(int* p, int v, byte pr, byte cmp)
{
    //p - unit
    //v - value
    //pr - property
    //cmp - compare method
    bool f = false;
    if (stat_byte(pr))
    {
        byte ob = v % 256;
        char buf[3] = { 0 };
        buf[0] = ob;
        buf[1] = *((byte*)((uintptr_t)p + pr));
        if (cmp_args(cmp, buf[1], buf[0]))
        {
            f = true;
        }
    }
    else
    {
        if (cmp_args2(cmp, *((WORD*)((uintptr_t)p + pr)), (WORD)v))
        {
            f = true;
        }
    }
    return f;
}

void set_stat(int* p, int v, byte pr)
{
    if (stat_byte(pr))
    {
        char buf[] = "\x0";
        buf[0] = v % 256;
        PATCH_SET((char*)((uintptr_t)p + pr), buf);
    }
    else
    {
        char buf[] = "\x0\x0";
        buf[0] = v % 256;
        buf[1] = (v / 256) % 256;
        PATCH_SET((char*)((uintptr_t)p + pr), buf);
    }
}

void unit_convert(byte player, int who, int tounit, int a)
{
    //original war2 func converts units
    ((void (*)(byte, int, int, int))F_UNIT_CONVERT)(player, who, tounit, a);
}

void unit_create(int x, int y, int id, byte owner, byte n)
{
    while (n > 0)
    {
        n -= 1;
        int* p = (int*)(UNIT_SIZE_TABLE + 4 * id);//unit sizes table
        ((void (*)(int, int, int, byte, int*))F_UNIT_CREATE)(x, y, id, owner, p);//original war2 func creates unit
        //just call n times to create n units
    }
}

void unit_kill(int* u)
{
    ((void (*)(int*))F_UNIT_KILL)(u);//original war2 func kills unit
}

void unit_remove(int* u)
{
    byte f = *((byte*)((uintptr_t)u + S_FLAGS3));
    f |= SF_HIDDEN;
    set_stat(u, f, S_FLAGS3);
    unit_kill(u);//hide unit then kill
}

void unit_cast(int* u)//unit autocast
{
    ((void (*)(int*))F_AI_CAST)(u);//original war2 ai cast spells
}

int* bullet_create(WORD x, WORD y, byte id)
{
    int* b = ((int* (*)(WORD, WORD, byte))F_BULLET_CREATE)(x, y, id);
    if (b)
    {
        if ((id == B_LIGHT_FIRE) || (id == B_HEAVY_FIRE))
        {
            char buf[] = "\x0";
            buf[0] = 5;//bullet action
            PATCH_SET((char*)((uintptr_t)b + 54), buf);//54 bullet action
            buf[0] = 1;//bullet info
            PATCH_SET((char*)((uintptr_t)b + 58), buf);//58 bullet user info
            buf[0] = 4;//bullet flags
            PATCH_SET((char*)((uintptr_t)b + 53), buf);//53 bullet flags
            buf[0] = 80;//ticks
            PATCH_SET((char*)((uintptr_t)b + 56), buf);//56 bullet life (WORD)
        }
    }
    return b;
}

void bullet_create_unit(int* u, byte b)
{
    WORD x = *((WORD*)((uintptr_t)u + S_DRAW_X));
    WORD y = *((WORD*)((uintptr_t)u + S_DRAW_Y));
    bullet_create(x + 16, y + 16, b);
}

void bullet_create8_around_unit(int* u, byte b)
{
    WORD ux = *((WORD*)((uintptr_t)u + S_DRAW_X));
    WORD uy = *((WORD*)((uintptr_t)u + S_DRAW_Y));
    WORD x = ux + 16;
    WORD y = uy + 16;
    if ((b == B_LIGHT_FIRE) || (b == B_HEAVY_FIRE))y -= 8;
    if ((b == B_LIGHTNING) || (b == B_COIL))
    {
        x += 16;
        y += 16;
    }
    WORD xx = x;
    WORD yy = y;
    bullet_create(xx + 48, yy, b);//right
    bullet_create(xx, yy + 48, b);//down
    bullet_create(xx + 32, yy + 32, b);//right down
    if (xx <= 32)xx = 0;
    else xx -= 32;
    bullet_create(xx, yy + 32, b);//left down
    if (yy <= 32)yy = 0;
    else yy -= 32;
    bullet_create(xx, yy, b);//left up
    xx = x;
    bullet_create(xx + 32, yy, b);//right up
    yy = y;
    if (xx <= 48)xx = 0;
    else xx -= 48;
    bullet_create(xx, yy, b);//left
    xx = x;
    if (yy <= 48)yy = 0;
    else yy -= 48;
    bullet_create(xx, yy, b);//up
}

void set_region(int x1, int y1, int x2, int y2)
{
    if (x1 < 0)x1 = 0;
    if (x1 > 127)x1 = 127;
    if (y1 < 0)y1 = 0;
    if (y1 > 127)y1 = 127;
    if (x2 < 0)x2 = 0;
    if (x2 > 127)x2 = 127;
    if (y2 < 0)y2 = 0;
    if (y2 > 127)y2 = 127;
    reg[0] = x1 % 256;
    reg[1] = y1 % 256;
    reg[2] = x2 % 256;
    reg[3] = y2 % 256;
}

bool in_region(byte x, byte y, byte x1, byte y1, byte x2, byte y2)
{
    //dnt know why but without this big monstrous ussless code gam crash 
    byte tmp;
    tmp = x % 256;
    x = tmp;
    tmp = y % 256;
    y = tmp;
    tmp = x1 % 256;
    x1 = tmp;
    tmp = y1 % 256;
    y1 = tmp;
    tmp = x2 % 256;
    x2 = tmp;
    tmp = y2 % 256;
    y2 = tmp;
    if (x < 0)x = 0;
    if (x > 127)x = 127;
    if (y < 0)y = 0;
    if (y > 127)y = 127;
    if (x1 < 0)x1 = 0;
    if (x1 > 127)x1 = 127;
    if (y1 < 0)y1 = 0;
    if (y1 > 127)y1 = 127;
    if (x2 < 0)x2 = 0;
    if (x2 > 127)x2 = 127;
    if (y2 < 0)y2 = 0;
    if (y2 > 127)y2 = 127;
    if (x2 < x1)
    {
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    if (y2 < y1)
    {
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    //just check if coords inside region
    return ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2));
}

bool check_unit_dead(int* p)
{
    bool dead = false;
    if (p)
    {
        if ((*((byte*)((uintptr_t)p + S_FLAGS3))
            & (SF_DEAD | SF_DIEING | SF_UNIT_FREE)) != 0)
            dead = true;
    }
    else
        dead = true;
    return dead;
}

bool check_unit_complete(int* p)//for buildings
{
    bool f = false;
    if (p)
    {
        if ((*((byte*)((uintptr_t)p + S_FLAGS3)) & SF_COMPLETED) != 0)//flags3 last bit
            f = true;
    }
    else
        f = false;
    return f;
}

bool check_unit_hidden(int* p)
{
    bool f = false;
    if (p)
    {
        if ((*((byte*)((uintptr_t)p + S_FLAGS3)) & SF_HIDDEN) != 0)//flags3 4 bit
            f = true;
    }
    else
        f = true;
    return f;
}

bool check_unit_preplaced(int* p)
{
    bool f = false;
    if (p)
    {
        if ((*((byte*)((uintptr_t)p + S_FLAGS3)) & SF_PREPLACED) != 0)//flags3
            f = true;
    }
    else
        f = false;
    return f;
}

bool check_unit_near_death(int* p)
{
    bool dead = false;
    if (p)
    {
        if (((*((byte*)((uintptr_t)p + S_FLAGS3)) & SF_DIEING) != 0)
            && ((*((byte*)((uintptr_t)p + S_FLAGS3)) & (SF_DEAD | SF_UNIT_FREE)) == 0))
            dead = true;
    }
    else
        dead = true;
    return dead;
}

bool check_peon_loaded(int* p, byte r)
{
    bool f = false;
    if (p)
    {
        if (r == 0)
        {
            if (((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_LOADED) != 0)
                && ((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_HARVEST_GOLD) != 0))
                f = true;
        }
        if (r == 1)
        {
            if (((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_LOADED) != 0)
                && ((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_HARVEST_LUMBER) != 0))
                f = true;
        }
        if (r == 2)
        {
            if (((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_LOADED) != 0))
                f = true;
        }
    }
    return f;
}

void find_all_units(byte id)
{
	//CAREFUL with this function - ALL units get into massive 
    //even if their memory was cleared already
    //all units by id will go in array
    units = 0;
    int* p = (int*)UNITS_MASSIVE;//pointer to units
    p = (int*)(*p);
    int k = *(int*)UNITS_NUMBER;
    while (k > 0)
    {
        bool f = *((byte*)((uintptr_t)p + S_ID)) == (byte)id;
        if (f)
        {
            unit[units] = p;
            units++;
        }
        p = (int*)((int)p + 0x98);
        k--;
    }
}

void find_all_alive_units(byte id)
{
    //all units by id will go in array
    units = 0;
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);//pointer to units list for each player
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = *((byte*)((uintptr_t)p + S_ID)) == (byte)id;
                if (id == ANY_BUILDING)
                    f = *((byte*)((uintptr_t)p + S_ID)) >= U_FARM;//buildings
                if (id == ANY_MEN)
                    f = *((byte*)((uintptr_t)p + S_ID)) < U_FARM;//all nonbuildings
                if (id == ANY_UNITS)
                    f = true;//all ALL units
                if (id == ANY_BUILDING_2x2)//small buildings
                {
                    byte sz = *((byte*)UNIT_SIZE_TABLE + *((byte*)((uintptr_t)p + S_ID)) * 4);
                    f = sz == 2;
                }
                if (id == ANY_BUILDING_3x3)//med buildings
                {
                    byte sz = *((byte*)UNIT_SIZE_TABLE + *((byte*)((uintptr_t)p + S_ID)) * 4);
                    f = sz == 3;
                }
                if (id == ANY_BUILDING_4x4)//big buildings
                {
                    byte sz = *((byte*)UNIT_SIZE_TABLE + *((byte*)((uintptr_t)p + S_ID)) * 4);
                    f = sz == 4;
                }
                if (f)
                {
                    if (!check_unit_dead(p))
                    {
                        unit[units] = p;
                        units++;
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void sort_complete()
{
    //only completed units stay in array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (check_unit_complete(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_in_region()
{
    //only units in region stay in array
    int k = 0;
    WORD x = 0, y = 0;
    for (int i = 0; i < units; i++)
    {
        x = *((WORD*)((uintptr_t)unit[i] + S_DRAW_X)) / 32;
        y = *((WORD*)((uintptr_t)unit[i] + S_DRAW_Y)) / 32;
        if (in_region((byte)x, (byte)y, reg[0], reg[1], reg[2], reg[3]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_not_in_region()
{
    //only units not in region stay in array
    int k = 0;
    WORD x = 0, y = 0;
    for (int i = 0; i < units; i++)
    {
        x = *((WORD*)((uintptr_t)unit[i] + S_DRAW_X)) / 32;
        y = *((WORD*)((uintptr_t)unit[i] + S_DRAW_Y)) / 32;
        if (!in_region((byte)x, (byte)y, reg[0], reg[1], reg[2], reg[3]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_target_in_region()
{
    //only units that have order coords in region stay in array
    int k = 0;
    byte x = 0, y = 0;
    for (int i = 0; i < units; i++)
    {
        x = *((byte*)((uintptr_t)unit[i] + S_ORDER_X));
        y = *((byte*)((uintptr_t)unit[i] + S_ORDER_Y));
        if (in_region(x, y, reg[0], reg[1], reg[2], reg[3]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_stat(byte pr, int v, byte cmp)
{
    //only units stay in array if have property compared to value is true
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (cmp_stat(unit[i], v, pr, cmp))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_hidden()
{
    //only not hidden units stay in array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!check_unit_hidden(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_self(int* u)
{
    //unit remove self from array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!(unit[i] == u))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_full_hp()
{
    //if hp not full
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        byte id = *((byte*)((uintptr_t)unit[i] + S_ID));//unit id
        WORD mhp = *(WORD*)(UNIT_HP_TABLE + 2 * id);//max hp
        WORD hp = *((WORD*)((uintptr_t)unit[i] + S_HP));//unit hp
        if (hp < mhp)//hp not full
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_fleshy()
{
    //only fleshy units stay in array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        byte id = *((byte*)((uintptr_t)unit[i] + S_ID));//unit id
        if ((*(int*)(UNIT_GLOBAL_FLAGS + id * 4) & IS_FLESHY) != 0)//fleshy global flag
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_order_hp()
{
    //order array by hp from low to high
    for (int i = 0; i < units; i++)
    {
        int sm = i;
        for (int j = i + 1; j < units; j++)
        {
            WORD hpsm = *((WORD*)((uintptr_t)unit[sm] + S_HP));//unit hp
            WORD hpj = *((WORD*)((uintptr_t)unit[j] + S_HP));//unit hp
            if (hpj < hpsm)
            {
                sm = j;
            }
        }
        int* tmp = unit[i];
        unit[i] = unit[sm];
        unit[sm] = tmp;
    }
}

void sort_preplaced()
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!check_unit_preplaced(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_near_death()
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (check_unit_near_death(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_attack_can_hit(int* p)
{
    //only units stay in array that *p can attack them
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        int a = 0;
        a = ((int(*)(int*, int*))F_ATTACK_CAN_HIT)(p, unit[i]);//attack can hit original war2 function
        if (a != 0)
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_attack_can_hit_range(int* p)
{
    //only units stay in array that *p can attack them and have passable terrain in attack range
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        int a = 0;
        a = ((int(*)(int*, int*))F_ATTACK_CAN_HIT)(p, unit[i]);//attack can hit
        if (a != 0)
        {
            byte id = *((byte*)((uintptr_t)unit[i] + S_ID));
            byte szx = *(byte*)(UNIT_SIZE_TABLE + 4 * id);
            byte szy = *(byte*)(UNIT_SIZE_TABLE + 4 * id + 2);
            byte idd = *((byte*)((uintptr_t)p + S_ID));
            byte rng = *(byte*)(UNIT_RANGE_TABLE + idd);
            byte ms = *(byte*)MAP_SIZE;
            byte xx = *((byte*)((uintptr_t)unit[i] + S_X));
            byte yy = *((byte*)((uintptr_t)unit[i] + S_Y));
            if (xx < rng)xx = 0;
            else xx -= rng;
            if (yy < rng)yy = 0;
            else yy -= rng;
            byte cl = *((byte*)((uintptr_t)p + S_MOVEMENT_TYPE));//movement type
            WORD mt = *(WORD*)(GLOBAL_MOVEMENT_TERRAIN_FLAGS + 2 * cl);//movement terrain flags

            bool f = false;
            for (int x = xx; (x < szx + xx + rng * 2 + 1) && (x < 127); x++)
            {
                for (int y = yy; (y < szy + yy + rng * 2 + 1) && (x < 127); y++)
                {
                    int aa = 1;
                    if ((cl == 0) || (cl == 3))//land and docked transport
                    {
                        aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);//original war2 func if terrain passable with that movement type
                    }
                    if ((x % 2 == 0) && (y % 2 == 0))//air and water
                    {
                        if ((cl == 1) || (cl == 2))
                        {
                            aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);
                        }
                    }
                    if (aa == 0)f = true;
                }
            }
            if (f)
            {
                unitt[k] = unit[i];
                k++;
            }
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_rune_near()
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        byte x = *((byte*)((uintptr_t)unit[i] + S_X));
        byte y = *((byte*)((uintptr_t)unit[i] + S_Y));
        bool f = false;
        for (int r = 0; r < 50; r++)//max runes 50
        {
            WORD d = *(WORD*)(RUNEMAP_TIMERS + 2 * r);
            if (d != 0)
            {
                byte xx = *(byte*)(RUNEMAP_X + r);
                byte yy = *(byte*)(RUNEMAP_Y + r);
                if (xx == x)
                {
                    if (yy > y)
                    {
                        if ((yy - y) == 1)f = true;
                    }
                    else
                    {
                        if ((y - yy) == 1)f = true;
                    }
                }
                if (yy == y)
                {
                    if (xx > x)
                    {
                        if ((xx - x) == 1)f = true;
                    }
                    else
                    {
                        if ((x - xx) == 1)f = true;
                    }
                }
            }
        }
        if (!f)
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_peon_loaded(byte r)
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (check_peon_loaded(unit[i], r))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_peon_not_loaded(byte r)
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!check_peon_loaded(unit[i], r))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void set_stat_all(byte pr, int v)
{
    for (int i = 0; i < units; i++)
    {
        set_stat(unit[i], v, pr);//set stat to all units in array
    }
}

void kill_all()
{
    for (int i = 0; i < units; i++)
    {
        unit_kill(unit[i]);//just kill all in array
    }
    units = 0;
}

void remove_all()
{
    for (int i = 0; i < units; i++)
    {
        unit_remove(unit[i]);//just kill all in array
    }
    units = 0;
}

void cast_all()
{
    for (int i = 0; i < units; i++)
    {
        unit_cast(unit[i]);//casting spells
    }
    units = 0;
}

void damag(int* p, byte n1, byte n2)
{
    WORD hp = *((WORD*)((uintptr_t)p + S_HP));//unit hp
    WORD n = n1 + 256 * n2;
    if (hp > n)
    {
        hp -= n;
        set_stat(p, hp, S_HP);
    }
    else
    {
        set_stat(p, 0, S_HP);
        unit_kill(p);
    }
}

void damag_all(byte n1, byte n2)
{
    for (int i = 0; i < units; i++)
    {
        damag(unit[i], n1, n2);
    }
}

void heal(int* p, byte n1, byte n2)
{
    byte id = *((byte*)((uintptr_t)p + S_ID));//unit id
    WORD mhp = *(WORD*)(UNIT_HP_TABLE + 2 * id);//max hp
    WORD hp = *((WORD*)((uintptr_t)p + S_HP));//unit hp
    WORD n = n1 + 256 * n2;
    if (hp < mhp)
    {
        hp += n;
        if (hp > mhp)
            hp = mhp;//canot heal more than max hp
        set_stat(p, hp, S_HP);
    }
}

void heal_all(byte n1, byte n2)
{
    for (int i = 0; i < units; i++)
    {
        heal(unit[i], n1, n2);
    }
}

void peon_load(int* u, byte r)
{
    byte f = *((byte*)((uintptr_t)u + S_PEON_FLAGS));
    if (!(f & PEON_LOADED))
    {
        if (r == 0)
        {
            f |= PEON_LOADED;
            f |= PEON_HARVEST_GOLD;
            set_stat(u, f, S_PEON_FLAGS);
            ((void (*)(int*))F_GROUP_SET)(u);
        }
        else
        {
            f |= PEON_LOADED;
            f |= PEON_HARVEST_LUMBER;
            set_stat(u, f, S_PEON_FLAGS);
            ((void (*)(int*))F_GROUP_SET)(u);
        }
    }
}

void peon_load_all(byte r)
{
    for (int i = 0; i < units; i++)
    {
        peon_load(unit[i], r);
    }
}

void viz_area(byte x, byte y, byte pl, byte sz)
{
    int Vf = F_VISION2;
    switch (sz)
    {
    case 0:Vf = F_VISION2; break;
    case 1:Vf = F_VISION2; break;
    case 2:Vf = F_VISION2; break;
    case 3:Vf = F_VISION3; break;
    case 4:Vf = F_VISION4; break;
    case 5:Vf = F_VISION5; break;
    case 6:Vf = F_VISION6; break;
    case 7:Vf = F_VISION7; break;
    case 8:Vf = F_VISION8; break;
    case 9:Vf = F_VISION9; break;
    default: Vf = F_VISION2; break;
    }
    for (byte i = 0; i < 8; i++)
    {
        if (((1 << i) & pl) != 0)
        {
            ((void (*)(WORD, WORD, byte))Vf)(x, y, i);
        }
    }
}

void viz_area_add(byte x, byte y, byte pl, byte sz)
{
    if ((vizs_n >= 0) && (vizs_n <= 255))
    {
        vizs_areas[vizs_n].x = x;
        vizs_areas[vizs_n].y = y;
        vizs_areas[vizs_n].p = pl;
        vizs_areas[vizs_n].s = sz;
        vizs_n++;
    }
}

void viz_area_all(byte pl, byte sz)
{
    for (int i = 0; i < units; i++)
    {
        byte x = *((byte*)((uintptr_t)unit[i] + S_X));
        byte y = *((byte*)((uintptr_t)unit[i] + S_Y));
        viz_area_add(x, y, pl, sz);
    }
}

void give(int* p, byte owner)
{
    ((void (*)(int*, byte, byte))F_CAPTURE)(p, owner, 1);//original capture unit war2 func
    *(byte*)(RESCUED_UNITS + 2 * owner) -= 1;//reset number of captured units
}

void give_all(byte o)
{
    for (int i = 0; i < units; i++)
    {
        give(unit[i], o);
    }
}

bool unit_move(byte x, byte y, int* unit)
{
    if (x < 0)return false;
    if (y < 0)return false;//canot go negative
    byte mxs = *(byte*)MAP_SIZE;//map size
    if (x >= mxs)return false;
    if (y >= mxs)return false;//canot go outside map
    if (check_unit_hidden(unit))return false;//if unit not hidden
    byte cl = *((byte*)((uintptr_t)unit + S_MOVEMENT_TYPE));//movement type
    WORD mt = *(WORD*)(GLOBAL_MOVEMENT_TERRAIN_FLAGS + 2 * cl);//movement terrain flags

    int aa = 1;
    if ((cl == 0) || (cl == 3))//land and docked transport
    {
        aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);//original war2 func if terrain passable with that movement type
    }
    if ((x % 2 == 0) && (y % 2 == 0))//air and water
    {
        if ((cl == 1) || (cl == 2))
        {
            aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);
        }
    }
    if (aa == 0)
    {
        ((void (*)(int*))F_UNIT_UNPLACE)(unit);//unplace
        set_stat(unit, x, S_X);
        set_stat(unit, y, S_Y);//change real coords
        set_stat(unit, x * 32, S_DRAW_X);
        set_stat(unit, y * 32, S_DRAW_Y);//change draw sprite coords
        ((void (*)(int*))F_UNIT_PLACE)(unit);//place
        return true;
    }
    return false;
}

void move_all(byte x, byte y)
{
    sort_stat(S_ID, U_FARM, CMP_SMALLER);//non buildings
    sort_stat(S_ANIMATION, 2, CMP_EQ);//only if animation stop
    for (int i = 0; i < units; i++)
    {
        int xx = 0, yy = 0, k = 1;
        bool f = unit_move(x, y, unit[i]);
        xx--;
        while ((!f) & (k < 5))//goes in spiral like original war2 (size 5)
        {
            while ((!f) & (yy < k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                yy++;
            }
            while ((!f) & (xx < k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                xx++;
            }
            while ((!f) & (yy > -k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                yy--;
            }
            while ((!f) & (xx >= -k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                xx--;
            }
            k++;
        }
    }
}

void give_order(int* u, byte x, byte y, byte o)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    if (id < U_FARM)
    {
        char buf[] = "\x0";
        bool f = ((o >= ORDER_SPELL_VISION) && (o <= ORDER_SPELL_ROT));
        if (f)
        {
            buf[0] = o;
            PATCH_SET((char*)GW_ACTION_TYPE, buf);
        }
        int* tr = NULL;
        for (int i = 0; i < 16; i++)
        {
            int* p = (int*)(UNITS_LISTS + 4 * i);//pointer to units list for each player
            if (p)
            {
                p = (int*)(*p);
                while (p)
                {
                    if (p!=u)
                    {
                        if (!check_unit_dead(p) && !check_unit_hidden(p))
                        {
                            byte xx = *(byte*)((uintptr_t)p + S_X);
                            byte yy = *(byte*)((uintptr_t)p + S_Y);
                            if ((abs(x - xx) <= 2) && (abs(y - yy) <= 2))
                            {
                                if (f)
                                {
                                    byte idd = *(byte*)((uintptr_t)p + S_ID);
                                    if (idd < U_FARM)
                                    {
                                        bool trf = true;
                                        if (o == ORDER_SPELL_ARMOR)
                                        {
                                            WORD ef = *(WORD*)((uintptr_t)p + S_SHIELD);
                                            trf = ef == 0;
                                        }
                                        if (o == ORDER_SPELL_BLOODLUST)
                                        {
                                            WORD ef = *(WORD*)((uintptr_t)p + S_BLOOD);
                                            trf = ef == 0;
                                        }
                                        if (o == ORDER_SPELL_HASTE)
                                        {
                                            WORD ef = *(WORD*)((uintptr_t)p + S_HASTE);
                                            trf = (ef == 0) || (ef > 0x7FFF);
                                        }
                                        if (o == ORDER_SPELL_SLOW)
                                        {
                                            WORD ef = *(WORD*)((uintptr_t)p + S_HASTE);
                                            trf = (ef == 0) || (ef <= 0x7FFF);
                                        }
                                        if (o == ORDER_SPELL_POLYMORPH)
                                        {
                                            trf = idd != U_CRITTER;
                                        }
                                        if (o == ORDER_SPELL_HEAL)
                                        {
                                            WORD mhp = *(WORD*)(UNIT_HP_TABLE + 2 * idd);
                                            WORD hp = *((WORD*)((uintptr_t)p + S_HP));
                                            trf = hp < mhp;
                                        }
                                        if (trf)
                                        {
                                            WORD efi = *(WORD*)((uintptr_t)p + S_INVIZ);
                                            trf = efi == 0;
                                        }
                                        if (trf)
                                            tr = p;
                                    }
                                }
                                else
                                    tr = p;
                            }
                        }
                    }
                    p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
                }
            }
        }
        bool aoe = (o == ORDER_SPELL_VISION) || (o == ORDER_SPELL_EXORCISM) || (o == ORDER_SPELL_FIREBALL) ||
            (o == ORDER_SPELL_BLIZZARD) || (o == ORDER_SPELL_EYE) || (o == ORDER_SPELL_RAISEDEAD) ||
            (o == ORDER_SPELL_DRAINLIFE) || (o == ORDER_SPELL_WHIRLWIND) || (o == ORDER_SPELL_RUNES) ||
            (o == ORDER_SPELL_ROT) || (o == ORDER_MOVE) || (o == ORDER_PATROL) ||
            (o == ORDER_ATTACK_AREA) || (o == ORDER_ATTACK_WALL) || (o == ORDER_STAND) ||
            (o == ORDER_ATTACK_GROUND) || (o == ORDER_ATTACK_GROUND_MOVE) || (o == ORDER_DEMOLISH) ||
            (o == ORDER_HARVEST) || (o == ORDER_RETURN) || (o == ORDER_UNLOAD_ALL) || (o == ORDER_STOP);

        if (o != ORDER_ATTACK_WALL)
        {
            int ord = *(int*)(ORDER_FUNCTIONS + 4 * o);//orders functions
            if (!aoe && (tr != NULL) && (tr != u))
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, 0, 0, tr, ord);//original war2 order
            if (aoe)
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, x, y, NULL, ord);//original war2 order
        }
        else
        {
            byte oru = *(byte*)((uintptr_t)u + S_ORDER);
            if (oru!=ORDER_ATTACK_WALL)
            {
                int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_STOP);//orders functions
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, 0, 0, NULL, ord);//original war2 order
            }
            set_stat(u, ORDER_ATTACK_WALL, S_NEXT_ORDER);
            set_stat(u, x, S_ORDER_X);
            set_stat(u, y, S_ORDER_Y);
        }

        if (f)
        {
            buf[0] = 0;
            PATCH_SET((char*)GW_ACTION_TYPE, buf);
        }
    }
}

void give_order_spell_target(int* u, int* t, byte o)
{
    if ((u != NULL) && (t != NULL))
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if (id < U_FARM)
        {
            char buf[] = "\x0";
            if ((o >= ORDER_SPELL_VISION) && (o <= ORDER_SPELL_ROT))
            {
                buf[0] = o;
                PATCH_SET((char*)GW_ACTION_TYPE, buf);

                int ord = *(int*)(ORDER_FUNCTIONS + 4 * o);//orders functions
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, 0, 0, t, ord);//original war2 order

                buf[0] = 0;
                PATCH_SET((char*)GW_ACTION_TYPE, buf);
            }
        }
    }
}

void order_all(byte x, byte y, byte o)
{
    for (int i = 0; i < units; i++)
    {
        give_order(unit[i], x, y, o);
    }
}

bool check_ally(byte p1, byte p2)
{
    //check allied table
    return ((*(byte*)(ALLY + p1 + 16 * p2) != 0) && (*(byte*)(ALLY + p2 + 16 * p1) != 0));
}

bool slot_alive(byte p)
{
    return (get_val(ALL_BUILDINGS, p) + (get_val(ALL_UNITS, p) - get_val(FLYER, p))) > 0;//no units and buildings
}

void ally(byte p1, byte p2, byte a)
{
    //set ally bytes in table
    *(byte*)(ALLY + p1 + 16 * p2) = a;
    *(byte*)(ALLY + p2 + 16 * p1) = a;
    ((void (*)())F_RESET_COLORS)();//orig war2 func reset colors of sqares around units
}

void ally_one_sided(byte p1, byte p2, byte a)
{
    //set ally bytes in table
    *(byte*)(ALLY + p1 + 16 * p2) = a;
    ((void (*)())F_RESET_COLORS)();//orig war2 func reset colors of sqares around units
}

bool check_opponents(byte player)
{
    //check if player have opponents
    bool f = false;
    byte o = C_NOBODY;
    for (byte i = 0; i < 8; i++)
    {
        if (player != i)
        {
            if (slot_alive(i) && !check_ally(player, i))//if enemy and not dead
                f = true;
        }
    }
    return f;
}

void viz(int p1, int p2, byte a)
{
    //set vision bits
    byte v = *(byte*)(VIZ + p1);
    if (a == 0)
        v = v & (~(1 << p2));
    else
        v = v | (1 << p2);
    *(byte*)(VIZ + p1) = v;

    v = *(byte*)(VIZ + p2);
    if (a == 0)
        v = v & (~(1 << p1));
    else
        v = v | (1 << p1);
    *(byte*)(VIZ + p2) = v;
}

void viz_one_sided(int p1, int p2, byte a)
{
    //set vision bits
    byte v = *(byte*)(VIZ + p1);
    if (a == 0)
        v = v & (~(1 << p2));
    else
        v = v | (1 << p2);
    *(byte*)(VIZ + p1) = v;
}

void comps_vision(bool v)
{
    //comps can give vision too
    if (v)
    {
        char o[] = "\x0";
        PATCH_SET((char*)COMPS_VIZION, o);
        char o2[] = "\x90\x90";
        PATCH_SET((char*)COMPS_VIZION2, o2);
        char o3[] = "\x90\x90\x90\x90\x90\x90";
        PATCH_SET((char*)COMPS_VIZION3, o3);
    }
    else
    {
        char o[] = "\xAA";
        PATCH_SET((char*)COMPS_VIZION, o);
        char o2[] = "\x84\xC9";
        PATCH_SET((char*)COMPS_VIZION2, o2);
        char o3[] = "\xF\x85\x8C\x0\x0\x0";
        PATCH_SET((char*)COMPS_VIZION3, o3);
    }
}

void change_res(byte p, byte r, byte k, int m)
{
    int a = GOLD;
    int* rs = (int*)a;
    DWORD res = 0;
    bool s = false;
    if (p >= 0 && p <= 8)//player id
    {
        switch (r)//select resource and add or substract it
        {
        case 0:
            a = GOLD + 4 * p;
            s = false;
            break;
        case 1:
            a = LUMBER + 4 * p;
            s = false;
            break;
        case 2:
            a = OIL + 4 * p;
            s = false;
            break;
        case 3:
            a = GOLD + 4 * p;
            s = true;
            break;
        case 4:
            a = LUMBER + 4 * p;
            s = true;
            break;
        case 5:
            a = OIL + 4 * p;
            s = true;
            break;
        default:break;
        }
        if (r >= 0 && r <= 5)
        {
            rs = (int*)a;//resourse pointer
            if (s)
            {
                if (*rs > (int)(k * m))
                    res = *rs - (k * m);
                else
                    res = 0;//canot go smaller than 0
            }
            else
            {
                if (*rs <= (256 * 256 * 256 * 32))
                    res = *rs + (k * m);
            }
            patch_setdword((DWORD*)a, res);
        }
    }
}

void add_total_res(byte p, byte r, byte k, int m)
{
    int a = GOLD_TOTAL;
    int* rs = (int*)a;
    DWORD res = 0;
    if (p >= 0 && p <= 8)//player id
    {
        switch (r)//select resource and add or substract it
        {
        case 0:
            a = GOLD_TOTAL + 4 * p;
            break;
        case 1:
            a = LUMBER_TOTAL + 4 * p;
            break;
        case 2:
            a = OIL_TOTAL + 4 * p;
            break;
        default:break;
        }
        if (r >= 0 && r <= 2)
        {
            rs = (int*)a;//resourse pointer
            if (*rs <= (256 * 256 * 256 * 32))
                res = *rs + (k * m);
            patch_setdword((DWORD*)a, res);
        }
    }
}

void set_res(byte p, byte r, byte k1, byte k2, byte k3, byte k4)
{
    //as before but dnt add or sub res, just set given value
    char buf[4] = { 0 };
    int a = 0;
    if (p >= 0 && p <= 8)
    {
        switch (r)
        {
        case 0:
            a = GOLD + 4 * p;
            break;
        case 1:
            a = LUMBER + 4 * p;
            break;
        case 2:
            a = OIL + 4 * p;
            break;
        default:break;
        }
        if (r >= 0 && r <= 2)
        {
            buf[0] = k1;
            buf[1] = k2;
            buf[2] = k3;
            buf[3] = k4;
            PATCH_SET((char*)a, buf);
        }
    }
}

bool cmp_res(byte p, byte r, byte k1, byte k2, byte k3, byte k4, byte cmp)
{
    //compare resource to value
    int a = GOLD;
    int* rs = (int*)a;
    if (p >= 0 && p <= 8)
    {
        switch (r)
        {
        case 0:
            a = GOLD + 4 * p;
            break;
        case 1:
            a = LUMBER + 4 * p;
            break;
        case 2:
            a = OIL + 4 * p;
            break;
        default:break;
        }
        if (r >= 0 && r <= 2)
        {
            rs = (int*)a;
            return cmp_args4(cmp, *rs, k1 + 256 * k2 + 256 * 256 * k3 + 256 * 256 * 256 * k4);
        }
    }
    return false;
}

int empty_false(byte) { return 0; }//always return false function
int empty_true(byte) { return 1; }//always return true function
void empty_build(int id)
{
    ((void (*)(int))F_TRAIN_UNIT)(id);//original build unit func
}
void empty_build_building(int id)
{
    ((void (*)(int))F_BUILD_BUILDING)(id);//original build func
}
void empty_build_research(int id)
{
    ((void (*)(int))F_BUILD_RESEARCH)(id);
}
void empty_build_research_spell(int id)
{
    ((void (*)(int))F_BUILD_RESEARCH_SPELL)(id);
}
void empty_build_upgrade_self(int id)
{
    ((void (*)(int))F_BUILD_UPGRADE_SELF)(id);
}
void empty_cast_spell(int id)
{
    ((void (*)(int))F_CAST_SPELL)(id);
}

void empty_upgrade_th1(int id)
{
    int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    set_stat(u, 0, S_AI_AIFLAGS);
    empty_build_upgrade_self(id);
}
void empty_upgrade_th2(int id)
{
    int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    set_stat(u, 1, S_AI_AIFLAGS);
    empty_build_upgrade_self(id);
}

int empty_research_swords(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SWORDS)(id); }//0 or 1
int empty_research_shield(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SHIELD)(id); }//0 or 1
int empty_research_cat(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_CAT)(id); }//0 or 1
int empty_research_arrows(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_ARROWS)(id); }//0 or 1
int empty_research_ships_at(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SHIPS_AT)(id); }//0 or 1
int empty_research_ships_def(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SHIPS_DEF)(id); }//0 or 1
int empty_research_ranger(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_RANGER)(id); }
int empty_research_scout(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SCOUT)(id); }
int empty_research_long(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_LONG)(id); }
int empty_research_marks(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_MARKS)(id); }
int empty_research_spells(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SPELL)(id); }
//00444410
int empty_upgrade_th(byte id) { return ((int (*)(int))F_CHECK_UPGRADE_TH)(id); }//0 or 1
int empty_upgrade_tower(byte id) { return ((int (*)(int))F_CHECK_UPGRADE_TOWER)(id); }//0 or 1
int empty_spell_learned(byte id) { return ((int (*)(int))F_CHECK_SPELL_LEARNED)(id); }

int _2tir() 
{ 
    if ((get_val(TH2, *(byte*)LOCAL_PLAYER) != 0) || (get_val(TH3, *(byte*)LOCAL_PLAYER) != 0))
        return 1;
    else
        return 0;
}

int _3tir()
{
    if (get_val(TH3, *(byte*)LOCAL_PLAYER) != 0)
        return 1;
    else
        return 0;
}

int get_marks()
{
    if (*(byte*)(GB_MARKS + *(byte*)LOCAL_PLAYER))
        return 1;
    else
        return 0;
}

void repair_cat(bool b)
{
    //peon can repair unit if it have transport flag OR catapult flag
    if (b)
    {
        char r1[] = "\xeb\x75\x90\x90\x90";//f6 c4 04 74 14
        PATCH_SET((char*)REPAIR_FLAG_CHECK2, r1);
        char r2[] = "\x66\xa9\x04\x04\x74\x9c\xeb\x86";
        PATCH_SET((char*)REPAIR_CODE_CAVE, r2);
    }
    else
    {
        char r1[] = "\xf6\xc4\x4\x74\x14";
        PATCH_SET((char*)REPAIR_FLAG_CHECK2, r1);
    }
}

void fireball_dmg(byte dmg)
{
    char fb[] = "\x28";//40 default
    fb[0] = dmg;
    PATCH_SET((char*)FIREBALL_DMG, fb);
}

void trigger_time(byte tm)
{
    //war2 will call victory check function every 200 game ticks
    char ttime[] = "\xc8";//200 default
    ttime[0] = tm;
    PATCH_SET((char*)TRIG_TIME, ttime);
}

void manacost(byte id, byte c)
{
    //spells cost of mana
    char mana[] = "\x1";
    mana[0] = c;
    PATCH_SET((char*)(MANACOST + 2 * id), mana);
}

void upgr(byte id, byte c)
{
    //upgrades power
    char up[] = "\x1";
    up[0] = c;
    PATCH_SET((char*)(UPGRD + id), up);
}

byte get_upgrade(byte id, byte pl)
{
    int a = GB_ARROWS;
    switch (id)
    {
    case ARROWS:a = GB_ARROWS; break;
    case SWORDS:a = GB_SWORDS; break;
    case ARMOR:a = GB_SHIELDS; break;
    case SHIP_DMG:a = GB_BOAT_ATTACK; break;
    case SHIP_ARMOR:a = GB_BOAT_ARMOR; break;
    case SHIP_SPEED:a = GB_BOAT_SPEED; break;
    case CATA_DMG:a = GB_CAT_DMG; break;
    default:a = GB_ARROWS; break;
    }
    return *(byte*)(a + pl);
}

void set_upgrade(byte id, byte pl, byte v)
{
    int a = GB_ARROWS;
    switch (id)
    {
    case ARROWS:a = GB_ARROWS; break;
    case SWORDS:a = GB_SWORDS; break;
    case ARMOR:a = GB_SHIELDS; break;
    case SHIP_DMG:a = GB_BOAT_ATTACK; break;
    case SHIP_ARMOR:a = GB_BOAT_ARMOR; break;
    case SHIP_SPEED:a = GB_BOAT_SPEED; break;
    case CATA_DMG:a = GB_CAT_DMG; break;
    default:a = GB_ARROWS; break;
    }
    char buf[] = "\x0";
    buf[0] = v;
    PATCH_SET((char*)(a + pl), buf);
    ((void (*)())F_STATUS_REDRAW)();//status redraw
}

int upgr_check_swords(byte b)
{
    byte u = get_upgrade(SWORDS, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

int upgr_check_shields(byte b)
{
    byte u = get_upgrade(ARMOR, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

int upgr_check_boat_attack(byte b)
{
    byte u = get_upgrade(SHIP_DMG, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

int upgr_check_boat_armor(byte b)
{
    byte u = get_upgrade(SHIP_ARMOR, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

int upgr_check_arrows(byte b)
{
    byte u = get_upgrade(ARROWS, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

void upgr_check_replace(bool f)
{
    if (f)
    {
        char buf[] = "\xC3";//ret
        patch_ljmp((char*)UPGR_CHECK_FIX_SWORDS, (char*)upgr_check_swords);
        PATCH_SET((char*)(UPGR_CHECK_FIX_SWORDS + 5), buf);
        patch_ljmp((char*)UPGR_CHECK_FIX_SHIELDS, (char*)upgr_check_shields);
        PATCH_SET((char*)(UPGR_CHECK_FIX_SHIELDS + 5), buf);
        patch_ljmp((char*)UPGR_CHECK_FIX_SHIPS_AT, (char*)upgr_check_boat_attack);
        PATCH_SET((char*)(UPGR_CHECK_FIX_SHIPS_AT + 5), buf);
        patch_ljmp((char*)UPGR_CHECK_FIX_SHIPS_DEF, (char*)upgr_check_boat_armor);
        PATCH_SET((char*)(UPGR_CHECK_FIX_SHIPS_DEF + 5), buf);
        patch_ljmp((char*)UPGR_CHECK_FIX_ARROWS, (char*)upgr_check_arrows);
        PATCH_SET((char*)(UPGR_CHECK_FIX_ARROWS + 5), buf);
    }
    else
    {
        char buf2[] = "\x33\xC0\x33\xC9\xA0\x18";//back
        PATCH_SET((char*)UPGR_CHECK_FIX_SWORDS, buf2);
        PATCH_SET((char*)UPGR_CHECK_FIX_SHIELDS, buf2);
        PATCH_SET((char*)UPGR_CHECK_FIX_SHIPS_AT, buf2);
        PATCH_SET((char*)UPGR_CHECK_FIX_SHIPS_DEF, buf2);
        PATCH_SET((char*)UPGR_CHECK_FIX_ARROWS, buf2);
    }
}

void center_view(byte x, byte y)
{
    ((void (*)(byte, byte))F_MINIMAP_CLICK)(x, y);//original war2 func that called when player click on minimap
}

PROC g_proc_00451054;
void count_add_to_tables_load_game(int* u)
{
    if (saveload_fixed)
    {
        byte f = *((byte*)((uintptr_t)u + S_AI_AIFLAGS));
        byte ff = f | AI_PASSIVE;
        set_stat(u, ff, S_AI_AIFLAGS);
        ((void (*)(int*))g_proc_00451054)(u);//original
        set_stat(u, f, S_AI_AIFLAGS);
    }
    else
        ((void (*)(int*))g_proc_00451054)(u);//original
}

PROC g_proc_00438A5C;
PROC g_proc_00438985;
void unset_peon_ai_flags(int* u)
{
    ((void (*)(int*))g_proc_00438A5C)(u);//original
    if (saveload_fixed)
    {
        char rep[] = "\x0\x0";
        WORD p = 0;
        for (int i = 0; i < 8; i++)
        {
            p = *((WORD*)((uintptr_t)SGW_REPAIR_PEONS + 2 * i));
            if (p > 1600)
                PATCH_SET((char*)(SGW_REPAIR_PEONS + 2 * i), rep);
            p = *((WORD*)((uintptr_t)SGW_GOLD_PEONS + 2 * i));
            if (p > 1600)
                PATCH_SET((char*)(SGW_GOLD_PEONS + 2 * i), rep);
            p = *((WORD*)((uintptr_t)SGW_TREE_PEONS + 2 * i));
            if (p > 1600)
                PATCH_SET((char*)(SGW_TREE_PEONS + 2 * i), rep);
        }
    }
}

void tech_built(int p, byte t)
{
    ((void (*)(int, byte))F_TECH_BUILT)(p, t);
}

void tech_reinit()
{
    for (int i = 0; i < 8; i++)
    {
        byte o = *(byte*)(CONTROLER_TYPE + i);
        byte a = 0;
        int s = 0;
        if (o == C_COMP)
        {
            a = *(byte*)(GB_ARROWS + i);
            if (a > 0)tech_built(i, UP_ARROW1);
            if (a > 1)tech_built(i, UP_ARROW2);
            a = *(byte*)(GB_SWORDS + i);
            if (a > 0)tech_built(i, UP_SWORD1);
            if (a > 1)tech_built(i, UP_SWORD2);
            a = *(byte*)(GB_SHIELDS + i);
            if (a > 0)tech_built(i, UP_SHIELD1);
            if (a > 1)tech_built(i, UP_SHIELD2);
            a = *(byte*)(GB_BOAT_ATTACK + i);
            if (a > 0)tech_built(i, UP_BOATATK1);
            if (a > 1)tech_built(i, UP_BOATATK2);
            a = *(byte*)(GB_BOAT_ARMOR + i);
            if (a > 0)tech_built(i, UP_BOATARM1);
            if (a > 1)tech_built(i, UP_BOATARM2);
            a = *(byte*)(GB_CAT_DMG + i);
            if (a > 0)tech_built(i, UP_CATDMG1);
            if (a > 1)tech_built(i, UP_CATDMG2);
            a = *(byte*)(GB_RANGER + i);
            if (a)tech_built(i, UP_RANGER);
            a = *(byte*)(GB_MARKS + i);
            if (a)tech_built(i, UP_SKILL1);
            a = *(byte*)(GB_LONGBOW + i);
            if (a)tech_built(i, UP_SKILL2);
            a = *(byte*)(GB_SCOUTING + i);
            if (a)tech_built(i, UP_SKILL3);

            s = *(int*)(SPELLS_LEARNED + 4 * i);
            if (s & (1 << L_ALTAR_UPGR))tech_built(i, UP_CLERIC);
            if (s & (1 << L_HEAL))tech_built(i, UP_CLERIC1);
            if (s & (1 << L_BLOOD))tech_built(i, UP_CLERIC1);
            if (s & (1 << L_EXORCISM))tech_built(i, UP_CLERIC2);
            if (s & (1 << L_RUNES))tech_built(i, UP_CLERIC2);
            if (s & (1 << L_FLAME_SHIELD))tech_built(i, UP_WIZARD1);
            if (s & (1 << L_RAISE))tech_built(i, UP_WIZARD1);
            if (s & (1 << L_SLOW))tech_built(i, UP_WIZARD2);
            if (s & (1 << L_HASTE))tech_built(i, UP_WIZARD2);
            if (s & (1 << L_INVIS))tech_built(i, UP_WIZARD3);
            if (s & (1 << L_WIND))tech_built(i, UP_WIZARD3);
            if (s & (1 << L_POLYMORF))tech_built(i, UP_WIZARD4);
            if (s & (1 << L_UNHOLY))tech_built(i, UP_WIZARD4);
            if (s & (1 << L_BLIZZARD))tech_built(i, UP_WIZARD5);
            if (s & (1 << L_DD))tech_built(i, UP_WIZARD5);

            find_all_alive_units(U_KEEP);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)tech_built(i, UP_KEEP);
            find_all_alive_units(U_STRONGHOLD);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)tech_built(i, UP_KEEP);
            find_all_alive_units(U_CASTLE);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)
            {
                tech_built(i, UP_KEEP);
                tech_built(i, UP_CASTLE);
            }
            find_all_alive_units(U_FORTRESS);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)
            {
                tech_built(i, UP_KEEP);
                tech_built(i, UP_CASTLE);
            }
        }
    }
}

void building_start_build(int* u, byte id, byte o)
{
    ((void (*)(int*, byte, byte))F_BLDG_START_BUILD)(u, id, o);
}

void build_inventor(int* u)
{
    if (check_unit_complete(u))
    {
        byte f = *((byte*)((uintptr_t)u + S_FLAGS1));
        if (!(f & UF_BUILD_ON))
        {
            byte id = *((byte*)((uintptr_t)u + S_ID));
            byte o = *((byte*)((uintptr_t)u + S_OWNER));
            int spr = get_val(ACTIVE_SAPPERS, o);
            byte nspr = *(byte*)(AIP_NEED_SAP + 48 * o);
            if (nspr > spr)
            {
                if (id == U_INVENTOR)building_start_build(u, U_DWARWES, 0);
                if (id == U_ALCHEMIST)building_start_build(u, U_GOBLINS, 0);
            }
            int flr = get_val(ACTIVE_FLYER, o);
            byte nflr = *(byte*)(AIP_NEED_FLYER + 48 * o);
            if (nflr > flr)
            {
                if (id == U_INVENTOR)building_start_build(u, U_FLYER, 0);
                if (id == U_ALCHEMIST)building_start_build(u, U_ZEPPELIN, 0);
            }
        }
    }
}

void build_sap_fix(bool f)
{
    if (f)
    {
        char b1[] = "\x80\xfa\x40\x0";
        void (*r1) (int*) = build_inventor;
        patch_setdword((DWORD*)b1, (DWORD)r1);
        PATCH_SET((char*)BLDG_WAIT_INVENTOR, b1);//human inv
        PATCH_SET((char*)(BLDG_WAIT_INVENTOR + 4), b1);//orc inv
    }
    else
    {
        char b1[] = "\x80\xfa\x40\x0";
        PATCH_SET((char*)BLDG_WAIT_INVENTOR, b1);//human inv
        PATCH_SET((char*)(BLDG_WAIT_INVENTOR + 4), b1);//orc inv
    }
}

void ai_fix_plugin(bool f)
{
    if (f)
    {
        char b1[] = "\xb2\x02";
        PATCH_SET((char*)AIFIX_PEONS_REP, b1);//2 peon rep
        char b21[] = "\xbb\x8";
        PATCH_SET((char*)AIFIX_GOLD_LUMB1, b21);//gold lumber
        char b22[] = "\xb4\x4";
        PATCH_SET((char*)AIFIX_GOLD_LUMB2, b22);//gold lumber
        char b3[] = "\x1";
        PATCH_SET((char*)AIFIX_BUILD_SIZE, b3);//packed build
        char b4[] = "\xbe\x0\x0\x0\x0\x90\x90";
        PATCH_SET((char*)AIFIX_FIND_HOME, b4);//th corner
        char b6[] = "\x90\x90\x90\x90\x90\x90";
        PATCH_SET((char*)AIFIX_POWERBUILD, b6);//powerbuild
        char m7[] = "\x90\x90";
        PATCH_SET((char*)AIFIX_RUNES_INV, m7);//runes no inviz
        ai_fixed = true;
        build_sap_fix(true);
    }
    else
    {
        char b1[] = "\x8a\xd0";
        PATCH_SET((char*)AIFIX_PEONS_REP, b1);//2 peon rep
        char b21[] = "\xd0\x7";
        PATCH_SET((char*)AIFIX_GOLD_LUMB1, b21);//gold lumber
        char b22[] = "\xf4\x1";
        PATCH_SET((char*)AIFIX_GOLD_LUMB2, b22);//gold lumber
        char b3[] = "\x6";
        PATCH_SET((char*)AIFIX_BUILD_SIZE, b3);//packed build
        char b4[] = "\xe8\xf8\x2a\x1\x0\x8b\xf0";
        PATCH_SET((char*)AIFIX_FIND_HOME, b4);//th corner
        char b6[] = "\xf\x84\x78\x1\x0\x0";
        PATCH_SET((char*)AIFIX_POWERBUILD, b6);//powerbuild
        char m7[] = "\x74\x1d";
        PATCH_SET((char*)AIFIX_RUNES_INV, m7);//runes no inviz
        ai_fixed = false;
        build_sap_fix(false);
    }
}

PROC g_proc_0040EEDD;
void upgrade_tower(int* u, int id, int b)
{
    if (ai_fixed)
    {
        bool c = false;
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        if ((get_val(LUMBERMILL, o) == 0) && (get_val(SMITH, o) != 0)) c = true;
        if ((get_val(LUMBERMILL, o) != 0) && (get_val(SMITH, o) != 0) && ((get_val(TOWER, o) % 2) == 0)) c = true;
        if (c)id += 2;
    }
    ((void (*)(int*, int, int))g_proc_0040EEDD)(u, id, b);//original
}

PROC g_proc_00442E25;
void create_skeleton(int x, int y, int id, int o)
{
    if (ai_fixed)
    {
        unit_create((x / 32) + 1, y / 32, id, o % 256, 1);
    }
    else
        ((void (*)(int, int, int, int))g_proc_00442E25)(x, y, id, o);//original
}

PROC g_proc_00425D1C;
int* cast_raise(int* u, int a1, int a2, int a3)
{
    if (ai_fixed)
    {
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        find_all_alive_units(U_SKELETON);
        sort_stat(S_OWNER, o, CMP_EQ);
        sort_preplaced();
        if (units < 10)
        {
            if (((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_RAISE)) == 0))return NULL;
            byte mp = *((byte*)((uintptr_t)u + S_MANA));
            byte cost = *(byte*)(MANACOST + 2 * RAISE_DEAD);
            if (mp < cost)return NULL;
            byte x = *((byte*)((uintptr_t)u + S_X));
            byte y = *((byte*)((uintptr_t)u + S_Y));
            set_region((int)x - 8, (int)y - 8, (int)x + 8, (int)y + 8);//set region around myself
            find_all_units(ANY_BUILDING);//dead body
            sort_in_region();
            sort_hidden();
            sort_near_death();
            if (units != 0)
            {
                byte xx = *((byte*)((uintptr_t)unit[0] + S_X));
                byte yy = *((byte*)((uintptr_t)unit[0] + S_Y));
                give_order(u, xx, yy, ORDER_SPELL_RAISEDEAD);
                return unit[0];
            }
        }
        return NULL;
    }
    else
        return ((int* (*)(int*, int, int, int))g_proc_00425D1C)(u, a1, a2, a3);//original
}

PROC g_proc_00424F94;
PROC g_proc_00424FD7;
int* cast_runes(int* u, int a1, int a2, int a3)
{
    if (ai_fixed)
    {
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        if (((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_RUNES)) == 0))return NULL;
        byte mp = *((byte*)((uintptr_t)u + S_MANA));
        byte cost = *(byte*)(MANACOST + 2 * RUNES);
        if (mp < cost)return NULL;
        byte x = *((byte*)((uintptr_t)u + S_X));
        byte y = *((byte*)((uintptr_t)u + S_Y));
        set_region((int)x - 14, (int)y - 14, (int)x + 14, (int)y + 14);//set region around myself
        find_all_alive_units(ANY_MEN);
        sort_in_region();
        sort_hidden();
        sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
        for (int ui = 0; ui < 16; ui++)
        {
            if (check_ally(o, ui))//only not allied units
                sort_stat(S_OWNER, ui, CMP_NEQ);
        }
        sort_rune_near();
        if (units != 0)
        {
            byte xx = *((byte*)((uintptr_t)unit[0] + S_X));
            byte yy = *((byte*)((uintptr_t)unit[0] + S_Y));
            give_order(u, xx, yy, ORDER_SPELL_RUNES);
            return unit[0];
        }
        return NULL;
    }
    else
        return ((int* (*)(int*, int, int, int))g_proc_00424F94)(u, a1, a2, a3);//original
}

byte get_manacost(byte s)
{
    return *(byte*)(MANACOST + 2 * s);
}

PROC g_proc_0042757E;
int ai_spell(int* u)
{
    if (ai_fixed)
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if ((id == U_MAGE) || (id == U_DK))
        {
            byte x = *((byte*)((uintptr_t)u + S_X));
            byte y = *((byte*)((uintptr_t)u + S_Y));
            set_region((int)x - 24, (int)y - 24, (int)x + 24, (int)y + 24);//set region around myself
            find_all_alive_units(ANY_UNITS);
            sort_in_region();
            byte o = *((byte*)((uintptr_t)u + S_OWNER));
            for (int ui = 0; ui < 16; ui++)
            {
                if (check_ally(o, ui))
                    sort_stat(S_OWNER, ui, CMP_NEQ);
            }
            if (units != 0)
            {
                byte mp = *((byte*)((uintptr_t)u + S_MANA));
                byte ord = *((byte*)((uintptr_t)u + S_ORDER));
                bool new_cast = (ord == ORDER_SPELL_ROT) || (ord == ORDER_SPELL_BLIZZARD) || (ord == ORDER_SPELL_INVIS) || (ord == ORDER_SPELL_ARMOR);
                WORD shl = *((WORD*)((uintptr_t)u + S_SHIELD));
                WORD inv = *((WORD*)((uintptr_t)u + S_INVIZ));
                if ((shl == 0) && (inv == 0))
                {
                    set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12);//set region around myself
                    find_all_alive_units(ANY_MEN);
                    sort_in_region();
                    for (int ui = 0; ui < 16; ui++)
                    {
                        if (check_ally(o, ui))//enemy
                            sort_stat(S_OWNER, ui, CMP_NEQ);
                    }
                    if (units != 0)
                    {
                        struct GPOINT
                        {
                            WORD x;
                            WORD y;
                        } l;
                        l.x = *((WORD*)((uintptr_t)u + S_X));
                        l.y = *((WORD*)((uintptr_t)u + S_Y));
                        ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(u, AI_ORDER_DEFEND, &l);
                        set_stat(u, l.x, S_AI_DEST_X);
                        set_stat(u, l.y, S_AI_DEST_Y);
                    }
                    if (mp < 50)new_cast = false;
                    else
                    {
                        if (id == U_MAGE)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_INVIS)) != 0)
                            {
                                if (ord != ORDER_SPELL_POLYMORPH)
                                {
                                    if (mp >= get_manacost(INVIS))
                                    {
                                        set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DK, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_MAGE, CMP_BIGGER_EQ);
                                        sort_self(u);
                                        sort_stat(S_INVIZ, 0, CMP_EQ);
                                        sort_stat(S_MANA, 150, CMP_BIGGER_EQ);
                                        sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (!check_ally(o, ui))
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_INVIS);
                                            new_cast = true;
                                        }
                                    }
                                }
                            }
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_POLYMORF)) != 0)
                            {
                                if (ord != ORDER_SPELL_POLYMORPH)
                                {
                                    if (mp >= get_manacost(POLYMORPH))
                                    {
                                        set_region((int)x - 18, (int)y - 18, (int)x + 18, (int)y + 18);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DRAGON, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_GRIFON, CMP_BIGGER_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (check_ally(o, ui))//enemy
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_POLYMORPH);
                                            new_cast = true;
                                        }
                                    }
                                }
                            }
                        }
                        else if (id == U_DK)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_UNHOLY)) != 0)
                            {
                                if (ord != ORDER_SPELL_ARMOR)
                                {
                                    if (mp >= get_manacost(UNHOLY_ARMOR))
                                    {
                                        set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DK, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_MAGE, CMP_BIGGER_EQ);
                                        sort_self(u);
                                        sort_stat(S_SHIELD, 0, CMP_EQ);
                                        sort_stat(S_MANA, 150, CMP_BIGGER_EQ);
                                        sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (!check_ally(o, ui))
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_ARMOR);
                                            new_cast = true;
                                        }
                                    }
                                }
                                else new_cast = true;
                            }
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_HASTE)) != 0)
                            {
                                if (ord != ORDER_SPELL_HASTE)
                                {
                                    if (mp >= get_manacost(HASTE))
                                    {
                                        set_region((int)x - 16, (int)y - 16, (int)x + 16, (int)y + 16);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DK, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_MAGE, CMP_BIGGER_EQ);
                                        sort_self(u);
                                        sort_stat(S_SHIELD, 0, CMP_NEQ);
                                        sort_stat(S_HASTE, 0, CMP_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (!check_ally(o, ui))
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_HASTE);
                                            new_cast = true;
                                        }
                                    }
                                }
                                else new_cast = true;
                            }
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_COIL)) != 0)
                            {
                                if (ord != ORDER_SPELL_DRAINLIFE)
                                {
                                    if (mp >= get_manacost(COIL))
                                    {
                                        set_region((int)x - 18, (int)y - 18, (int)x + 18, (int)y + 18);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DRAGON, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_GRIFON, CMP_BIGGER_EQ);
                                        sort_stat(S_ANIMATION, ANIM_MOVE, CMP_NEQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (check_ally(o, ui))//enemy
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            byte xx = *((byte*)((uintptr_t)unit[0] + S_X));
                                            byte yy = *((byte*)((uintptr_t)unit[0] + S_Y));
                                            give_order(u, xx, yy, ORDER_SPELL_DRAINLIFE);
                                            new_cast = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    set_region((int)x - 50, (int)y - 50, (int)x + 50, (int)y + 50);//set region around myself
                    find_all_alive_units(ANY_MEN);
                    sort_in_region();
                    sort_stat(S_ID, U_PEON, CMP_SMALLER_EQ);
                    sort_stat(S_ID, U_PEASANT, CMP_BIGGER_EQ);
                    sort_peon_loaded(0);
                    sort_hidden();
                    for (int ui = 0; ui < 16; ui++)
                    {
                        if (check_ally(o, ui))
                            sort_stat(S_OWNER, ui, CMP_NEQ);//enemy peons
                    }
                    if (units == 0)
                    {
                        find_all_alive_units(ANY_BUILDING);
                        sort_in_region();
                        sort_stat(S_ID, U_FORTRESS, CMP_SMALLER_EQ);//fortres castle stronghold keep
                        sort_stat(S_ID, U_KEEP, CMP_BIGGER_EQ);
                        for (int ui = 0; ui < 16; ui++)
                        {
                            if (check_ally(o, ui))
                                sort_stat(S_OWNER, ui, CMP_NEQ);
                        }
                        if (units == 0)
                        {
                            find_all_alive_units(ANY_BUILDING);
                            sort_in_region();
                            sort_stat(S_ID, U_GREAT_HALL, CMP_SMALLER_EQ);
                            sort_stat(S_ID, U_TOWN_HALL, CMP_BIGGER_EQ);
                            for (int ui = 0; ui < 16; ui++)
                            {
                                if (check_ally(o, ui))
                                    sort_stat(S_OWNER, ui, CMP_NEQ);
                            }
                        }
                    }
                    if (units != 0)
                    {
                        if (id == U_MAGE)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_BLIZZARD)) != 0)
                            {
                                if (ord != ORDER_SPELL_BLIZZARD)
                                {
                                    if (mp >= get_manacost(BLIZZARD))
                                    {
                                        byte tx = *((byte*)((uintptr_t)unit[0] + S_X));
                                        byte ty = *((byte*)((uintptr_t)unit[0] + S_Y));
                                        give_order(u, tx, ty, ORDER_SPELL_BLIZZARD);
                                        new_cast = true;
                                    }
                                }
                                else new_cast = true;
                            }
                        }
                        if (id == U_DK)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_DD)) != 0)
                            {
                                if (ord != ORDER_SPELL_ROT)
                                {
                                    if (mp >= get_manacost(DEATH_AND_DECAY))
                                    {
                                        byte tx = *((byte*)((uintptr_t)unit[0] + S_X));
                                        byte ty = *((byte*)((uintptr_t)unit[0] + S_Y));
                                        give_order(u, tx, ty, ORDER_SPELL_ROT);
                                        new_cast = true;
                                    }
                                }
                                else new_cast = true;
                            }
                            else if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_WIND)) != 0)
                            {
                                if (!new_cast && (ord != ORDER_SPELL_WHIRLWIND))
                                {
                                    if (mp >= get_manacost(WHIRLWIND))
                                    {
                                        byte tx = *((byte*)((uintptr_t)unit[0] + S_X));
                                        byte ty = *((byte*)((uintptr_t)unit[0] + S_Y));
                                        give_order(u, tx, ty, ORDER_SPELL_WHIRLWIND);
                                        new_cast = true;
                                    }
                                }
                                else new_cast = true;
                            }
                        }
                    }
                }
                if (!new_cast)
                    return ((int (*)(int*))g_proc_0042757E)(u);//original
            }
        }
        else
            return ((int (*)(int*))g_proc_0042757E)(u);//original
        return 0;
    }
    else
        return ((int (*)(int*))g_proc_0042757E)(u);//original
}

PROC g_proc_00427FAE;
void ai_attack(int* u, int b, int a)
{
    if (ai_fixed)
    {
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        for (int i = 0; i < 16; i++)
        {
            int* p = (int*)(UNITS_LISTS + 4 * i);
            if (p)
            {
                p = (int*)(*p);
                while (p)
                {
                    bool f = ((*((byte*)((uintptr_t)p + S_ID)) == U_MAGE) || (*((byte*)((uintptr_t)p + S_ID)) == U_DK));
                    if (f)
                    {
                        if (!check_unit_dead(p) && !check_unit_hidden(p))
                        {
                            byte ow = *((byte*)((uintptr_t)p + S_OWNER));
                            if (ow == o)
                            {
                                if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                                {
                                    WORD inv = *((WORD*)((uintptr_t)p + S_INVIZ));
                                    if (inv == 0)
                                    {
                                        byte aor = *((byte*)((uintptr_t)p + S_AI_ORDER));
                                        if (aor != AI_ORDER_ATTACK)
                                        {
                                            byte mp = *((byte*)((uintptr_t)p + S_MANA));
                                            if (mp >= 200)
                                            {
                                                ((void (*)(int*, int, int))F_ICE_SET_AI_ORDER)(p, AI_ORDER_ATTACK, a);//ai attack
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
                }
            }
        }

        find_all_alive_units(ANY_MEN);
        sort_stat(S_ID, U_GOBLINS, CMP_SMALLER_EQ);
        sort_stat(S_ID, U_DWARWES, CMP_BIGGER_EQ);
        sort_stat(S_OWNER, o, CMP_EQ);
        sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_NEQ);//not attack already
        for (int i = 0; i < units; i++)
        {
            ((void (*)(int*, int, int))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, a);//ai attack
        }
    }

    byte id = *((byte*)((uintptr_t)u + S_ID));
    if (id==U_GRIFON)
    {
        byte od = *((byte*)((uintptr_t)u + S_ORDER));
        if (od != ORDER_SPELL_RAISEDEAD)((void (*)(int*, int, int))g_proc_00427FAE)(u, b, a);//original
    }
    else((void (*)(int*, int, int))g_proc_00427FAE)(u, b, a);//original
}

PROC g_proc_00427FFF;
void ai_attack_nearest_enemy(int* u, WORD x, WORD y, int* t, int ordr)
{
    if (ai_fixed)
    {
        struct GPOINT
        {
            WORD x;
            WORD y;
        };
        GPOINT l;
        l.x = *((WORD*)((uintptr_t)t + S_X));
        l.y = *((WORD*)((uintptr_t)t + S_Y));
        ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(u, AI_ORDER_ATTACK, &l);
    }
    else ((void (*)(int*, WORD, WORD, int*, int))g_proc_00427FFF)(u, x, y, t, ordr);//original
}

void sap_behaviour()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = ((*((byte*)((uintptr_t)p + S_ID)) == U_DWARWES) || (*((byte*)((uintptr_t)p + S_ID)) == U_GOBLINS));
                if (f)
                {
                    if (!check_unit_dead(p) && !check_unit_hidden(p))
                    {
                        byte o = *((byte*)((uintptr_t)p + S_OWNER));
                        if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                        {
                            byte ord = *((byte*)((uintptr_t)p + S_ORDER));
                            byte x = *((byte*)((uintptr_t)p + S_X));
                            byte y = *((byte*)((uintptr_t)p + S_Y));
                            if ((ord != ORDER_DEMOLISH) && (ord != ORDER_DEMOLISH_NEAR) && (ord != ORDER_DEMOLISH_AT))
                            {
                                set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12);//set region around myself
                                find_all_alive_units(ANY_UNITS);
                                sort_in_region();
                                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                                for (int ui = 0; ui < 16; ui++)
                                {
                                    if (check_ally(o, ui))//only not allied units
                                        sort_stat(S_OWNER, ui, CMP_NEQ);
                                }
                                if (units != 0)
                                {
                                    int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_DEMOLISH);
                                    ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(p, 0, 0, unit[0], ord);
                                }
                                set_region((int)x - 5, (int)y - 5, (int)x + 5, (int)y + 5);//set region around myself
                                find_all_alive_units(ANY_UNITS);
                                sort_in_region();
                                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                                for (int ui = 0; ui < 16; ui++)
                                {
                                    if (check_ally(o, ui))//only not allied units
                                        sort_stat(S_OWNER, ui, CMP_NEQ);
                                }
                                if (units != 0)
                                {
                                    int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_DEMOLISH);
                                    ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(p, 0, 0, unit[0], ord);
                                }
                                set_region((int)x - 1, (int)y - 1, (int)x + 1, (int)y + 1);//set region around myself
                                find_all_alive_units(ANY_UNITS);
                                sort_in_region();
                                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                                for (int ui = 0; ui < 16; ui++)
                                {
                                    if (check_ally(o, ui))//only not allied units
                                        sort_stat(S_OWNER, ui, CMP_NEQ);
                                }
                                if (units != 0)
                                {
                                    int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_DEMOLISH);
                                    ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(p, 0, 0, unit[0], ord);
                                }
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void unstuk()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                byte id = *((byte*)((uintptr_t)p + S_ID));
                byte mov = *((byte*)((uintptr_t)p + S_MOVEMENT_TYPE));
                byte ord = *((byte*)((uintptr_t)p + S_ORDER));
                byte aord = *((byte*)((uintptr_t)p + S_AI_ORDER));
                bool f = ((((id < U_CRITTER) && (mov == MOV_LAND))
                    && !check_unit_preplaced(p) && ((ord == ORDER_STOP) || (ord == ORDER_MOVE_PATROL)) && (aord == AI_ORDER_ATTACK)) ||
                    ((id == U_PEASANT) || (id == U_PEON)));
                //if (*(byte*)GB_MULTIPLAYER)if (!((id == U_PEASANT) || (id == U_PEON)))f = false;
                if (f)
                {
                    if (!check_unit_dead(p))
                    {
                        if (!check_unit_hidden(p))
                        {
                            byte o = *((byte*)((uintptr_t)p + S_OWNER));
                            if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                            {
                                byte st = *((byte*)((uintptr_t)p + S_NEXT_FIRE));
                                byte frm = *((byte*)((uintptr_t)p + S_FRAME));
                                byte pfrm = *((byte*)((uintptr_t)p + S_NEXT_FIRE + 1));
                                if (st == 0)
                                {
                                    byte map = *(byte*)MAP_SIZE - 1;
                                    byte x = *((byte*)((uintptr_t)p + S_X));
                                    byte y = *((byte*)((uintptr_t)p + S_Y));
                                    int xx = 0, yy = 0, dir = 0;
                                    xx += x;
                                    yy += y;
                                    dir = ((int (*)())F_NET_RANDOM)();
                                    dir %= 8;
                                    if (dir == 0)
                                    {
                                        if (yy > 0)yy -= 1;
                                    }
                                    if (dir == 1)
                                    {
                                        if (yy > 0)yy -= 1;
                                        if (xx < map)xx += 1;
                                    }
                                    if (dir == 2)
                                    {
                                        if (xx < map)xx += 1;
                                    }
                                    if (dir == 3)
                                    {
                                        if (xx < map)xx += 1;
                                        if (yy < map)yy += 1;
                                    }
                                    if (dir == 4)
                                    {
                                        if (yy < map)yy += 1;
                                    }
                                    if (dir == 5)
                                    {
                                        if (yy < map)yy += 1;
                                        if (xx > 0)xx -= 1;
                                    }
                                    if (dir == 6)
                                    {
                                        if (xx > 0)xx -= 1;
                                    }
                                    if (dir == 7)
                                    {
                                        if (xx > 0)xx -= 1;
                                        if (yy > 0)yy -= 1;
                                    }
                                    give_order(p, xx % 256, yy % 256, ORDER_MOVE);
                                    st = 255;
                                }
                                if (st > 0)st -= 1;
                                if (frm != pfrm)st = 255;
                                set_stat(p, st, S_NEXT_FIRE);
                                set_stat(p, frm, S_NEXT_FIRE + 1);
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void goldmine_ai()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = ((*((byte*)((uintptr_t)p + S_ID)) == U_MINE));
                if (f)
                {
                    if (check_unit_complete(p))
                    {
                        byte x = *((byte*)((uintptr_t)p + S_X));
                        byte y = *((byte*)((uintptr_t)p + S_Y));
                        set_region((int)x - 9, (int)y - 9, (int)x + 8, (int)y + 8);
                        find_all_alive_units(ANY_BUILDING_4x4);
                        sort_in_region();
                        sort_stat(S_ID, U_PORTAL, CMP_NEQ);
                        bool th = units != 0;
                        byte x1, y1, x2, y2;
                        if (x > 3)x1 = x - 3;
                        else x1 = 0;
                        if (y > 3)y1 = y - 3;
                        else y1 = 0;
                        x += 3;
                        y += 3;
                        if (x >= (127 - 3))x2 = 127;
                        else x2 = x + 3;
                        if (y >= (127 - 3))y2 = 127;
                        else y2 = y + 3;
                        char* sq = (char*)*(int*)(MAP_SQ_POINTER);
                        byte mxs = *(byte*)MAP_SIZE;//map size
                        for (int xx = x1; xx < x2; xx++)
                        {
                            for (int yy = y1; yy < y2; yy++)
                            {
                                char buf[] = "\x0";
                                buf[0] = *(char*)(sq + 2 * xx + 2 * yy * mxs + 1);
                                if (th)buf[0] |= SQ_AI_BUILDING >> 8;
                                else buf[0] &= ~(SQ_AI_BUILDING >> 8);
                                PATCH_SET((char*)(sq + 2 * xx + 2 * yy * mxs + 1), buf);
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void set_tp_flag(bool f)
{
    for (int i = 0; i < units; i++)
    {
        //set if unit can be teleported by portal (that flag unused in actual game)
        if (f)
            *((byte*)((uintptr_t)unit[i] + S_FLAGS3)) |= SF_TELEPORT;
        else
            *((byte*)((uintptr_t)unit[i] + S_FLAGS3)) &= ~SF_TELEPORT;
    }
}

void sort_tp_flag()
{
    //if not teleported by portal
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if ((*((byte*)((uintptr_t)unit[i] + S_FLAGS3)) & SF_TELEPORT) == 0)//unused in actual game flag
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void portal()
{
    bool mp = true;
    for (int i = 0; i < 16; i++)
    {
        units = 0;
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            int* fp = NULL;
            while (p)
            {
                bool f = *((byte*)((uintptr_t)p + S_ID)) == U_PORTAL;
                if (f)
                {
                    if (!check_unit_dead(p) && check_unit_complete(p))
                    {//alive and completed portal
                        if (!fp)fp = p;//remember first portal
                        byte tx = *((byte*)((uintptr_t)p + S_X)) + 1;
                        byte ty = *((byte*)((uintptr_t)p + S_Y)) + 1;//exit point is in center of portal
                        move_all(tx, ty);//teleport from previous portal
                        set_tp_flag(true);
                        set_stat_all(S_NEXT_ORDER, ORDER_STOP);
                        set_stat_all(S_ORDER_X, 128);
                        set_stat_all(S_ORDER_Y, 128);
                        byte x = *((byte*)((uintptr_t)p + S_X));
                        byte y = *((byte*)((uintptr_t)p + S_Y));
                        set_region((int)x - 1, (int)y - 1, (int)x + 4, (int)y + 4);//set region around myself
                        find_all_alive_units(ANY_MEN);
                        sort_in_region();
                        sort_tp_flag();//flag show if unit was not teleported
                        sort_stat(S_ORDER, ORDER_STOP, CMP_EQ);
                        sort_stat(S_ORDER_UNIT_POINTER, 0, CMP_EQ);
                        sort_stat(S_ORDER_UNIT_POINTER + 1, 0, CMP_EQ);
                        sort_stat(S_ORDER_UNIT_POINTER + 2, 0, CMP_EQ);
                        sort_stat(S_ORDER_UNIT_POINTER + 3, 0, CMP_EQ);
                        set_region((int)x, (int)y, (int)x + 3, (int)y + 3);//set region inside myself
                        sort_target_in_region();
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
            if (fp)//first portal teleports from last
            {
                byte tx = *((byte*)((uintptr_t)fp + S_X)) + 1;
                byte ty = *((byte*)((uintptr_t)fp + S_Y)) + 1;
                move_all(tx, ty);
                set_tp_flag(true);
                set_stat_all(S_NEXT_ORDER, ORDER_STOP);
                set_stat_all(S_ORDER_X, 128);
                set_stat_all(S_ORDER_Y, 128);
            }
        }
    }
    find_all_alive_units(ANY_MEN);
    set_tp_flag(false);//reset tp flags to all
}

void wharf()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = (*((byte*)((uintptr_t)p + S_ID)) == U_SHIPYARD) || (*((byte*)((uintptr_t)p + S_ID)) == U_WHARF);
                if (f)
                {
                    if (!check_unit_dead(p) && check_unit_complete(p))
                    {
                        byte x = *((byte*)((uintptr_t)p + S_X));
                        byte y = *((byte*)((uintptr_t)p + S_Y));
                        set_region((int)x - 2, (int)y - 2, (int)x + 4, (int)y + 4);//set region around myself
                        find_all_alive_units(ANY_MEN);
                        sort_in_region();
                        sort_hidden();
                        sort_stat(S_MOVEMENT_TYPE, MOV_WATER, CMP_BIGGER_EQ);//find ships - movement type >= water (2 or 3 actually(ships=2 transport=3))
                        byte o = *((byte*)((uintptr_t)p + S_OWNER));
                        for (int ui = 0; ui < 16; ui++)
                        {
                            if (!check_ally(o, ui))//only allied ships
                                sort_stat(S_OWNER, ui, CMP_NEQ);
                        }
                        heal_all(4, 0);
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void paladin()
{
    for (int ii = 0; ii < 16; ii++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * ii);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = ((*((byte*)((uintptr_t)p + S_ID)) == U_PALADIN) ||
                    (*((byte*)((uintptr_t)p + S_ID)) == U_UTER) ||
                    (*((byte*)((uintptr_t)p + S_ID)) == U_TYRALYON));
                if (f)
                {
                    if (!check_unit_dead(p) && !check_unit_hidden(p))
                    {
                        byte o = *((byte*)((uintptr_t)p + S_OWNER));
                        if (((*(byte*)(SPELLS_LEARNED + 4 * o) & (1 << L_HEAL)) != 0) &&
                            ((*(byte*)(SPELLS_LEARNED + 4 * o) & (1 << L_GREATER_HEAL)) != 0))
                            //if player learned heal and autoheal
                        {
                            byte x = *((byte*)((uintptr_t)p + S_X));
                            byte y = *((byte*)((uintptr_t)p + S_Y));
                            set_region((int)x - 5, (int)y - 5, (int)x + 5, (int)y + 5);//set region around myself
                            find_all_alive_units(ANY_MEN);
                            sort_in_region();
                            sort_hidden();
                            sort_fleshy();//fleshy units (not heal cata and ships)
                            sort_full_hp();//if unit hp not full
                            sort_self(p);//not heal self
                            sort_order_hp();//heal lovest hp first
                            for (int ui = 0; ui < 16; ui++)
                            {
                                if (!check_ally(o, ui))//only allied units
                                    sort_stat(S_OWNER, ui, 1);
                            }
                            byte cost = *(byte*)(MANACOST + 2 * GREATER_HEAL);//2* cause manacost is WORD
                            for (int i = 0; i < units; i++)
                            {
                                byte mp = *((byte*)((uintptr_t)p + S_MANA));//paladin mp
                                if (mp >= cost)
                                {
                                    byte id = *((byte*)((uintptr_t)unit[i] + S_ID));//unit id
                                    WORD mhp = *(WORD*)(UNIT_HP_TABLE + 2 * id);//max hp
                                    WORD hp = *((WORD*)((uintptr_t)unit[i] + S_HP));//unit hp
                                    WORD shp = mhp - hp;//shortage of hp
                                    while (!(mp >= (shp * cost)) && (shp > 0))shp -= 1;
                                    if (shp > 0)//if can heal at least 1 hp
                                    {
                                        heal(unit[i], (byte)shp, 0);
                                        mp -= shp * cost;
                                        *((byte*)((uintptr_t)p + S_MANA)) = mp;
                                        WORD xx = *((WORD*)((uintptr_t)unit[i] + S_DRAW_X));
                                        WORD yy = *((WORD*)((uintptr_t)unit[i] + S_DRAW_Y));
                                        bullet_create(xx + 16, yy + 16, B_HEAL);//create heal effect
                                        if (shp >= 3)
                                            ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(xx + 16, yy + 16, SS_HEAL);//heal sound
                                    }
                                }
                                else i = units;
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

bool devotion_aura(int* trg, byte id)
{
    byte tid = *((byte*)((uintptr_t)trg + S_ID));//unit id
    if (tid < U_FARM)//not buildings
    {
        byte x = *((byte*)((uintptr_t)trg + S_X));
        byte y = *((byte*)((uintptr_t)trg + S_Y));
        set_region((int)x - 5, (int)y - 5, (int)x + 5, (int)y + 5);//set region around myself
        find_all_alive_units(id);
        sort_in_region();
        byte o = *((byte*)((uintptr_t)trg + S_OWNER));
        for (int ui = 0; ui < 16; ui++)
        {
            if (!check_ally(o, ui))
                sort_stat(S_OWNER, ui, CMP_NEQ);
        }
        return units != 0;
    }
    else
        return false;
}

void bullet_disp_fires()
{
    int* p = (int*)BULLETS_MASSIVE;//pointer to bullets
    p = (int*)(*p);
    int k = *(int*)BULLETS_NUMBER;//unit number
    while (k > 0)
    {
        bool f = (((*((byte*)((uintptr_t)p + 52)) == B_LIGHT_FIRE)
            || (*((byte*)((uintptr_t)p + 52)) == B_HEAVY_FIRE))//both fires
            && (*((byte*)((uintptr_t)p + 58)) == 1));//user info
        if (f)
        {
            byte life = *((byte*)((uintptr_t)p + 56));
            if ((life > 0) && ((life < 180) && ((life % 20) == 0)))((void (*)(int*, int))0x00410520)(p, 1);//damage_area(bullet, hit_air); every 20 tiks
            if (life > 0)life--;
            char buf[] = "\x0";
            buf[0] = life;
            PATCH_SET((char*)((uintptr_t)p + 56), buf);//56 bullet life (WORD)
            if (life == 0)
            {
                buf[0] = 1;
                PATCH_SET((char*)((uintptr_t)p + 53), buf);//53 bullet flags
            }
            else
            {
                buf[0] = 4;
                PATCH_SET((char*)((uintptr_t)p + 53), buf);//53 bullet flags
            }
        }
        p = (int*)((int)p + 0x40);
        k--;
    }
}

void breath_ai()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = (*((byte*)((uintptr_t)p + S_ID)) == U_GRIFON);
                if (f)
                {
                    if (!check_unit_dead(p))
                    {
                        byte od = *((byte*)((uintptr_t)p + S_ORDER));
                        if (od != ORDER_SPELL_RAISEDEAD)
                        {
                            byte o = *((byte*)((uintptr_t)p + S_OWNER));
                            byte co = *(byte*)(CONTROLER_TYPE + o);
                            if (co == C_COMP)
                            {
                                byte mp = *((byte*)((uintptr_t)p + S_MANA));
                                if (mp >= BREATH_MANACOST)
                                {
                                    int* trg = NULL;
                                    byte x = *((byte*)((uintptr_t)p + S_X));
                                    byte y = *((byte*)((uintptr_t)p + S_Y));
                                    set_region((int)x - 6, (int)y - 6, (int)x + 6, (int)y + 6);//set region around myself
                                    find_all_alive_units(ANY_UNITS);
                                    sort_in_region();
                                    sort_hidden();
                                    for (int ui = 0; ui < 16; ui++)
                                    {
                                        if (check_ally(o, ui))//only not allied
                                            sort_stat(S_OWNER, ui, CMP_NEQ);
                                    }
                                    if (units != 0)
                                    {
                                        trg = unit[(((int (*)())F_NET_RANDOM)() % units)];
                                    }
                                    set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12);//set region around myself
                                    find_all_alive_units(ANY_UNITS);
                                    sort_stat(S_ID, U_PEON, CMP_SMALLER_EQ);
                                    sort_stat(S_ID, U_PEASANT, CMP_BIGGER_EQ);
                                    sort_in_region();
                                    sort_hidden();
                                    for (int ui = 0; ui < 16; ui++)
                                    {
                                        if (check_ally(o, ui))//only not allied
                                            sort_stat(S_OWNER, ui, CMP_NEQ);
                                    }
                                    if (units != 0)
                                    {
                                        trg = unit[(((int (*)())F_NET_RANDOM)() % units)];
                                    }
                                    if (trg)
                                    {
                                        byte tid = *((byte*)((uintptr_t)trg + S_ID));
                                        byte tx = *((byte*)((uintptr_t)trg + S_X));
                                        byte ty = *((byte*)((uintptr_t)trg + S_Y));
                                        if (tid >= U_FARM)
                                        {
                                            tx++;
                                            ty++;
                                        }
                                        char buf[] = "\x0";
                                        buf[0] = ORDER_SPELL_RAISEDEAD;
                                        PATCH_SET((char*)GW_ACTION_TYPE, buf);

                                        int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_SPELL_RAISEDEAD);//orders functions
                                        ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(p, tx, ty, NULL, ord);//original war2 order

                                        buf[0] = 0;
                                        PATCH_SET((char*)GW_ACTION_TYPE, buf);
                                    }
                                }
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void make_rain()
{
    if (can_rain)
    {
        for (int i = 0; i < raindrops_amount; i++)
        {
            if (raindrops[i].l != 0)
            {
                int t = 0;
                int ms = raindrops_speed;
                while (ms > 0)
                {
                    t = raindrops[i].x2;
                    raindrops[i].x2 += abs(raindrops[i].x2 - raindrops[i].x1);
                    raindrops[i].x1 = t;
                    ms--;
                }
                ms = raindrops_speed;
                while (ms > 0)
                {
                    t = raindrops[i].y2;
                    raindrops[i].y2 += abs(raindrops[i].y2 - raindrops[i].y1);
                    raindrops[i].y1 = t;
                    ms--;
                }
                if (raindrops[i].l > 0)raindrops[i].l--;
                if ((raindrops[i].x1 >= m_maxx) || (raindrops[i].y1 >= m_maxy))raindrops[i].l = 0;
            }
        }

        int n = 1 + rand() % raindrops_density;
        int last = 0;
        for (int k = 0; k < n; k++)
        {
            int c = 0;
            for (int i = last; i < raindrops_amount; i++)
            {
                last = i;
                if (raindrops[i].l == 0)
                {
                    c = i;
                    break;
                }
            }
            if (last == (raindrops_amount - 1))
            {
                c = rand() % raindrops_amount;
            }
            if (rand() % 2 == 0)
            {
                raindrops[c].x1 = m_minx + (rand() % my_max(1, (m_maxx - m_minx - raindrops_size)));
                raindrops[c].y1 = m_miny;
                raindrops[c].x2 = raindrops[c].x1 + raindrops_size_x + (rand() % my_max(1, raindrops_align_x));
                raindrops[c].y2 = raindrops[c].y1 + raindrops_size_y + (rand() % my_max(1, raindrops_align_y));
            }
            else
            {
                raindrops[c].x1 = m_minx;
                raindrops[c].y1 = m_miny + (rand() % my_max(1, (m_maxy - m_miny - raindrops_size)));
                raindrops[c].x2 = raindrops[c].x1 + raindrops_size_x + (rand() % my_max(1, raindrops_align_x));
                raindrops[c].y2 = raindrops[c].y1 + raindrops_size_y + (rand() % my_max(1, raindrops_align_y));
            }
            raindrops[c].l = 20 + (rand() % my_max(1, (m_maxy / my_max(1, (raindrops_size / 2)))));
        }

        if (raindrops_thunder != 0)
        {
            if (raindrops_thunder_timer > 0)raindrops_thunder_timer--;
            if (raindrops_thunder_timer == 0)
            {
                if (raindrops_thunder_gradient == THUNDER_GRADIENT)
                {
                    save_palette();
                }
                if (raindrops_thunder_gradient > 0)raindrops_thunder_gradient--;
                change_palette(raindrops_thunder_gradient > (THUNDER_GRADIENT / 2));
                if (raindrops_thunder_gradient == (THUNDER_GRADIENT / 2))
                {
                    ((void (*)(WORD))F_WAR2_SOUND_PLAY)(81);
                }
                if (raindrops_thunder_gradient == 0)
                {
                    raindrops_thunder_timer = (raindrops_thunder / 2) + (rand() % (1 + (raindrops_thunder / 2)));
                    raindrops_thunder_gradient = THUNDER_GRADIENT;
                    reset_palette();
                }
            }
        }
    }
}

PROC g_proc_0045271B;
void update_spells()
{
    ((void (*)())g_proc_0045271B)();//original
    // this function called every game tick
    // you can place your self-writed functions here in the end if need

    make_rain();
    breath_ai();
    bullet_disp_fires();

    if (A_portal)portal();
    if (A_autoheal)paladin();

	if (saveload_fixed)tech_reinit();
    if (ai_fixed)
    {
		unstuk();
		//goldmine_ai();
        sap_behaviour();
    }
	if (vizs_n > 0)
    {
        for (int i = 0; i < vizs_n; i++)
        {
            viz_area(vizs_areas[i].x, vizs_areas[i].y, vizs_areas[i].p, vizs_areas[i].s);
        }
    }

    char buf[] = "\x0";
    buf[0] = 0;
    if (get_val(LUMBERMILL, *(byte*)LOCAL_PLAYER) != 0)
        buf[0] = 25;
    if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_DD)) != 0)
        buf[0] += 50;
    PATCH_SET((char*)TEXT_PRODUCTION_LUMBER_TH, buf);//lumber amount
    PATCH_SET((char*)TEXT_PRODUCTION_LUMBER_MILL, buf);//lumber amount mill

    ((void (*)(int, int, int, int))F_INVALIDATE)(0, 0, m_screen_w, m_screen_h);//screen redraw
}

void seq_change(int* u, byte tt)
{
    byte t = tt;
    if (t == 1)
    {
        byte t = *((byte*)((uintptr_t)u + S_ANIMATION_TIMER));
        byte id = *((byte*)((uintptr_t)u + S_ID));
        byte a = *((byte*)((uintptr_t)u + S_ANIMATION));
        byte f = *((byte*)((uintptr_t)u + S_FRAME));
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        if ((id == U_ARCHER) || (id == U_RANGER))
        {
            if (a == ANIM_ATTACK)
            {
                if (t > 1)t -= 1;
                set_stat(u, 1, S_ATTACK_COUNTER);
            }
            set_stat(u, t, S_ANIMATION_TIMER);
        }
        else if (id == U_FOOTMAN)
        {
            if (a == ANIM_MOVE)
            {
                byte pf = *((byte*)((uintptr_t)u + S_PEON_FLAGS));
                if (pf & PEON_HARVEST_GOLD)t += 1;
            }
            set_stat(u, t, S_ANIMATION_TIMER);
        }
        else if ((id == U_KNIGHT) || (id == U_PALADIN))
        {
            if (a == ANIM_MOVE)
            {
                if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_BLOOD)) != 0)
                {
                    if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_RUNES)) != 0)
                    {
                        t = 1;
                    }
                    else
                    {
                        //default knight
                    }
                }
                else
                {
                    //if (f == 0)t += 1;
                    if (f == 10)t += 1;
                }
            }
            set_stat(u, t, S_ANIMATION_TIMER);
        }
        else if (id == U_GRIFON)
        {
            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_UNHOLY)) != 0)
            {
                if (a == ANIM_MOVE)
                {
                    if (f == 0)if (t > 1)t -= 1;
                }
                if (a == ANIM_ATTACK)
                {
                    if (t > 1)t -= 1;
                    set_stat(u, 1, S_ATTACK_COUNTER);
                }
            }
            set_stat(u, t, S_ANIMATION_TIMER);
        }
        else if (id == U_KARGATH)
        {
            if (a == ANIM_MOVE)
            {
                t += 3;
            }
            set_stat(u, t, S_ANIMATION_TIMER);
        }
        
        int pflag = *(int*)(UNIT_GLOBAL_FLAGS + id * 4);
        if (pflag & IS_FLESHY)
        {
            if ((id != U_PEASANT) && (id != U_PEON))
            {
                byte k = *((byte*)((uintptr_t)u + S_PEON_BUILD));
                if (k > 0) 
                {
                    k--;
                    set_stat(u, k, S_PEON_BUILD);
                    if ((a == ANIM_MOVE) || (a == ANIM_ATTACK))set_stat(u, 255, S_ANIMATION_TIMER);
                }
            }
        }
    }
}

#define L_SPELL_21 21
#define UG_SP_52 52

PROC g_proc_004522B9;
int seq_run(int* u)
{
    byte t = *((byte*)((uintptr_t)u + S_ANIMATION_TIMER));
    byte id = *((byte*)((uintptr_t)u + S_ID));
    int original = 0;
    original = ((int (*)(int*))g_proc_004522B9)(u);//original
    seq_change(u, t);
    if (id == U_BALLISTA)
    {
        byte mp = *((byte*)((uintptr_t)u + S_MANA));
        if (mp >= 20)
        {
            byte a = *((byte*)((uintptr_t)u + S_ANIMATION));
            if (a == ANIM_ATTACK)
            {
                byte nod = *((byte*)((uintptr_t)u + S_NEXT_ORDER));
                if (nod != ORDER_NONE)
                {
                    byte o = *((byte*)((uintptr_t)u + S_OWNER));
                    WORD ac = *((WORD*)((uintptr_t)u + S_ATTACK_COUNTER));
                    //if (ac < 175)
                    if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_SPELL_21)) != 0)
                    {
                        while (original == 0)
                        {
                            byte t = *((byte*)((uintptr_t)u + S_ANIMATION_TIMER));
                            original = ((int (*)(int*))g_proc_004522B9)(u);//original
                            seq_change(u, t);
                        }
                        if ((ac < 175) && (ac > 100))
                        {
                            mp -= 20;
                            set_stat(u, mp, S_MANA);
                        }
                        set_stat(u, 1, S_ATTACK_COUNTER);
                    }
                }
            }
        }
    }
    return original;
}

PROC g_proc_00409F3B, //action
g_proc_0040AF70, g_proc_0040AF99, //demolish
g_proc_0041038E, g_proc_00410762, //bullet
g_proc_004428AD;//spell
char fdmg = 0;//final damage
void damage(int* atk, int* trg, char dmg)
{
    fdmg = dmg;
    if ((trg != NULL) && (atk != NULL))
    {
        if (!check_unit_dead(trg))
        {
            byte aid = *((byte*)((uintptr_t)atk + S_ID));//attacker id
            byte tid = *((byte*)((uintptr_t)trg + S_ID));//target id
            byte dmg2 = dmg;//can deal damage by default
            int i = 0;
            while (i < 255)
            {
                if ((tid == ut[i]) && (aid != ua[i]))
                {
                    dmg2 = 0;//canot deal damage
                }
                if ((tid == ut[i]) && (aid == ua[i]))//check if only some certain units can attack that unit
                {
                    dmg2 = dmg;//can deal damage
                    i = 255;
                }
                i++;
                if (ua[i] == 255)//pairs must go in a row
                {
                    i = 255;
                }
            }
            if (*((WORD*)((uintptr_t)trg + S_SHIELD)) != 0)dmg2 = 0;
            fdmg = dmg2;
            if (fdmg != 0)
            {
                if (tid == U_FOOTMAN)
                {
                    byte to = *((byte*)((uintptr_t)trg + S_OWNER));
                    to = *(byte*)(CONTROLER_TYPE + to);
                    if (to == C_COMP)
                    {
                        byte pf = *((byte*)((uintptr_t)trg + S_PEON_FLAGS));
                        pf |= PEON_HARVEST_GOLD;
                        set_stat(trg, pf, S_PEON_FLAGS);
                    }
                    byte pf = *((byte*)((uintptr_t)trg + S_PEON_FLAGS));
                    if (pf & PEON_HARVEST_GOLD)
                    {
                        dmg2 = dmg2 * 0.75;//25% block
                        fdmg = (dmg2 == 0) ? 1 : dmg2;
                    }
                }
                bool f = false;
                for (int i = 0; i < 255; i++)
                {
                    if ((m_devotion[i] != 255) && (!f))
                        f = devotion_aura(trg, m_devotion[i]);
                    else
                        i = 256;
                }
                if (f)//defence
                {
                    dmg2 = fdmg;
                    if (dmg2 > 3)dmg2 -= 3;
                    else dmg2 = 0;
                    fdmg = dmg2;
                }
            }
            WORD hp = *((WORD*)((uintptr_t)trg + S_HP));//unit hp
            if ((tid < U_FARM) && (fdmg >= hp))
            {
                if ((aid != U_HTRANSPORT) || (aid != U_OTRANSPORT))
                {
                    bool killed = true;
                    if ((tid == U_KNIGHT) || (tid == U_PALADIN))
                    {
                        byte to = *((byte*)((uintptr_t)trg + S_OWNER));
                        if (((*(DWORD*)(SPELLS_LEARNED + 4 * to) & (1 << L_BLOOD)) != 0) && ((*(DWORD*)(SPELLS_LEARNED + 4 * to) & (1 << L_RUNES)) != 0))
                        {
                            byte pf = *((byte*)((uintptr_t)trg + S_PEON_FLAGS));
                            if ((pf & PEON_HARVEST_GOLD) == 0)
                            {
                                pf |= PEON_HARVEST_GOLD;
                                set_stat(trg, pf, S_PEON_FLAGS);
                                set_stat(trg, 20, S_HP);
                                killed = false;
                                fdmg = 0;
                                ((void (*)())F_STATUS_REDRAW)();//status redraw
                            }
                        }
                    }
                    if (tid == U_RANGER)
                    {
                        byte to = *((byte*)((uintptr_t)trg + S_OWNER));
                        if (*(DWORD*)(GB_SCOUTING + to) != 0)
                        {
                            byte pf = *((byte*)((uintptr_t)trg + S_PEON_FLAGS));
                            if ((pf & PEON_HARVEST_LUMBER) == 0)
                            {
                                pf |= PEON_HARVEST_LUMBER;
                                set_stat(trg, pf, S_PEON_FLAGS);
                                set_stat(trg, 15, S_HP);
                                killed = false;
                                fdmg = 0;
                                ((void (*)())F_STATUS_REDRAW)();//status redraw
                            }
                        }
                    }
                    if (killed)
                    {
                        byte k = *((byte*)((uintptr_t)atk + S_KILLS));
                        if (k < 255)k += 1;
                        set_stat(atk, (int)k, S_KILLS);
                    }
                }
            }
        }
    }
}

void damage1(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_00409F3B)(atk, trg, fdmg);
}

void damage2(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_0041038E)(atk, trg, fdmg);
}

void damage3(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_0040AF70)(atk, trg, fdmg);
}

void damage4(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_0040AF99)(atk, trg, fdmg);
}

void damage5(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_00410762)(atk, trg, fdmg);
}

void damage6(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_004428AD)(atk, trg, fdmg);
}

void tower_set_target(int* p, int x, int y)
{
    set_stat(p, 0, S_RETARGET_X1 - 2);
    set_stat(p, 0, S_RETARGET_X1 - 1);
    int* u = NULL;
    set_region(x - 3, y - 3, x, y);
    find_all_alive_units(ANY_BUILDING_4x4);
    sort_in_region();
    sort_hidden();
    sort_attack_can_hit(p);
    if (units != 0)u = unit[0];
    set_region(x - 2, y - 2, x, y);
    find_all_alive_units(ANY_BUILDING_3x3);
    sort_in_region();
    sort_hidden();
    sort_attack_can_hit(p);
    if (units != 0)u = unit[0];
    set_region(x - 1, y - 1, x, y);
    find_all_alive_units(ANY_BUILDING_2x2);
    sort_in_region();
    sort_hidden();
    sort_attack_can_hit(p);
    if (units != 0)u = unit[0];
    set_region(x, y, x, y);
    find_all_alive_units(ANY_UNITS);
    sort_in_region();
    sort_hidden();
    sort_attack_can_hit(p);
    if (units != 0)u = unit[0];
    if (u)
    {
        int fx = ((int (*)(int*, int))F_UNIT_FIXUP)(u, 1);//fixup save
        set_stat(p, (int)fx % 256, S_RETARGET_X1 - 2);
        set_stat(p, (int)fx / 256, S_RETARGET_X1 - 1);
    }
}

PROC g_proc_0043BAE1;
void rc_snd(int* p)
{
    if (*((byte*)((uintptr_t)p + S_ID)) < U_FARM)
        ((void (*)(int*))g_proc_0043BAE1)(p);
}

PROC g_proc_0043B943;
void rc_build_click(int* p, int x, int y, int* t, int a)
{
    byte id = *(byte*)((uintptr_t)p + S_ID);
    if (id >= U_FARM)
    {
        if ((id == U_HARROWTOWER) || (id == U_OARROWTOWER)
            || (id == U_HCANONTOWER) || (id == U_OCANONTOWER)
            || (id == U_HTOWER) || (id == U_OTOWER))
        {
            if (A_tower)
                tower_set_target(p, x, y);
        }
        else
        {
            if (A_rally)
            {
                set_stat(p, x | 128, S_RETARGET_X1 - 2);
                set_stat(p, y, S_RETARGET_X1 - 1);
                set_stat(p, 0, S_RETARGET_ORDER - 1);
                bool h = false;
                set_region(x - 3, y - 3, x, y);
                find_all_alive_units(U_MINE);
                sort_in_region();
                if (units != 0)h = true;
                find_all_alive_units(ANY_BUILDING);
                sort_stat(S_ID, U_HPLATFORM, CMP_BIGGER_EQ);
                sort_stat(S_ID, U_OPLATFORM, CMP_SMALLER_EQ);
                sort_in_region();
                if (units != 0)h = true;

                struct GPOINT
                {
                    WORD x;
                    WORD y;
                };
                GPOINT l;
                l.x = (WORD)x;
                l.y = (WORD)y;
                int tr1 = ((int (*)(GPOINT*))F_TILE_IS_TREE)(&l);
                int tr2 = ((int (*)(GPOINT*))F_TILE_IS_CHOPPING_TREE)(&l);
                if ((tr1 == 1) || (tr2 == 1))h = true;
                if (h)set_stat(p, 1, S_RETARGET_ORDER - 1);
            }
        }
    }
    else
        ((void (*)(int*, int, int, int*, int))g_proc_0043B943)(p, x, y, t, a);//original
}

void rc_jmp(bool b)
{
    if (b)
    {
        char r[] = "\xf\x84\xa2\x0\x0\x0";
        PATCH_SET((char*)RIGHT_CLICK_1, r);
        patch_ljmp((char*)RIGHT_CLICK_CODE_CAVE, (char*)RIGHT_CLICK_2);
    }
    else
    {
        char r[] = "\xf\x84\x8b\x0\x0\x0";
        PATCH_SET((char*)RIGHT_CLICK_1, r);
    }
}

void brclik(bool b)
{
    if (b)
    {
        char r[] = "\x90\x90\x90\x90\x90\x90";
        PATCH_SET((char*)RIGHT_CLICK_ALLOW_BUILDINGS, r);
        rc_jmp(true);
    }
    else
    {
        char r[] = "\xf\x84\x26\x01\x0\x0";
        PATCH_SET((char*)RIGHT_CLICK_ALLOW_BUILDINGS, r);
        rc_jmp(false);
    }
}

PROC g_proc_0040AFBF;
int* tower_find_attacker(int* p)
{
    int* tr = NULL;
    byte id = *((byte*)((uintptr_t)p + S_ID));
    if ((id == U_HARROWTOWER) || (id == U_OARROWTOWER) || (id == U_HCANONTOWER) || (id == U_OCANONTOWER))
    {
        byte a1 = *((byte*)((uintptr_t)p + S_RETARGET_X1 - 2));
        byte a2 = *((byte*)((uintptr_t)p + S_RETARGET_X1 - 1));
        tr = (int*)(a1 + 256 * a2);
        tr = ((int* (*)(int*, int))F_UNIT_FIXUP)(tr, 0);//fixup load
        if (tr)
        {
            if (!check_unit_near_death(tr) && !check_unit_dead(tr) && !check_unit_hidden(tr))
            {
                int a = ((int(*)(int*, int*))F_ATTACK_CAN_HIT)(p, tr);
                if (a != 0)
                {
                    byte id = *((byte*)((uintptr_t)tr + S_ID));
                    byte szx = *(byte*)(UNIT_SIZE_TABLE + 4 * id);
                    byte szy = *(byte*)(UNIT_SIZE_TABLE + 4 * id + 2);
                    byte idd = *((byte*)((uintptr_t)p + S_ID));
                    byte rng = *(byte*)(UNIT_RANGE_TABLE + idd);
                    byte ms = *(byte*)MAP_SIZE;
                    byte xx = *((byte*)((uintptr_t)tr + S_X));
                    byte yy = *((byte*)((uintptr_t)tr + S_Y));
                    byte x1 = *((byte*)((uintptr_t)p + S_X));
                    byte y1 = *((byte*)((uintptr_t)p + S_Y));
                    byte x2 = x1;
                    byte y2 = y1;
                    if (x1 < rng)x1 = 0;
                    else x1 -= rng;
                    if (y1 < rng)y1 = 0;
                    else y1 -= rng;
                    if ((x2 + rng + 1) > ms)x2 = ms;
                    else x2 += rng + 1;
                    if ((y2 + rng + 1) > ms)y2 = ms;
                    else y2 += rng + 1;
                    if (!((xx >= x1) && (xx <= x2) && (yy >= y1) && (yy <= y2)))
                        tr = NULL;
                }
            }
            else
                tr = NULL;
        }
    }
    if (!tr)
        return ((int* (*)(int*))g_proc_0040AFBF)(p);//original
    else
    {
        return tr;
    }
}

PROC g_proc_0040DF71;
int* bld_unit_create(int a1,int a2,int a3,byte a4,int* a5)
{
    int *b = (int*)*(int*)UNIT_RUN_UNIT_POINTER;
    int* u = ((int* (*)(int, int, int, byte, int*))g_proc_0040DF71)(a1, a2, a3, a4, a5);
    if (b)
    {
        if (u)
        {
            byte idd = *((byte*)((uintptr_t)u + S_ID));
            if (idd == U_DANATH)
            {
                find_all_alive_units(U_DANATH);
                if (units > 1)
                {
                    unit_remove(u);
                    return NULL;
                }
                else set_stat(u, P_GREEN, S_COLOR);
            }
            byte x = *((byte*)((uintptr_t)b + S_RETARGET_X1 - 2));
            byte y = *((byte*)((uintptr_t)b + S_RETARGET_X1 - 1));
            byte ro = *((byte*)((uintptr_t)b + S_RETARGET_ORDER - 1));
            byte bp = x & 128;
            if (bp != 0)
            {
                x &= ~128;
                byte uid = *((byte*)((uintptr_t)u + S_ID));
                byte o = ORDER_ATTACK_AREA;
                if ((uid == U_PEON) || (uid == U_PEASANT) || (uid == U_HTANKER) || (uid == U_OTANKER))
                {
                    if (ro == 1)o = ORDER_HARVEST;
                }
                give_order(u, x, y, o);
                set_stat(u, x, S_RETARGET_X1);
                set_stat(u, y, S_RETARGET_Y1);
                set_stat(u, o, S_RETARGET_ORDER);
            }
            if (ai_fixed)
            {
                byte o = *((byte*)((uintptr_t)u + S_OWNER));
                byte m = *((byte*)((uintptr_t)u + S_MANA));
                if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                {
                    if (m = 0x55)//85 default starting mana
                    {
                        char buf[] = "\xA0";//160
                        PATCH_SET((char*)u + S_MANA, buf);
                    }
                }
                byte id = *((byte*)((uintptr_t)u + S_ID));
                if (id == U_GRIFON)
                {
                    if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                    {
                        char buf[] = "\xC0";//192
                        PATCH_SET((char*)u + S_MANA, buf);
                    }
                    else
                    {
                        char buf[] = "\x80";//128
                        PATCH_SET((char*)u + S_MANA, buf);
                    }
                }
            }
        }
    }
    return u;
}

PROC g_proc_00451728;
void unit_kill_deselect(int* u)
{
    int* ud = u;
    ((void (*)(int*))g_proc_00451728)(u);//original
    if (ai_fixed)
    {
        for (int i = 0; i < 16; i++)
        {
            int* p = (int*)(UNITS_LISTS + 4 * i);
            if (p)
            {
                p = (int*)(*p);
                while (p)
                {
                    byte id = *((byte*)((uintptr_t)p + S_ID));
                    bool f = ((id == U_HARROWTOWER) || (id == U_OARROWTOWER)
                        || (id == U_HCANONTOWER) || (id == U_OCANONTOWER));
                    bool f2 = ((id == U_DWARWES) || (id == U_GOBLINS));
                    if (f)
                    {
                        if (!check_unit_dead(p) && check_unit_complete(p))
                        {
                            byte a1 = *((byte*)((uintptr_t)p + S_RETARGET_X1));
                            byte a2 = *((byte*)((uintptr_t)p + S_RETARGET_X1 + 1));
                            byte a3 = *((byte*)((uintptr_t)p + S_RETARGET_X1 + 2));
                            byte a4 = *((byte*)((uintptr_t)p + S_RETARGET_X1 + 3));
                            int* tr = (int*)(a1 + 256 * a2 + 256 * 256 * a3 + 256 * 256 * 256 * a4);
                            if (tr == ud)
                            {
                                set_stat(p, 0, S_RETARGET_X1);
                                set_stat(p, 0, S_RETARGET_X1 + 1);
                                set_stat(p, 0, S_RETARGET_X1 + 2);
                                set_stat(p, 0, S_RETARGET_X1 + 3);
                            }
                        }
                    }
                    if (f2)
                    {
                        if (!check_unit_dead(p))
                        {
                            byte a1 = *((byte*)((uintptr_t)p + S_ORDER_UNIT_POINTER));
                            byte a2 = *((byte*)((uintptr_t)p + S_ORDER_UNIT_POINTER + 1));
                            byte a3 = *((byte*)((uintptr_t)p + S_ORDER_UNIT_POINTER + 2));
                            byte a4 = *((byte*)((uintptr_t)p + S_ORDER_UNIT_POINTER + 3));
                            int* tr = (int*)(a1 + 256 * a2 + 256 * 256 * a3 + 256 * 256 * 256 * a4);
                            if (tr == ud)
                            {
                                set_stat(p, 0, S_ORDER_UNIT_POINTER);
                                set_stat(p, 0, S_ORDER_UNIT_POINTER + 1);
                                set_stat(p, 0, S_ORDER_UNIT_POINTER + 2);
                                set_stat(p, 0, S_ORDER_UNIT_POINTER + 3);
                                give_order(p, 0, 0, ORDER_STOP);
                            }
                        }
                    }
                    p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
                }
            }
        }
    }
}

PROC g_proc_00452939;
void grow_struct_count_tables(int* u)
{
    ((void (*)(int*))g_proc_00452939)(u);//original
}

void allow_table(byte p, int t, byte n, byte a)
{
    if (t == 0)t = ALLOWED_UNITS + (4 * p) + (n / 8);
    else if (t == 1)t = ALLOWED_UPGRADES + (4 * p) + (n / 8);
    else if (t == 2)t = ALLOWED_SPELLS + (4 * p) + (n / 8);
    else t = SPELLS_LEARNED + (4 * p) + (n / 8);
    byte b = *(byte*)t;
    if (a == 1)b |= (1 << (n % 8));
    else b &= (~(1 << (n % 8)));
    char buf[] = "\xff";
    buf[0] = b;
    PATCH_SET((char*)t, buf);
}

void draw_stats_fix(bool b)
{
    if (b)
    {
        char buf[] = "\xa0\x5b";
        PATCH_SET((char*)DEMON_STATS_DRAW, buf);//demon
        PATCH_SET((char*)CRITTER_STATS_DRAW, buf);//critter
    }
    else
    {
        char buf[] = "\xf0\x57";
        PATCH_SET((char*)DEMON_STATS_DRAW, buf);//demon
        PATCH_SET((char*)CRITTER_STATS_DRAW, buf);//critter
    }
}

void send_cheat(byte c)
{
    int b = *(int*)CHEATBITS;
    if ((b & (1 << c)) != 0)
        b &= ~(1 << c);
    else
        b |= 1 << c;
    ((void (*)(int))F_SEND_CHEAT_PACKET)(b);
}

void rec_autoheal()
{
    byte p = *(byte*)PACKET_PLAYER;//player
    byte local = *(byte*)LOCAL_PLAYER;
    byte b = *(byte*)(SPELLS_LEARNED + 4 * p);
    byte sp = GREATER_HEAL;
    if ((b & (1 << sp)) != 0)
        b &= ~(1 << sp);
    else
        b |= 1 << sp;
    char buf[] = "\x0";
    buf[0] = b;
    PATCH_SET((char*)(SPELLS_LEARNED + 4 * p), buf);

    if ((*(byte*)(SPELLS_LEARNED + 4 * (*(byte*)LOCAL_PLAYER)) & (1 << L_GREATER_HEAL)) != 0)
    {
        churc[20 * 3 + 2] = '\x5b';//icon
        if (p == local)
        {
            char msg[] = "autoheal\x5 enabled";
            show_message(10, msg);
        }
    }
    else
    {
        churc[20 * 3 + 2] = '\x6d';//icon
        if (p == local)
        {
            char msg[] = "autoheal\x3 disabled";
            show_message(10, msg);
        }
    }
    ((void (*)())F_STATUS_REDRAW)();
}

PROC g_proc_0045614E;
void receive_cheat_single(int c, int a1)
{
    char buf[] = "\x0";
    buf[0] = *(byte*)LOCAL_PLAYER;
    PATCH_SET((char*)PACKET_PLAYER, buf);
    bool f = true;
    if ((c & (1 << 8)) != 0)//8 - autoheal
    {
        rec_autoheal();
        f = false;
    }
    if ((c & (1 << 9)) != 0)//9 - attack peons
    {
        //rec_peons();
        f = false;
    }
    if (f)
    {
        c &= 141439;//0 on screen;1 fast build;2 god;3 magic;4 upgrades;5 money;6 finale;11 hatchet;17 disco;13 tigerlily
        if (*(byte*)LEVEL_OBJ == LVL_HUMAN10)c = 0;
        ((void (*)(int, int))g_proc_0045614E)(c, a1);//orig
    }
    else
    {
        char buf[] = "\x0";
        PATCH_SET((char*)PLAYER_CHEATED, buf);
    }
}

void button_autoheal(int)
{
    send_cheat(8);
    if ((*(byte*)(SPELLS_LEARNED + 4 * (*(byte*)LOCAL_PLAYER)) & (1 << L_GREATER_HEAL)) != 0)
        churc[20 * 3 + 2] = '\x5b';//icon
    else
        churc[20 * 3 + 2] = '\x6d';//icon
    ((void (*)())F_STATUS_REDRAW)();
}

void autoheal(bool b)
{
    if (b)
    {
        if ((*(byte*)(SPELLS_LEARNED + 4 * (*(byte*)LOCAL_PLAYER)) & (1 << L_GREATER_HEAL)) != 0)
            churc[20 * 3 + 2] = '\x5b';//icon
        else
            churc[20 * 3 + 2] = '\x6d';//icon

        void (*r1) (int) = button_autoheal;
        patch_setdword((DWORD*)(churc + (20 * 3 + 8)), (DWORD)r1);

        char b1[] = "\04\x0\x0\x0\x68\x37\x4a\x0";
        char* repf = churc;
        patch_setdword((DWORD*)(b1 + 4), (DWORD)repf);
        PATCH_SET((char*)CHURCH_BUTTONS, b1);
        A_autoheal = true;
    }
    else
    {
        char b1[] = "\03\x0\x0\x0\x68\x37\x4a\x0";
        PATCH_SET((char*)CHURCH_BUTTONS, b1);
        A_autoheal = false;
    }
}

void call_default_kill()//default kill all victory
{
    byte l = *(byte*)LOCAL_PLAYER;
    if (!slot_alive(l))lose(true);
    else
    {
        if (!check_opponents(l))win(true);
    }
}

//-------------files
const char FILES_PATH[] = ".\\RedMist\\";

void* grp_port_forest;
void* grp_port_winter;
void* grp_port_wast;
void* grp_port_swamp;
void* grp_ruka;
void* grp_arrow;
void* grp_bliz_icon;
void* grp_ace_black_icon;
void* grp_haste;
void* grp_potion;
void* grp_saw;
void* grp_arta;
void* grp_shield;
void* grp_aviary_black_icon;
void* grp_fire;

void* grp_aviary;
void* grp_aviary_black;
void* grp_aviary_green;

void* grp_ace;
void* grp_ace_black;
void* grp_foot;
void* grp_foot_shield;
void* grp_prince;
void* grp_ranger;
void* grp_ranger2;
void* grp_wizard;

void* grp_bliz;
void* grp_ballistabliz;

void* tbl_credits;
char tbl_credits_title[] = "THE MIST TURNS RED!";

void* tbl_brif1;
void* tbl_brif2;
void* tbl_brif3;
void* tbl_brif4;
void* tbl_brif5;
void* tbl_brif6;
void* tbl_brif7;
char tbl_brif_secret[] = "%!^@#(&> !*@&$ &$)()!*# ((#*$ {}!@ #))( #|}!@# (*$&*?><!$  !>!>}!@ |}!#<#@? !@#%|#@%} !#@!<> |!@$|}|";
void* tbl_end;
char tbl_brif8[] = " ";

void* tbl_task1;
void* tbl_task2;
void* tbl_task3;
void* tbl_task4;
void* tbl_task5;
void* tbl_task6;
void* tbl_task7;
char tbl_task_secret[] = "Kill 5000 orcs!";
void* tbl_task8;

void* tbl_title1;
void* tbl_title2;
void* tbl_title3;
void* tbl_title4;
void* tbl_title5;
void* tbl_title6;
void* tbl_title7;
void* tbl_title8;
void* tbl_title9;

void* tbl_name1;
void* tbl_name2;
void* tbl_name3;
void* tbl_name4;
void* tbl_name5;
void* tbl_name6;
void* tbl_name7;
void* tbl_name8;
void* tbl_name9;
void* tbl_name10;
void* tbl_name11;

void* tbl_skill1;
void* tbl_skill2;
void* tbl_skill3;
void* tbl_skill4;
void* tbl_skill5;
void* tbl_skill6;
void* tbl_skill7;
void* tbl_skill8;
void* tbl_skill9;
void* tbl_skill10;
void* tbl_skill11;

void* tbl_research1;
void* tbl_research2;
void* tbl_research3;
void* tbl_research4;
void* tbl_research5;
void* tbl_research6;
void* tbl_research7;
void* tbl_research8;
void* tbl_research9;
void* tbl_research10;
void* tbl_research11;

void* tbl_build1;

void* tbl_train1;
void* tbl_train2;

void* tbl_kills;

void* tbl_tutorial;

void* tbl_nations;

void* pud_map1;
DWORD pud_map1_size;
void* pud_map2;
DWORD pud_map2_size;
void* pud_map3;
DWORD pud_map3_size;
void* pud_map4;
DWORD pud_map4_size;
void* pud_map5;
DWORD pud_map5_size;
void* pud_map6;
DWORD pud_map6_size;
void* pud_map7;
DWORD pud_map7_size;
void* pud_map_secret;
DWORD pud_map_secret_size;
void* pud_emap1;
DWORD pud_emap1_size;
void* pud_emap2;
DWORD pud_emap2_size;
void* pud_emap3;
DWORD pud_emap3_size;
void* pud_emap4;
DWORD pud_emap4_size;
void* pud_emap5;
DWORD pud_emap5_size;
void* pud_emap6;
DWORD pud_emap6_size;
void* pud_emap7;
DWORD pud_emap7_size;
void* pud_map_tutorial;
DWORD pud_map_tutorial_size;

void* bin_menu;
DWORD bin_menu_size;
void* bin_menu_copy;
void* bin_sngl;
DWORD bin_sngl_size;
void* bin_sngl_copy;
void* bin_newcmp;
DWORD bin_newcmp_size;
void* bin_newcmp_copy;
void* bin_quit;
DWORD bin_quit_size;
void* bin_quit_copy;

void* bin_AI;
DWORD bin_AI_size;
void* bin_script;
DWORD bin_script_size;
void* bin_unitdata;
DWORD bin_unitdata_size;
void* bin_unitdato;
DWORD bin_unitdato_size;
void* bin_upgrades;
DWORD bin_upgrades_size;

void* pcx_splash;
void* pcx_splash_pal;
void* pcx_menu;
void* pcx_menu_pal;
void* pcx_end;
void* pcx_end_pal;
void* pcx_credits;
void* pcx_credits_pal;
void* pcx_act1;
void* pcx_act1_pal;
void* pcx_act2;
void* pcx_act2_pal;
void* pcx_act3;
void* pcx_act3_pal;
void* pcx_act4;
void* pcx_act4_pal;
void* pcx_act5;
void* pcx_act5_pal;
void* pcx_act6;
void* pcx_act6_pal;
void* pcx_act7;
void* pcx_act7_pal;
void* pcx_act8;
void* pcx_act8_pal;
void* pcx_b2;
void* pcx_b2_pal;
void* pcx_b3;
void* pcx_b3_pal;
void* pcx_b4;
void* pcx_b4_pal;
void* pcx_b15;
void* pcx_b15_pal;
void* pcx_b67;
void* pcx_b67_pal;

PROC g_proc_004542FB;
int grp_draw_cross(int a, int* u, void* grp, int frame)
{
    void* new_grp = NULL;
    //-------------------------------------------------
    //if level = orc1
    //if race = human
    //etc
    //new_grp = grp_runecircle;

    if (new_grp)
        return ((int (*)(int, int*, void*, int))g_proc_004542FB)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int*, void*, int))g_proc_004542FB)(a, u, grp, frame);//original
}

PROC g_proc_00454DB4;
int grp_draw_bullet(int a, int* u, void* grp, int frame)
{
    bool draw = true;
    void* new_grp = NULL;
    //-------------------------------------------------
    //44 - target
    byte id = *((byte*)((uintptr_t)u + 52));//52 - id
    int* c = (int*)*((int*)((uintptr_t)u + 48));//48 - creator
    if (id == B_BLIZZARD)
    {
        byte cid = *((byte*)((uintptr_t)c + S_ID));
        if (cid == U_MAGE)new_grp = grp_bliz;
        else if (cid == U_BALLISTA)new_grp = grp_ballistabliz;
    }
    else if (id == B_LIGHT_FIRE)
    {
        if (c)
        {
            byte cid = *((byte*)((uintptr_t)c + S_ID));
            if (cid == U_GRIFON)
            {
                byte life = *((byte*)((uintptr_t)u + 56));//56 - life
                if (life > 180)draw = false;
            }
        }
    }

    if (draw)
    {
        if (new_grp)
            return ((int (*)(int, int*, void*, int))g_proc_00454DB4)(a, u, new_grp, frame);
        else
            return ((int (*)(int, int*, void*, int))g_proc_00454DB4)(a, u, grp, frame);//original
    }
}

PROC g_proc_00454BCA;
int grp_draw_unit(int a, int* u, void* grp, int frame)
{
    void* new_grp = NULL;
    //-------------------------------------------------
    byte id = *((byte*)((uintptr_t)u + S_ID));
    if (id == U_FOOTMAN)
    {
        byte pf = *((byte*)((uintptr_t)u + S_PEON_FLAGS));
        if ((pf & PEON_HARVEST_GOLD) != 0)new_grp = grp_foot_shield;
        else new_grp = grp_foot;
    }
    else if (id == U_RANGER)
    {
        byte pf = *((byte*)((uintptr_t)u + S_PEON_FLAGS));
        if ((pf & PEON_HARVEST_GOLD) != 0)new_grp = grp_ranger2;
        else new_grp = grp_ranger;
    }
    else if (id == U_KARGATH)
    {
        if ((*(byte*)LEVEL_OBJ == LVL_HUMAN1) || (*(byte*)LEVEL_OBJ == LVL_XHUMAN1))new_grp = grp_prince;
    }
    else if (id == U_GRIFON)
    {
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        if (o == *(byte*)LOCAL_PLAYER)new_grp = grp_ace;
        else new_grp = grp_ace_black;
    }
    else if (id == U_MAGE)
    {
        new_grp = grp_wizard;
    }

    if (new_grp)
        return ((int (*)(int, int*, void*, int))g_proc_00454BCA)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int*, void*, int))g_proc_00454BCA)(a, u, grp, frame);//original
}

PROC g_proc_00455599;
int grp_draw_building(int a, int* u, void* grp, int frame)
{
    void* new_grp = NULL;
    //-------------------------------------------------
    byte id = *((byte*)((uintptr_t)u + S_ID));
    byte mp = *((byte*)((uintptr_t)u + S_MANA));
    WORD hp = *((WORD*)((uintptr_t)u + S_HP));
    WORD mhp = *(WORD*)(UNIT_HP_TABLE + 2 * id);//max hp
    if ((mp > 127) || check_unit_complete(u) || (hp >= (mhp / 2)))
    {
        if (id == U_AVIARY)
        {
            if (*(byte*)LEVEL_OBJ==LVL_ORC1)new_grp = grp_aviary;
            else
            {
                byte o = *((byte*)((uintptr_t)u + S_OWNER));
                if (o == *(byte*)LOCAL_PLAYER)new_grp = grp_aviary;
                else new_grp = grp_aviary_black;
            }
        }
        else if (id == U_DRAGONROOST)
        {
            new_grp = grp_aviary_green;
        }
    }
    //-------------------------------------------------
    if (new_grp)
        return ((int (*)(int, int*, void*, int))g_proc_00455599)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int*, void*, int))g_proc_00455599)(a, u, grp, frame);//original
}

PROC g_proc_0043AE54;
void grp_draw_building_placebox(void* grp, int frame, int a, int b)
{
    void* new_grp = NULL;
    int* peon = (int*)*(int*)LOCAL_UNITS_SELECTED;
    byte id = (*(int*)b) % 256;
    //-------------------------------------------------
    if (id == U_AVIARY)
    {
        new_grp = grp_aviary;
    }
    //-------------------------------------------------
    if (new_grp)
        ((void (*)(void*, int, int, int))g_proc_0043AE54)(new_grp, frame, a, b);
    else
        ((void (*)(void*, int, int, int))g_proc_0043AE54)(grp, frame, a, b);//original
}

int* portrait_unit;

PROC g_proc_0044538D;
void grp_portrait_init(int* a)
{
    ((void (*)(int*))g_proc_0044538D)(a);//original
    portrait_unit = (int*)*((int*)((uintptr_t)a + 0x26));
}

PROC g_proc_004453A7;//draw unit port
void grp_draw_portrait(void* grp, byte frame, int b, int c)
{
    bool f = true;
    void* new_grp = NULL;
    //-------------------------------------------------
    int* u = portrait_unit;
    if (u != NULL)
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if (id == U_GRIFON)
        {
            byte o = *((byte*)((uintptr_t)u + S_OWNER));
            if (o != *(byte*)LOCAL_PLAYER)
            {
                new_grp = grp_ace_black_icon;
                frame = 1;
                f = false;
            }
        }
        else if (id == U_AVIARY)
        {
            if (*(byte*)LEVEL_OBJ != LVL_ORC1)
            {
                byte o = *((byte*)((uintptr_t)u + S_OWNER));
                if (o != *(byte*)LOCAL_PLAYER)
                {
                    new_grp = grp_aviary_black_icon;
                    frame = *(byte*)MAP_ERA;
                    f = false;
                }
            }
        }
    }

    if (f)
    {
        byte era = *(byte*)MAP_ERA;
        if (era == 0)new_grp = grp_port_forest;
        else if (era == 1)new_grp = grp_port_winter;
        else if (era == 2)new_grp = grp_port_wast;
        else if (era == 3)new_grp = grp_port_swamp;
    }

    if (new_grp)
        return ((void (*)(void*, byte, int, int))g_proc_004453A7)(new_grp, frame, b, c);
    else
        return ((void (*)(void*, byte, int, int))g_proc_004453A7)(grp, frame, b, c);//original
}

PROC g_proc_004452B0;//draw buttons
void grp_draw_portrait_buttons(void* grp, byte frame, int b, int c)
{
    bool f = true;
    void* new_grp = NULL;
    //-------------------------------------------------
    int* u = NULL;
    //int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    for (int i = 0; ((i < 9) && (u == NULL)); i++)
    {
        u = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (u)break;
    }
    if (u != NULL)
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if ((id == U_MAGE) || (id == U_MAGE_TOWER))
        {
            if (frame == 105)
            {
                new_grp = grp_bliz_icon;
                frame = 1;
                f = false;
            }
        }
        else if ((id == U_ARCHER) || (id == U_RANGER))
        {
            if (frame == 1)
            {
                new_grp = grp_arrow;
                f = false;
            }
            if (frame == 2)
            {
                new_grp = grp_potion;
                frame = 1;
                f = false;
            }
        }
        else if (id == U_AVIARY)
        {
            if (frame == 1)
            {
                new_grp = grp_haste;
                f = false;
            }
            if (frame == 2)
            {
                new_grp = grp_fire;
                frame = 1;
                f = false;
            }
        }
        else if (id == U_HLUMBER)
        {
            if (frame == 2)
            {
                new_grp = grp_saw;
                f = false;
            }
        }
        else if ((id == U_HSMITH) || (id == U_BALLISTA))
        {
            if (frame == 3)
            {
                new_grp = grp_arta;
                f = false;
            }
        }
        else if (id == U_DANATH)
        {
            if (frame == 4)
            {
                new_grp = grp_shield;
                f = false;
            }
        }
        else if (id == U_GRIFON)
        {
            if (frame == 1)
            {
                new_grp = grp_fire;
                f = false;
            }
        }
    }
    //-------------------------------------------------
    //new_grp = grp_port;

    if (frame == 155)
    {
        int* uf = NULL;
        for (int i = 0; ((i < 9) && (uf == NULL)); i++)
        {
            uf = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
            if (uf)
            {
                byte id = *((byte*)((uintptr_t)uf + S_ID));
                if (id != U_FOOTMAN)uf = NULL;
                else break;
            }
        }
        if (uf)
        {
            byte pf = *((byte*)((uintptr_t)uf + S_PEON_FLAGS));
            if ((pf & PEON_HARVEST_GOLD) != 0)
            {
                new_grp = grp_ruka;
                frame = 1;
                f = false;
            }
        }
    }

    if (f)
    {
        byte era = *(byte*)MAP_ERA;
        if (era == 0)new_grp = grp_port_forest;
        else if (era == 1)new_grp = grp_port_winter;
        else if (era == 2)new_grp = grp_port_wast;
        else if (era == 3)new_grp = grp_port_swamp;
    }

    if (new_grp)
        return ((void (*)(void*, byte, int, int))g_proc_004452B0)(new_grp, frame, b, c);
    else
        return ((void (*)(void*, byte, int, int))g_proc_004452B0)(grp, frame, b, c);//original
}

PROC g_proc_0044A65C;
int status_get_tbl(void* tbl, WORD str_id)
{
    int* u = (int*)*(int*)UNIT_STATUS_TBL;
    void* new_tbl = NULL;
    //-------------------------------------------------
    if (u != NULL)
    {
        if (str_id < U_EMPTY_BUTTONS)
        {
            byte id = *((byte*)((uintptr_t)u + S_ID));
            if (id == U_KARGATH)
            {
                if ((*(byte*)LEVEL_OBJ == LVL_HUMAN1) || (*(byte*)LEVEL_OBJ == LVL_XHUMAN1))new_tbl = tbl_name1;
                else new_tbl = tbl_name9;
                str_id = 1;
            }
            else if (id == U_GROM)
            {
                new_tbl = tbl_name8;
                str_id = 1;
            }
            else if (id == U_TERON)
            {
                new_tbl = tbl_name8;
                str_id = 1;
            }
            else if (id == U_CHOGAL)
            {
                new_tbl = tbl_name10;
                str_id = 1;
            }
            else if (id == U_HLUMBER)
            {
                new_tbl = tbl_name11;
                str_id = 1;
            }
            else if (id == U_DANATH)
            {
                new_tbl = tbl_name2;
                str_id = 1;
            }
            else if (id == U_ARCHER)
            {
                new_tbl = tbl_name3;
                str_id = 1;
            }
            else if (id == U_HDESTROYER)
            {
                new_tbl = tbl_name4;
                str_id = 1;
            }
            else if (id == U_GRIFON)
            {
                new_tbl = tbl_name6;
                str_id = 1;
            }
            else if (id == U_AVIARY)
            {
                if (*(byte*)LEVEL_OBJ == LVL_ORC1)new_tbl = tbl_name5;
                else
                {
                    byte o = *((byte*)((uintptr_t)u + S_OWNER));
                    if (o == *(byte*)LOCAL_PLAYER)new_tbl = tbl_name5;
                    else new_tbl = tbl_name7;
                }
                str_id = 1;
            }
        }
    }
    //-------------------------------------------------

    if (str_id == 416)
    {
        new_tbl = tbl_kills;
        str_id = 1;
    }

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0044A65C)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0044A65C)(tbl, str_id);//original

}

int* hover_unit;

PROC g_proc_0044AC83;
void unit_hover_get_id(int a, int* b)
{
    if (b != NULL)
    {
        byte id = *((byte*)((uintptr_t)b + 0x20));
        hover_unit = (int*)*(int*)(LOCAL_UNIT_SELECTED_PANEL + 4 * id);
    }
    else
        hover_unit = NULL;
    ((void (*)(int, int*))g_proc_0044AC83)(a, b);//original
}

PROC g_proc_0044AE27;
int unit_hover_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    int* u = hover_unit;
    if (u != NULL)
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if (id == U_KARGATH)
        {
            if ((*(byte*)LEVEL_OBJ == LVL_HUMAN1) || (*(byte*)LEVEL_OBJ == LVL_XHUMAN1))new_tbl = tbl_name1;
            else new_tbl = tbl_name9;
            str_id = 1;
        }
        else if (id == U_GROM)
        {
            new_tbl = tbl_name8;
            str_id = 1;
        }
        else if (id == U_TERON)
        {
            new_tbl = tbl_name8;
            str_id = 1;
        }
        else if (id == U_CHOGAL)
        {
            new_tbl = tbl_name10;
            str_id = 1;
        }
        else if (id == U_HLUMBER)
        {
            new_tbl = tbl_name11;
            str_id = 1;
        }
        else if (id == U_DANATH)
        {
            new_tbl = tbl_name2;
            str_id = 1;
        }
        else if (id == U_ARCHER)
        {
            new_tbl = tbl_name3;
            str_id = 1;
        }
        else if (id == U_HDESTROYER)
        {
            new_tbl = tbl_name4;
            str_id = 1;
        }
        else if (id == U_GRIFON)
        {
            new_tbl = tbl_name6;
            str_id = 1;
        }
        else if (id == U_AVIARY)
        {
            if (*(byte*)LEVEL_OBJ == LVL_ORC1)new_tbl = tbl_name5;
            else
            {
                byte o = *((byte*)((uintptr_t)u + S_OWNER));
                if (o == *(byte*)LOCAL_PLAYER)new_tbl = tbl_name5;
                else new_tbl = tbl_name7;
            }
            str_id = 1;
        }
    }
    //-------------------------------------------------
    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0044AE27)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0044AE27)(tbl, str_id);//original

}

PROC g_proc_0044AE56;
void button_description_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    int* u = NULL;
    //int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    for (int i = 0; ((i < 9) && (u == NULL)); i++)
    {
        u = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (u)break;
    }
    if (u != NULL)
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if (id == U_DANATH)
        {
            if (str_id == 1) new_tbl = tbl_skill2;
            else if (str_id == 2)
            {
                new_tbl = tbl_skill3;
                str_id = 1;
            }
        }
        else if (id == U_BALLISTA)
        {
            if (str_id == 1) new_tbl = tbl_skill4;
        }
        else if ((id == U_ARCHER)|| (id == U_RANGER))
        {
            if (str_id == 1) new_tbl = tbl_skill5;
            else if (str_id == 2)
            {
                new_tbl = tbl_skill9;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_skill11;
                str_id = 1;
            }
        }
        else if (id == U_MAGE)
        {
            if (str_id == 354)
            {
                new_tbl = tbl_skill6;
                str_id = 1;
            }
            if (str_id == 351)
            {
                new_tbl = tbl_skill10;
                str_id = 1;
            }
        }
        else if ((id == U_KNIGHT) || (id == U_PALADIN))
        {
            if (str_id == 1) new_tbl = tbl_skill7;
        }
        else if (id == U_GRIFON)
        {
            if (str_id == 1) new_tbl = tbl_skill8;
        }
        else if (id == U_PEASANT)
        {
            if (str_id == 323) 
            { 
                new_tbl = tbl_build1;
                str_id = 1;
            }
        }
        else if (id == U_AVIARY)
        {
            if (str_id == 295)
            {
                new_tbl = tbl_train1;
                str_id = 1;
            }
            else if (str_id == 1)new_tbl = tbl_research4;
            else if (str_id == 2)
            {
                new_tbl = tbl_research7;
                str_id = 1;
            }
        }
        else if ((id == U_TOWN_HALL)|| (id == U_KEEP) || (id == U_CASTLE))
        {
            if (str_id == 1)new_tbl = tbl_train2;
        }
        else if (id == U_MAGE_TOWER)
        {
            if (str_id == 377)
            {
                new_tbl = tbl_research1;
                str_id = 1;
            }
            if (str_id == 374)
            {
                new_tbl = tbl_research9;
                str_id = 1;
            }
        }
        else if (id == U_STABLES)
        {
            if (str_id == 1) new_tbl = tbl_research2;
        }
        else if (id == U_HLUMBER)
        {
            if (str_id == 338)
            {
                new_tbl = tbl_research3;
                str_id = 1;
            }
            else if (str_id == 340)
            {
                new_tbl = tbl_research11;
                str_id = 1;
            }
            else if (str_id == 1) new_tbl = tbl_research8;
        }
        else if (id == U_HBARRACK)
        {
            if (str_id == 1) new_tbl = tbl_research5;
        }
        else if (id == U_HSMITH)
        {
            if (str_id == 1) new_tbl = tbl_research6;
            else if (str_id == 2)
            {
                new_tbl = tbl_research10;
                str_id = 1;
            }
        }
    }
    //-------------------------------------------------
    //new_tbl = tbl_names;

    int* uf = NULL;
    for (int i = 0; (i < 9) && (uf == NULL); i++)
    {
        uf = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (uf)
        {
            byte id = *((byte*)((uintptr_t)uf + S_ID));
            if (id != U_FOOTMAN)break;
            else if (str_id == 1)new_tbl = tbl_skill1;
        }
    }

    if (new_tbl)
        return ((void (*)(void*, int))g_proc_0044AE56)(new_tbl, str_id);
    else
        return ((void (*)(void*, int))g_proc_0044AE56)(tbl, str_id);//original

}

PROC g_proc_0044B23D;
void button_hotkey_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    int* u = NULL;
    //int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    for (int i = 0; ((i < 9) && (u == NULL)); i++)
    {
        u = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (u)break;
    }
    if (u != NULL)
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if (id == U_DANATH)
        {
            if (str_id == 1) new_tbl = tbl_skill2;
            else if (str_id == 2)
            {
                new_tbl = tbl_skill3;
                str_id = 1;
            }
        }
        else if (id == U_BALLISTA)
        {
            if (str_id == 1) new_tbl = tbl_skill4;
        }
        else if ((id == U_ARCHER) || (id == U_RANGER))
        {
            if (str_id == 1) new_tbl = tbl_skill5;
            else if (str_id == 2)
            {
                new_tbl = tbl_skill9;
                str_id = 1;
            }
            else if (str_id == 3)
            {
                new_tbl = tbl_skill11;
                str_id = 1;
            }
        }
        else if (id == U_MAGE)
        {
            if (str_id == 354)
            {
                new_tbl = tbl_skill6;
                str_id = 1;
            }
            if (str_id == 351)
            {
                new_tbl = tbl_skill10;
                str_id = 1;
            }
        }
        else if ((id == U_KNIGHT) || (id == U_PALADIN))
        {
            if (str_id == 1) new_tbl = tbl_skill7;
        }
        else if (id == U_GRIFON)
        {
            if (str_id == 1) new_tbl = tbl_skill8;
        }
        else if (id == U_PEASANT)
        {
            if (str_id == 323)
            {
                new_tbl = tbl_build1;
                str_id = 1;
            }
        }
        else if (id == U_AVIARY)
        {
            if (str_id == 295)
            {
                new_tbl = tbl_train1;
                str_id = 1;
            }
            else if (str_id == 1)new_tbl = tbl_research4;
            else if (str_id == 2)
            {
                new_tbl = tbl_research7;
                str_id = 1;
            }
        }
        else if ((id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_CASTLE))
        {
            if (str_id == 1)new_tbl = tbl_train2;
        }
        else if (id == U_MAGE_TOWER)
        {
            if (str_id == 377)
            {
                new_tbl = tbl_research1;
                str_id = 1;
            }
            if (str_id == 374)
            {
                new_tbl = tbl_research9;
                str_id = 1;
            }
        }
        else if (id == U_STABLES)
        {
            if (str_id == 1) new_tbl = tbl_research2;
        }
        else if (id == U_HLUMBER)
        {
            if (str_id == 338)
            {
                new_tbl = tbl_research3;
                str_id = 1;
            }
            else if (str_id == 340)
            {
                new_tbl = tbl_research11;
                str_id = 1;
            }
            else if (str_id == 1) new_tbl = tbl_research8;
        }
        else if (id == U_HBARRACK)
        {
            if (str_id == 1) new_tbl = tbl_research5;
        }
        else if (id == U_HSMITH)
        {
            if (str_id == 1) new_tbl = tbl_research6;
            else if (str_id == 2)
            {
                new_tbl = tbl_research10;
                str_id = 1;
            }
        }
    }
    //-------------------------------------------------
    //new_tbl = tbl_names;

    int* uf = NULL;
    for (int i = 0; (i < 9) && (uf == NULL); i++)
    {
        uf = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (uf)
        {
            byte id = *((byte*)((uintptr_t)uf + S_ID));
            if (id != U_FOOTMAN)break;
            else if (str_id == 1)new_tbl = tbl_skill1;
        }
    }

    if (new_tbl)
        return ((void (*)(void*, int))g_proc_0044B23D)(new_tbl, str_id);
    else
        return ((void (*)(void*, int))g_proc_0044B23D)(tbl, str_id);//original

}

char tbl_kill[] = "Kill all";

PROC g_proc_004354C8;
int objct_get_tbl_custom(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    return (int)tbl_kill;
    //-------------------------------------------------
    //new_tbl = tbl_obj;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004354C8)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004354C8)(tbl, str_id);//original
}

PROC g_proc_004354FA;
int objct_get_tbl_campanign(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))new_tbl = tbl_task1;
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))new_tbl = tbl_task2;
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))new_tbl = tbl_task3;
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))new_tbl = tbl_task4;
    else if ((lvl == LVL_HUMAN5) || (lvl == LVL_XHUMAN5))new_tbl = tbl_task5;
    else if ((lvl == LVL_HUMAN6) || (lvl == LVL_XHUMAN6))new_tbl = tbl_task6;
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))new_tbl = tbl_task7;
    else if (lvl == LVL_HUMAN10)return (int)tbl_task_secret;
    else if (lvl == LVL_ORC1)new_tbl = tbl_task8;
    str_id = 1;
    //-------------------------------------------------
    //new_tbl = tbl_obj;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004354FA)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004354FA)(tbl, str_id);//original
}

PROC g_proc_004300A5;
int objct_get_tbl_briefing_task(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))new_tbl = tbl_task1;
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))new_tbl = tbl_task2;
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))new_tbl = tbl_task3;
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))new_tbl = tbl_task4;
    else if ((lvl == LVL_HUMAN5) || (lvl == LVL_XHUMAN5))new_tbl = tbl_task5;
    else if ((lvl == LVL_HUMAN6) || (lvl == LVL_XHUMAN6))new_tbl = tbl_task6;
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))new_tbl = tbl_task7;
    else if (lvl == LVL_HUMAN10)return (int)tbl_task_secret;
    else if (lvl == LVL_ORC1)new_tbl = tbl_task8;
    str_id = 1;
    //-------------------------------------------------
    //new_tbl = tbl_obj;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004300A5)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004300A5)(tbl, str_id);//original
}

PROC g_proc_004300CA;
int objct_get_tbl_briefing_title(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))new_tbl = tbl_title1;
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))new_tbl = tbl_title2;
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))new_tbl = tbl_title3;
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))new_tbl = tbl_title4;
    else if ((lvl == LVL_HUMAN5) || (lvl == LVL_XHUMAN5))new_tbl = tbl_title5;
    else if ((lvl == LVL_HUMAN6) || (lvl == LVL_XHUMAN6))new_tbl = tbl_title6;
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))new_tbl = tbl_title7;
    else if (lvl == LVL_HUMAN10)new_tbl = tbl_title8;
    else if (lvl == LVL_ORC1)new_tbl = tbl_title9;
    str_id = 1;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004300CA)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004300CA)(tbl, str_id);//original
}

PROC g_proc_004301CA;
int objct_get_tbl_briefing_text(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))new_tbl = tbl_brif1;
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))new_tbl = tbl_brif2;
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))new_tbl = tbl_brif3;
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))new_tbl = tbl_brif4;
    else if ((lvl == LVL_HUMAN5) || (lvl == LVL_XHUMAN5))new_tbl = tbl_brif5;
    else if ((lvl == LVL_HUMAN6) || (lvl == LVL_XHUMAN6))new_tbl = tbl_brif6;
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))new_tbl = tbl_brif7;
    else if (lvl == LVL_HUMAN10)return (int)tbl_brif_secret;
    else if (lvl == LVL_ORC1)return (int)tbl_brif8;
    str_id = 1;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004301CA)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004301CA)(tbl, str_id);//original
}

char story1[] = "RedMist\\storyteller\\1.wav";
char story2[] = "RedMist\\storyteller\\2.wav";
char story3[] = "RedMist\\storyteller\\3.wav";
char story4[] = "RedMist\\storyteller\\4.wav";
char story5[] = "RedMist\\storyteller\\5.wav";
char story6[] = "RedMist\\storyteller\\6.wav";
char story7[] = "RedMist\\storyteller\\7.wav";
char story_secret[] = "RedMist\\storyteller\\secret.wav";
char story_end[] = "RedMist\\storyteller\\end.wav";
char story8[] = "RedMist\\storyteller\\tutorial.wav";

void set_speech(char* speech, char* adr)
{
    patch_setdword((DWORD*)(speech + 4), (DWORD)adr);
    patch_setdword((DWORD*)(speech + 12), 0);
}

DWORD remember_music = 101;
DWORD remember_sound = 101;

PROC g_proc_00430113;
int objct_get_briefing_speech(char* speech)
{
    remember_music = *(DWORD*)VOLUME_MUSIC;
    remember_sound = *(DWORD*)VOLUME_SOUND;
    if (remember_music != 0)
        *(DWORD*)VOLUME_MUSIC = 20;
    *(DWORD*)VOLUME_SOUND = 100;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM);//set volume

    DWORD remember1 = *(DWORD*)(speech + 4);
    DWORD remember2 = *(DWORD*)(speech + 12);
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))set_speech(speech, story1);
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))set_speech(speech, story2);
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))set_speech(speech, story3);
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))set_speech(speech, story4);
    else if ((lvl == LVL_HUMAN5) || (lvl == LVL_XHUMAN5))set_speech(speech, story5);
    else if ((lvl == LVL_HUMAN6) || (lvl == LVL_XHUMAN6))set_speech(speech, story6);
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))set_speech(speech, story7);
    else if (lvl == LVL_HUMAN10)set_speech(speech, story_secret);
    else if (lvl == LVL_ORC1)set_speech(speech, story8);
    //-------------------------------------------------

    int original = ((int (*)(char*))g_proc_00430113)(speech);//original
    patch_setdword((DWORD*)(speech + 4), remember1);
    patch_setdword((DWORD*)(speech + 12), remember2);
    return original;
}

bool finale_dlg = false;

PROC g_proc_0041F0F5;
int finale_get_tbl(void* tbl, WORD str_id)
{
    finale_dlg = false;
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if (lvl == (LVL_XHUMAN12 + 2))return (int)tbl_brif8;
    else
    {
        new_tbl = tbl_end;
        str_id = 1;
    }
    //-------------------------------------------------    

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0041F0F5)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0041F0F5)(tbl, str_id);//original
}

PROC g_proc_0041F1E8;
int finale_credits_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if (lvl == (LVL_XHUMAN12 + 2))return (int)tbl_brif8;
    else
    {
        new_tbl = tbl_credits;
        str_id = 1;
    }
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0041F1E8)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0041F1E8)(tbl, str_id);//original
}

PROC g_proc_0041F027;
int finale_get_speech(char* speech)
{
    remember_music = *(DWORD*)VOLUME_MUSIC;
    remember_sound = *(DWORD*)VOLUME_SOUND;
    if (remember_music != 0)
        *(DWORD*)VOLUME_MUSIC = 22;
    *(DWORD*)VOLUME_SOUND = 100;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM);//set volume

    DWORD remember1 = *(DWORD*)(speech + 4);
    DWORD remember2 = *(DWORD*)(speech + 12);

    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if (lvl == (LVL_XHUMAN12 + 2))set_speech(speech, story8);
    else set_speech(speech, story_end);
    //-------------------------------------------------
    int original = ((int (*)(char*))g_proc_0041F027)(speech);//original
    patch_setdword((DWORD*)(speech + 4), remember1);
    patch_setdword((DWORD*)(speech + 12), remember2);
    return original;
}

int cred_num = 0;

PROC g_proc_00417E33;
int credits_small_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    new_tbl = tbl_credits;
    str_id = 1;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_00417E33)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_00417E33)(tbl, str_id);//original
}

PROC g_proc_00417E4A;
int credits_big_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    return (int)tbl_credits_title;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_00417E4A)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_00417E4A)(tbl, str_id);//original
}

char tbl_empty[] = " ";

PROC g_proc_0042968A;
int act_get_tbl_small(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    return (int)tbl_empty;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0042968A)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0042968A)(tbl, str_id);//original
}

PROC g_proc_004296A9;
int act_get_tbl_big(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    return (int)tbl_empty;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004296A9)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004296A9)(tbl, str_id);//original
}

PROC g_proc_0041C51C;
int netstat_get_tbl_nation(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    // 46 blue hum
    // 47 blue orc
    // 48 white hum
    // 49 white orc
    // 50 red hum
    // 51 red orc
    // 52 green hum
    // 53 green orc
    // 54 black hum
    // 55 black orc
    // 56 violet hum
    // 57 violet orc
    // 58 orange hum
    // 59 orange orc
    // 60 yellow hum
    // 61 yellow orc
    // 92 blue xorc
    // 93 white xorc
    // 94 red xorc
    // 95 green xorc
    // 96 black xorc
    // 97 violet xorc
    // 98 orange xorc
    // 99 yellow xorc
    //-------------------------------------------------
    if (str_id == 50)
    {
        new_tbl = tbl_nations;
        str_id = 1;
    }
    else if (str_id == 46)
    {
        new_tbl = tbl_nations;
        str_id = 2;
    }
    else if (str_id == 58)
    {
        new_tbl = tbl_nations;
        str_id = 3;
    }
    else if (str_id == 56)
    {
        new_tbl = tbl_nations;
        str_id = 4;
    }
    else if (str_id == 54)
    {
        new_tbl = tbl_nations;
        str_id = 1;
    }
    else if (((str_id >= 46) && (str_id <= 61)) || ((str_id >= 92) && (str_id <= 99)))
    {
        new_tbl = tbl_nations;
        str_id = 5;
    }
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0041C51C)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0041C51C)(tbl, str_id);//original
}

PROC g_proc_00431229;
int rank_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------

    //-------------------------------------------------
    //new_tbl = tbl_rank;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_00431229)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_00431229)(tbl, str_id);//original
}

char instal2[] = "\\RedMist\\fonts.mpq";
DWORD loaded_instal2 = 0;

char my_font6[] = "RedMistFonts\\font6.fnt";
char my_font10x[] = "RedMistFonts\\font10x.fnt";
char my_font12x[] = "RedMistFonts\\font12x.fnt";
char my_font32[] = "RedMistFonts\\font32.fnt";
char my_font50[] = "RedMistFonts\\font50.fnt";

void reload_install_exe2()
{
    if (loaded_instal2 == 0)
    {
        *(DWORD*)INSTALL_EXE_POINTER = 0;//remove existing install
        char buf[] = "\x0\x0\x0\x0";
        patch_setdword((DWORD*)buf, (DWORD)instal2);
        PATCH_SET((char*)INSTALL_EXE_NAME1, buf);
        PATCH_SET((char*)INSTALL_EXE_NAME2, buf);//change names
        PATCH_SET((char*)INSTALL_EXE_NAME3, buf);
        ((int (*)(int, int))F_RELOAD_INSTALL_EXE)(1, 0);//load install.exe
        loaded_instal2 = *(DWORD*)INSTALL_EXE_POINTER;
    }
    else
    {
        *(DWORD*)INSTALL_EXE_POINTER = loaded_instal2;
    }
}

PROC g_proc_00424A9C;//32
PROC g_proc_00424AB2;//50
PROC g_proc_004288B2;//12
PROC g_proc_00428896;//10
PROC g_proc_0042887D;//6
void* storm_font_load(char* name, char* a1, int a2)
{
    DWORD orig_instal = *(DWORD*)INSTALL_EXE_POINTER;//remember existing install
    reload_install_exe2();
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font6.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font6;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font10x.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font10x;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font12x.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font12x;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font32.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font32;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\font50.fnt", -1) == CSTR_EQUAL)
    {
        name = my_font50;
    }
    void* original = ((void* (*)(char*, char*, int))g_proc_00424A9C)(name, a1, a2);//original
    *(DWORD*)INSTALL_EXE_POINTER = orig_instal;//restore existing install
    return original;
}

void small_fonts_reload()
{
    DWORD orig_instal = *(DWORD*)INSTALL_EXE_POINTER;//remember existing install
    reload_install_exe2();
    *(DWORD*)FONT_POINTER_6 = ((DWORD(*)(char*, char*, int))F_FONT_RELOAD)(my_font6, (char*)FONT_RELOAD_PARAM, 0x3B9);//original storm font load
    *(DWORD*)FONT_POINTER_10 = ((DWORD(*)(char*, char*, int))F_FONT_RELOAD)(my_font10x, (char*)FONT_RELOAD_PARAM, 0x3BA);//original storm font load
    *(DWORD*)FONT_POINTER_12 = ((DWORD(*)(char*, char*, int))F_FONT_RELOAD)(my_font12x, (char*)FONT_RELOAD_PARAM, 0x3BB);//original storm font load
    *(DWORD*)INSTALL_EXE_POINTER = orig_instal;//restore existing install
}

void pal_load(byte* palette_adr, void* pal)
{
    if (palette_adr != NULL)
    {
        if (pal != NULL)
        {
            DWORD i = 0;
            while (i < (256 * 4))
            {
                *(byte*)(palette_adr + i) = *(byte*)((DWORD)pal + i);
                i++;
            }
        }
    }
}

PROC g_proc_004372EE;
void pcx_load_menu(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_004372EE)(name, pcx_info, palette_adr);//original
    void* new_pcx_pixels = NULL; 
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\titlemenu_bne.pcx", -1) == CSTR_EQUAL)
    {
        cred_num = 0;
        new_pcx_pixels = pcx_menu;
        pal_load(palette_adr, pcx_menu_pal);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\title_rel_bne.pcx", -1) == CSTR_EQUAL)
    {
        small_fonts_reload();
        new_pcx_pixels = pcx_splash;
        pal_load(palette_adr, pcx_splash_pal);
    }
    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00430058;
void pcx_load_briefing(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_00430058)(name, pcx_info, palette_adr);//original
    byte lvl = *(byte*)LEVEL_OBJ;
    void* new_pcx_pixels = NULL;
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1) | (lvl == LVL_HUMAN5) | (lvl == LVL_XHUMAN5))
    {
        new_pcx_pixels = pcx_b15;
        pal_load(palette_adr, pcx_b15_pal);
    }
    if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
    {
        new_pcx_pixels = pcx_b2;
        pal_load(palette_adr, pcx_b2_pal);
    }
    if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
    {
        new_pcx_pixels = pcx_b3;
        pal_load(palette_adr, pcx_b3_pal);
    }
    if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
    {
        new_pcx_pixels = pcx_b4;
        pal_load(palette_adr, pcx_b4_pal);
    }
    if ((lvl == LVL_HUMAN6) || (lvl == LVL_XHUMAN6) | (lvl == LVL_HUMAN14) | (lvl == LVL_XHUMAN12))
    {
        new_pcx_pixels = pcx_b67;
        pal_load(palette_adr, pcx_b67_pal);
    }

    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00429625;//load palette
PROC g_proc_00429654;//load image
void pcx_load_act(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_00429625)(name, pcx_info, palette_adr);//original
    byte lvl = *(byte*)LEVEL_OBJ;
    void* new_pcx_pixels = NULL;
    
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))
    {
        new_pcx_pixels = pcx_act1;
        pal_load(palette_adr, pcx_act1_pal);
    }
    else if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))
    {
        new_pcx_pixels = pcx_act2;
        pal_load(palette_adr, pcx_act2_pal);
    }
    else if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))
    {
        new_pcx_pixels = pcx_act3;
        pal_load(palette_adr, pcx_act3_pal);
    }
    else if ((lvl == LVL_HUMAN4) || (lvl == LVL_XHUMAN4))
    {
        new_pcx_pixels = pcx_act4;
        pal_load(palette_adr, pcx_act4_pal);
    }
    else if ((lvl == LVL_HUMAN5) || (lvl == LVL_XHUMAN5))
    {
        new_pcx_pixels = pcx_act5;
        pal_load(palette_adr, pcx_act5_pal);
    }
    else if ((lvl == LVL_HUMAN6) || (lvl == LVL_XHUMAN6))
    {
        new_pcx_pixels = pcx_act6;
        pal_load(palette_adr, pcx_act6_pal);
    }
    else if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))
    {
        new_pcx_pixels = pcx_act7;
        pal_load(palette_adr, pcx_act7_pal);
    }
    else if (lvl == LVL_HUMAN10)
    {
        new_pcx_pixels = pcx_act8;
        pal_load(palette_adr, pcx_act8_pal);
    }
    else
    {
        new_pcx_pixels = pcx_act1;
        pal_load(palette_adr, pcx_act1_pal);
    }

    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_0041F004;
void pcx_load_final(char* name, void* pcx_info, byte* palette_adr)
{
    finale_dlg = true;
    ((void (*)(char*, void*, byte*))g_proc_0041F004)(name, pcx_info, palette_adr);//original
    void* new_pcx_pixels = NULL;
    if (*(byte*)LEVEL_OBJ == (LVL_XHUMAN12 + 2))
    {
        new_pcx_pixels = pcx_act1;
        pal_load(palette_adr, pcx_act1_pal);
    }
    else
    {
        new_pcx_pixels = pcx_end;
        pal_load(palette_adr, pcx_end_pal);
    }
    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00417DDB;
void pcx_load_credits(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_00417DDB)(name, pcx_info, palette_adr);//original
    void* new_pcx_pixels = NULL;
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\credits.pcx", -1) == CSTR_EQUAL)
    {
        if (cred_num == 0)
        {
            cred_num = 1; 
            new_pcx_pixels = pcx_credits;
            pal_load(palette_adr, pcx_credits_pal);
        }
        if (cred_num == 2)
        {
            cred_num = 3;
            new_pcx_pixels = pcx_credits;
            pal_load(palette_adr, pcx_credits_pal);
        }
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\2xcredit.pcx", -1) == CSTR_EQUAL)
    {
        cred_num = 2;
        new_pcx_pixels = pcx_credits;
        pal_load(palette_adr, pcx_credits_pal);
    }
    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_0043169E;
void pcx_load_statistic(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_0043169E)(name, pcx_info, palette_adr);//original
    void* new_pcx_pixels = NULL;
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\hwinglu.pcx", -1) == CSTR_EQUAL)
    {
        //new_pcx_pixels = pcx_win;
        //pal_load(palette_adr, pcx_win_pal);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\hlosglu.pcx", -1) == CSTR_EQUAL)
    {
        //new_pcx_pixels = pcx_loss;
        //pal_load(palette_adr, pcx_loss_pal);
    }
    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00462D4D;
void* storm_file_load(char* name, int a1, int a2, int a3, int a4, int a5, int a6)
{
    void* original = ((void* (*)(void*, int, int, int, int, int, int))g_proc_00462D4D)(name, a1, a2, a3, a4, a5, a6);//original
    void* new_file = NULL;
    //-------------------------------------------------
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\mainmenu.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_menu_copy, bin_menu, bin_menu_size);
        new_file = bin_menu_copy;
        patch_setdword((DWORD*)a2, bin_menu_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\snglplay.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_sngl_copy, bin_sngl, bin_sngl_size);
        new_file = bin_sngl_copy;
        patch_setdword((DWORD*)a2, bin_sngl_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\newcmpgn.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_newcmp_copy, bin_newcmp, bin_newcmp_size);
        new_file = bin_newcmp_copy;
        patch_setdword((DWORD*)a2, bin_newcmp_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\quit.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_quit_copy, bin_quit, bin_quit_size);
        new_file = bin_quit_copy;
        patch_setdword((DWORD*)a2, bin_quit_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\ai.bin", -1) == CSTR_EQUAL)
    {
        new_file = bin_AI;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\script.bin", -1) == CSTR_EQUAL)
    {
        new_file = bin_script;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\unitdata.dat", -1) == CSTR_EQUAL)
    {
        new_file = bin_unitdata;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\unitdato.dat", -1) == CSTR_EQUAL)
    {
        new_file = bin_unitdato;
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\upgrades.dat", -1) == CSTR_EQUAL)
    {
        new_file = bin_upgrades;
    }
    //-------------------------------------------------
    if (new_file)
        return new_file;
    else
        return original;
}

void set_rgb(DWORD* adr, byte r, byte g, byte b)
{
    patch_setdword(adr, r + (g << 8) + (b << 16));
}

void set_rgb4(DWORD* adr, byte r, byte g, byte b)
{
    patch_setdword(adr, (4 * r) + ((4 * g) << 8) + ((4 * b) << 16));
}

void set_rgb8(DWORD* adr, byte r, byte g, byte b)
{
    patch_setdword(adr, (8 * r) + ((8 * g) << 8) + ((8 * b) << 16));
}

void set_palette(void* pal)
{
    for (int i = 0; i < 256; i++)
    {
        byte r = *(byte*)((int)pal + 3 * i);
        byte g = *(byte*)((int)pal + 3 * i + 1);
        byte b = *(byte*)((int)pal + 3 * i + 2);
        set_rgb4((DWORD*)(SCREEN_INITIAL_PALETTE + i * 4), r, g, b);
    }
}

void set_yellow_palette()
{
    //change human blue buttons to yellowish
    int k = 0xB0;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 4, 4, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 6, 6, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 8, 8, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 9, 9, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 11, 11, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 14, 14, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 15, 15, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 16, 16, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 18, 18, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 19, 19, 0);
}

PROC g_proc_0041F9FD;
int tilesets_load(int a)
{
    int original = ((int (*)(int))g_proc_0041F9FD)(a);
    byte lvl = *(byte*)LEVEL_OBJ;
    if (lvl == LVL_HUMAN10)set_rgb((DWORD*)(SCREEN_INITIAL_PALETTE + RAINDROPS_COLOR * 4), 255, 255, 255);
    //set_yellow_palette();
    return original;
}

PROC g_proc_0041F97D;
int map_file_load(int a, int b, void** map, DWORD* size)
{
    byte lvl = *(byte*)LEVEL_OBJ;
    byte d = *(byte*)GB_DEMO;//demo
    bool f = false;
    if (d == 1)
    {
        //*map = pud_map_atr;
        //*size = pud_map_atr_size;
        //f = true;
    }
    else
    {
        if (lvl == LVL_HUMAN1)
        {
            *map = pud_map1;
            *size = pud_map1_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN2)
        {
            *map = pud_map2;
            *size = pud_map2_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN3)
        {
            *map = pud_map3;
            *size = pud_map3_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN4)
        {
            *map = pud_map4;
            *size = pud_map4_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN5)
        {
            *map = pud_map5;
            *size = pud_map5_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN6)
        {
            *map = pud_map6;
            *size = pud_map6_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN14)
        {
            *map = pud_map7;
            *size = pud_map7_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN1)
        {
            *map = pud_emap1;
            *size = pud_emap1_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN2)
        {
            *map = pud_emap2;
            *size = pud_emap2_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN3)
        {
            *map = pud_emap3;
            *size = pud_emap3_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN4)
        {
            *map = pud_emap4;
            *size = pud_emap4_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN5)
        {
            *map = pud_emap5;
            *size = pud_emap5_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN6)
        {
            *map = pud_emap6;
            *size = pud_emap6_size;
            f = true;
        }
        else if (lvl == LVL_XHUMAN12)
        {
            *map = pud_emap7;
            *size = pud_emap7_size;
            f = true;
        }
        else if (lvl == LVL_HUMAN10)
        {
            *map = pud_map_secret;
            *size = pud_map_secret_size;
            f = true;
        }
        else if (lvl == LVL_ORC1)
        {
            *map = pud_map_tutorial;
            *size = pud_map_tutorial_size;
            f = true;
        }
    }
    if (f)return 1;
    else return ((int (*)(int, int, void*, DWORD*))g_proc_0041F97D)(a, b, map, size);//original
}

void* file_load(const char name[])
{
    void* file = NULL;
    FILE* fp;
    char path[MAX_PATH] = { 0 };
    _snprintf(path, sizeof(path), "%s%s", FILES_PATH, name);
    if ((fp = fopen(path, "rb")) != NULL)//file opened
    {
        fseek(fp, 0, SEEK_END); // seek to end of file
        DWORD size = ftell(fp); // get current file pointer
        fseek(fp, 0, SEEK_SET); // seek back to beginning of file
        file = malloc(size);
        fread(file, sizeof(unsigned char), size, fp);//read
        fclose(fp);
    }
    return file;
}

void file_load_size(const char name[], void** m, DWORD* s)
{
    void* file = NULL;
    FILE* fp;
    char path[MAX_PATH] = { 0 };
    _snprintf(path, sizeof(path), "%s%s", FILES_PATH, name);
    if ((fp = fopen(path, "rb")) != NULL)//file opened
    {
        fseek(fp, 0, SEEK_END); // seek to end of file
        DWORD size = ftell(fp); // get current file pointer
        *s = size;
        fseek(fp, 0, SEEK_SET); // seek back to beginning of file
        file = malloc(size);
        fread(file, sizeof(unsigned char), size, fp);//read
        fclose(fp);
    }
    *m = file;
}

char video_mod_shab[] = "RedMist\\videos\\%s";
char video1[] = "start.smk";
char video2[] = "intro.smk";
char video3[] = "final.smk";

PROC g_proc_0043B16F;
void smk_play_sprintf_name(int dest, char* shab, char* name)
{
    if (!lstrcmpi(name, "hvicx_m.smk"))
    {
        shab = video_mod_shab;
        name = video3;
    }
    else if (!lstrcmpi(name, "intro_m.smk"))
    {
        shab = video_mod_shab;
        name = video2;
    }
    else if (!lstrcmpi(name, "introx_m.smk"))
    {
        shab = video_mod_shab;
        name = video2;
    }
    else if (!lstrcmpi(name, "hvict_m.smk"))
    {
        shab = video_mod_shab;
        name = video3;
    }
    ((void (*)(int, char*, char*))g_proc_0043B16F)(dest, shab, name);//original
}

PROC g_proc_0043B362;
void smk_play_sprintf_blizzard(int dest, char* shab, char* name)
{
    ((void (*)(int, char*, char*))g_proc_0043B362)(dest, video_mod_shab, video1);//original
}

char instal[] = "\\RedMist\\music.mpq";
DWORD loaded_instal = 0;
int need_instal = 0;
char music1[] = "menu.wav";
char music2[] = "end.wav";
char music3[] = "brif.wav";

void reload_install_exe()
{
    if (loaded_instal == 0)
    {
        *(DWORD*)INSTALL_EXE_POINTER = 0;//remove existing install
        char buf[] = "\x0\x0\x0\x0";
        patch_setdword((DWORD*)buf, (DWORD)instal);
        PATCH_SET((char*)INSTALL_EXE_NAME1, buf);
        PATCH_SET((char*)INSTALL_EXE_NAME2, buf);//change names
        PATCH_SET((char*)INSTALL_EXE_NAME3, buf);
        ((int (*)(int, int))F_RELOAD_INSTALL_EXE)(1, 0);//load install.exe
        loaded_instal = *(DWORD*)INSTALL_EXE_POINTER;
    }
    else
    {
        *(DWORD*)INSTALL_EXE_POINTER = loaded_instal;
    }
}

void music_play_sprintf_name(int dest, char* shab, char* name)
{
    if (!lstrcmpi(name, "owarroom.wav"))
    {
        need_instal = 2;
        name = music1;
    }
    if (!lstrcmpi(name, "hwarroom.wav"))
    {
        need_instal = 2;
        name = music3;
    }
    if (finale_dlg)
    {
        need_instal = 2;
        if (*(byte*)LEVEL_OBJ == (LVL_XHUMAN12 + 2))name = music3;
        else name = music2;
    }
    DWORD orig = *(DWORD*)F_MUSIC_SPRINTF;//original music sprintf call
    ((void (*)(int, char*, char*))orig)(dest, shab, name);//original
}

PROC g_proc_00440F4A;
PROC g_proc_00440F5F;
int music_play_get_install()
{
    DWORD orig_instal = *(DWORD*)INSTALL_EXE_POINTER;//remember existing install
    if (need_instal == 1)
    {
        reload_install_exe();
        need_instal--;
    }
    if (need_instal == 2)need_instal--;
    int original = ((int (*)())g_proc_00440F4A)();//original
    *(DWORD*)INSTALL_EXE_POINTER = orig_instal;//restore existing install
    return original;
}

void files_sounds_check()
{
    for (int i = 0; i < 18; i++)if (access(w_names[i], F_OK) != 0)w_sounds_e = false;
    for (int i = 0; i < 21; i++)if (access(hu_names[i], F_OK) != 0)hu_sounds_e = false;
    for (int i = 0; i < 12; i++)if (access(e_names[i], F_OK) != 0)e_sounds_e = false;
    for (int i = 0; i < 12; i++)if (access(k_names[i], F_OK) != 0)k_sounds_e = false;
    for (int i = 0; i < 12; i++)if (access(pk_names[i], F_OK) != 0)pk_sounds_e = false;
    for (int i = 0; i < 10; i++)if (access(wz_names[i], F_OK) != 0)wz_sounds_e = false;
    for (int i = 0; i < 11; i++)if (access(d_names[i], F_OK) != 0)d_sounds_e = false;
    for (int i = 0; i < 7; i++)if (access(g_names[i], F_OK) != 0)g_sounds_e = false;
    for (int i = 0; i < 10; i++)if (access(s_names[i], F_OK) != 0)s_sounds_e = false;
    if (access(hh1_name, F_OK) != 0)hh1_sounds_e = false;
    if (access(hh2_name, F_OK) != 0)hh2_sounds_e = false;
}

void files_init()
{
    files_sounds_check();

    grp_port_forest = file_load("icons\\port_forest.grp");
    grp_port_winter = file_load("icons\\port_winter.grp");
    grp_port_wast = file_load("icons\\port_wast.grp");
    grp_port_swamp = file_load("icons\\port_swamp.grp");
    grp_ruka = file_load("icons\\ruka.grp");
    grp_arrow = file_load("icons\\arrow.grp");
    grp_bliz_icon = file_load("icons\\bliz.grp");
    grp_ace_black_icon = file_load("icons\\ace_black.grp");
    grp_haste = file_load("icons\\haste.grp");
    grp_potion = file_load("icons\\potion.grp");
    grp_saw = file_load("icons\\saw.grp");
    grp_arta = file_load("icons\\arta.grp");
    grp_shield = file_load("icons\\shield.grp");
    grp_aviary_black_icon = file_load("icons\\aviary_black.grp");
    grp_fire = file_load("icons\\fire.grp");

    grp_aviary = file_load("buildings\\aviary.grp");
    grp_aviary_black = file_load("buildings\\aviary_black.grp");
    grp_aviary_green = file_load("buildings\\aviary_green.grp");

    grp_ace = file_load("units\\ace.grp");
    grp_ace_black = file_load("units\\ace_black.grp");
    grp_foot = file_load("units\\foot.grp");
    grp_foot_shield = file_load("units\\foot_shield.grp");
    grp_prince = file_load("units\\prince.grp");
    grp_ranger = file_load("units\\ranger.grp");
    grp_ranger2 = file_load("units\\ranger2.grp");
    grp_wizard = file_load("units\\wizard.grp");

    grp_bliz = file_load("bullets\\bliz.grp");
    grp_ballistabliz = file_load("bullets\\ballistabliz.grp");

    tbl_credits = file_load("textes\\credits.tbl");
    tbl_brif1 = file_load("textes\\brif1.tbl");
    tbl_brif2 = file_load("textes\\brif2.tbl");
    tbl_brif3 = file_load("textes\\brif3.tbl");
    tbl_brif4 = file_load("textes\\brif4.tbl");
    tbl_brif5 = file_load("textes\\brif5.tbl");
    tbl_brif6 = file_load("textes\\brif6.tbl");
    tbl_brif7 = file_load("textes\\brif7.tbl");
    tbl_end = file_load("textes\\end.tbl");

    tbl_task1 = file_load("textes\\task1.tbl");
    tbl_task2 = file_load("textes\\task2.tbl");
    tbl_task3 = file_load("textes\\task3.tbl");
    tbl_task4 = file_load("textes\\task4.tbl");
    tbl_task5 = file_load("textes\\task5.tbl");
    tbl_task6 = file_load("textes\\task6.tbl");
    tbl_task7 = file_load("textes\\task7.tbl");
    tbl_task8 = file_load("textes\\task8.tbl");

    tbl_title1 = file_load("textes\\title1.tbl");
    tbl_title2 = file_load("textes\\title2.tbl");
    tbl_title3 = file_load("textes\\title3.tbl");
    tbl_title4 = file_load("textes\\title4.tbl");
    tbl_title5 = file_load("textes\\title5.tbl");
    tbl_title6 = file_load("textes\\title6.tbl");
    tbl_title7 = file_load("textes\\title7.tbl");
    tbl_title8 = file_load("textes\\title8.tbl");
    tbl_title9 = file_load("textes\\title9.tbl");

    tbl_name1 = file_load("textes\\name1.tbl");
    tbl_name2 = file_load("textes\\name2.tbl");
    tbl_name3 = file_load("textes\\name3.tbl");
    tbl_name4 = file_load("textes\\name4.tbl");
    tbl_name5 = file_load("textes\\name5.tbl");
    tbl_name6 = file_load("textes\\name6.tbl");
    tbl_name7 = file_load("textes\\name7.tbl");
    tbl_name8 = file_load("textes\\name8.tbl");
    tbl_name9 = file_load("textes\\name9.tbl");
    tbl_name10 = file_load("textes\\name10.tbl");
    tbl_name11 = file_load("textes\\name11.tbl");

    tbl_skill1 = file_load("textes\\skill1.tbl");
    tbl_skill2 = file_load("textes\\skill2.tbl");
    tbl_skill3 = file_load("textes\\skill3.tbl");
    tbl_skill4 = file_load("textes\\skill4.tbl");
    tbl_skill5 = file_load("textes\\skill5.tbl");
    tbl_skill6 = file_load("textes\\skill6.tbl");
    tbl_skill7 = file_load("textes\\skill7.tbl");
    tbl_skill8 = file_load("textes\\skill8.tbl");
    tbl_skill9 = file_load("textes\\skill9.tbl");
    tbl_skill10 = file_load("textes\\skill10.tbl");
    tbl_skill11 = file_load("textes\\skill11.tbl");

    tbl_research1 = file_load("textes\\research1.tbl");
    tbl_research2 = file_load("textes\\research2.tbl");
    tbl_research3 = file_load("textes\\research3.tbl");
    tbl_research4 = file_load("textes\\research4.tbl");
    tbl_research5 = file_load("textes\\research5.tbl");
    tbl_research6 = file_load("textes\\research6.tbl");
    tbl_research7 = file_load("textes\\research7.tbl");
    tbl_research8 = file_load("textes\\research8.tbl");
    tbl_research9 = file_load("textes\\research9.tbl");
    tbl_research10 = file_load("textes\\research10.tbl");
    tbl_research11 = file_load("textes\\research11.tbl");

    tbl_build1 = file_load("textes\\build1.tbl");

    tbl_train1 = file_load("textes\\train1.tbl");
    tbl_train2 = file_load("textes\\train2.tbl");

    tbl_kills = file_load("textes\\kills.tbl");

    tbl_tutorial = file_load("textes\\tutorial.tbl");
    tbl_nations = file_load("textes\\nations.tbl");

    file_load_size("maps\\map1.pud", &pud_map1, &pud_map1_size);
    file_load_size("maps\\map2.pud", &pud_map2, &pud_map2_size);
    file_load_size("maps\\map3.pud", &pud_map3, &pud_map3_size);
    file_load_size("maps\\map4.pud", &pud_map4, &pud_map4_size);
    file_load_size("maps\\map5.pud", &pud_map5, &pud_map5_size);
    file_load_size("maps\\map6.pud", &pud_map6, &pud_map6_size);
    file_load_size("maps\\map7.pud", &pud_map7, &pud_map7_size);
    file_load_size("maps\\map_secret.pud", &pud_map_secret, &pud_map_secret_size);
    file_load_size("maps\\emap1.pud", &pud_emap1, &pud_emap1_size);
    file_load_size("maps\\emap2.pud", &pud_emap2, &pud_emap2_size);
    file_load_size("maps\\emap3.pud", &pud_emap3, &pud_emap3_size);
    file_load_size("maps\\emap4.pud", &pud_emap4, &pud_emap4_size);
    file_load_size("maps\\emap5.pud", &pud_emap5, &pud_emap5_size);
    file_load_size("maps\\emap6.pud", &pud_emap6, &pud_emap6_size);
    file_load_size("maps\\emap7.pud", &pud_emap7, &pud_emap7_size);
    file_load_size("maps\\tutorial.pud", &pud_map_tutorial, &pud_map_tutorial_size);

    file_load_size("bin\\menu.bin", &bin_menu, &bin_menu_size);
    file_load_size("bin\\menu.bin", &bin_menu_copy, &bin_menu_size);
    file_load_size("bin\\sngl.bin", &bin_sngl, &bin_sngl_size);
    file_load_size("bin\\sngl.bin", &bin_sngl_copy, &bin_sngl_size);
    file_load_size("bin\\newcmp.bin", &bin_newcmp, &bin_newcmp_size);
    file_load_size("bin\\newcmp.bin", &bin_newcmp_copy, &bin_newcmp_size);
    file_load_size("bin\\quit.bin", &bin_quit, &bin_quit_size);
    file_load_size("bin\\quit.bin", &bin_quit_copy, &bin_quit_size);

    file_load_size("bin\\ai.bin", &bin_AI, &bin_AI_size);
    file_load_size("bin\\script.bin", &bin_script, &bin_script_size);
    file_load_size("bin\\unitdata.dat", &bin_unitdata, &bin_unitdata_size);
    file_load_size("bin\\unitdato.dat", &bin_unitdato, &bin_unitdato_size);
    file_load_size("bin\\upgrades.dat", &bin_upgrades, &bin_upgrades_size);

    pcx_splash = file_load("images\\title.raw");
    pcx_splash_pal = file_load("images\\title.pal");
    pcx_menu = file_load("images\\menu.raw");
    pcx_menu_pal = file_load("images\\menu.pal");
    pcx_end = file_load("images\\end.raw");
    pcx_end_pal = file_load("images\\end.pal");
    pcx_credits = file_load("images\\credits.raw");
    pcx_credits_pal = file_load("images\\credits.pal");
    pcx_act1 = file_load("images\\act1.raw");
    pcx_act1_pal = file_load("images\\act1.pal");
    pcx_act2 = file_load("images\\act2.raw");
    pcx_act2_pal = file_load("images\\act2.pal");
    pcx_act3 = file_load("images\\act3.raw");
    pcx_act3_pal = file_load("images\\act3.pal");
    pcx_act4 = file_load("images\\act4.raw");
    pcx_act4_pal = file_load("images\\act4.pal");
    pcx_act5 = file_load("images\\act5.raw");
    pcx_act5_pal = file_load("images\\act5.pal");
    pcx_act6 = file_load("images\\act6.raw");
    pcx_act6_pal = file_load("images\\act6.pal");
    pcx_act7 = file_load("images\\act7.raw");
    pcx_act7_pal = file_load("images\\act7.pal");
    pcx_act8 = file_load("images\\act8.raw");
    pcx_act8_pal = file_load("images\\act8.pal");
    pcx_b2 = file_load("images\\b2.raw");
    pcx_b2_pal = file_load("images\\b2.pal");
    pcx_b3 = file_load("images\\b3.raw");
    pcx_b3_pal = file_load("images\\b3.pal");
    pcx_b4 = file_load("images\\b4.raw");
    pcx_b4_pal = file_load("images\\b4.pal");
    pcx_b15 = file_load("images\\b15.raw");
    pcx_b15_pal = file_load("images\\b15.pal");
    pcx_b67 = file_load("images\\b67.raw");
    pcx_b67_pal = file_load("images\\b67.pal");
}

PROC g_proc_0042A443;
void act_init()
{
    WORD m = *(WORD*)LEVEL_ID;
    if (*(byte*)LEVEL_OBJ == LVL_ORC1)*(WORD*)LEVEL_ID = 0x52D4;
    else *(WORD*)LEVEL_ID = 0x52C8;//mission file number
    *(WORD*)PREVIOUS_ACT = 0;//prev act
    ((void (*)())g_proc_0042A443)();//original
    *(WORD*)LEVEL_ID = m;//mission file number restore
}

PROC g_proc_00422D76;
void sound_play_unit_speech_replace(WORD sid, int a, int* u, int b, void* snd, char* name)
{
    def_name = (void*)*(int*)(SOUNDS_FILES_LIST + 8 + 24 * sid);
    def_sound = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);//save default
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)name);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)snd);
    ((void (*)(WORD, int, int*, int))g_proc_00422D76)(sid, a, u, b);//original
    snd = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)def_sound);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)def_name);//restore default
}

//PROC g_proc_00422D76;
void sound_play_unit_speech(WORD sid, int a, int* u, int b)
{
    bool f = true;
    if (u != NULL)
    {
        WORD sn = 0;
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if (id == U_DANATH)
        {
            if ((sid >= 262) && (sid <= 270))
            {
                sn = sid - 262;
                sound_play_unit_speech_replace(sid, a, u, b, h_sounds[sn], h_names[sn]);
                f = false;
            }
        }
        else if (id == U_KARGATH)
        {
            if ((*(byte*)LEVEL_OBJ == LVL_HUMAN1) || (*(byte*)LEVEL_OBJ == LVL_XHUMAN1))
            {
                if ((sid >= 325) && (sid <= 333))
                {
                    sn = sid - 325;
                    sound_play_unit_speech_replace(sid, a, u, b, p_sounds[sn], p_names[sn]);
                    f = false;
                }
            }
        }
        else if ((id == U_GRIFON) || (id == U_AVIARY))
        {
            if (((sid >= 243) && (sid <= 245)) || (sid == 62))
            {
                sn = (((int (*)())F_NET_RANDOM)() % 8);
                sound_play_unit_speech_replace(sid, a, u, b, drak_sounds[sn], drak_names[sn]);
                f = false;
            }
        }
        else if ((id == U_PEASANT) || (id == U_TOWN_HALL) || (id == U_KEEP) || (id == U_CASTLE))
        {
            if (w_sounds_e)
            {
                if ((sid >= 223) && (sid <= 238))//peasant
                {
                    sn = sid - 223;
                    sound_play_unit_speech_replace(sid, a, u, b, w_sounds[sn], w_names[sn]);
                    f = false;
                }
                else if (sid == 21)//research
                {
                    sound_play_unit_speech_replace(sid, a, u, b, wdn_sound, wdn_name);
                    f = false;
                }
            }
            if ((sid >= 262) && (sid <= 270))//danath
            {
                sn = sid - 262;
                sound_play_unit_speech_replace(sid, a, u, b, h_sounds[sn], h_names[sn]);
                f = false;
            }
        }
        else if ((id == U_HBARRACK) || (id == U_FOOTMAN) || (id == U_ARCHER) || (id == U_RANGER) || (id == U_KNIGHT) || (id == U_PALADIN))
        {
            if (sid == 21)//research
            {
                if (w_sounds_e)
                {
                    sound_play_unit_speech_replace(sid, a, u, b, wdn_sound, wdn_name);
                    f = false;
                }
            }
            else if ((sid >= 3) && (sid <= 23))//footman
            {
                if (hu_sounds_e)
                {
                    sn = sid - 3;
                    sound_play_unit_speech_replace(sid, a, u, b, hu_sounds[sn], hu_names[sn]);
                    f = false;
                }
            }
            else if ((sid >= 106) && (sid <= 117))//elf
            {
                if (e_sounds_e)
                {
                    sn = sid - 106;
                    sound_play_unit_speech_replace(sid, a, u, b, e_sounds[sn], e_names[sn]);
                    f = false;
                }
            }
            else if ((sid >= 141) && (sid <= 152))//knight
            {
                if (k_sounds_e)
                {
                    sn = sid - 141;
                    sound_play_unit_speech_replace(sid, a, u, b, k_sounds[sn], k_names[sn]);
                    f = false;
                }
            }
            else if ((sid >= 153) && (sid <= 164))//paladin
            {
                if (pk_sounds_e)
                {
                    sn = sid - 153;
                    sound_play_unit_speech_replace(sid, a, u, b, pk_sounds[sn], pk_names[sn]);
                    f = false;
                }
            }
        }
        else if ((id == U_INVENTOR) || (id == U_DWARWES) || (id == U_FLYER))
        {
            if ((sid >= 95) && (sid <= 105))//dwarf
            {
                if (d_sounds_e)
                {
                    sn = sid - 95;
                    sound_play_unit_speech_replace(sid, a, u, b, d_sounds[sn], d_names[sn]);
                    f = false;
                }
            }
            else if ((sid >= 118) && (sid <= 124))//gnome
            {
                if (g_sounds_e)
                {
                    sn = sid - 118;
                    sound_play_unit_speech_replace(sid, a, u, b, g_sounds[sn], g_names[sn]);
                    f = false;
                }
            }
        }
        else if ((id == U_MAGE_TOWER) || (id == U_MAGE))
        {
            if ((sid >= 213) && (sid <= 222))//mage
            {
                if (wz_sounds_e)
                {
                    sn = sid - 213;
                    sound_play_unit_speech_replace(sid, a, u, b, wz_sounds[sn], wz_names[sn]);
                    f = false;
                }
            }
            else if (sid == 21)//research
            {
                if (w_sounds_e)
                {
                    sound_play_unit_speech_replace(sid, a, u, b, wdn_sound, wdn_name);
                    f = false;
                }
            }
        }
        else if ((id == U_SHIPYARD) || (id == U_HTANKER) || (id == U_HDESTROYER) || (id == U_BATTLESHIP) || (id == U_HTRANSPORT) || (id == U_SUBMARINE))
        {
            if ((sid >= 189) && (sid <= 198))//ships
            {
                if (s_sounds_e)
                {
                    sn = sid - 189;
                    sound_play_unit_speech_replace(sid, a, u, b, s_sounds[sn], s_names[sn]);
                    f = false;
                }
            }
        }
    }
    if (f)
    {
        if (sid == 22)
        {
            if (w_sounds_e)
            {
                sound_play_unit_speech_replace(sid, a, u, b, wd_sound, wd_name);
                f = false;
            }
        }
        else if (sid == 21)
        {
            if (w_sounds_e)
            {
                sound_play_unit_speech_replace(sid, a, u, b, wdn_sound, wdn_name);
                f = false;
            }
        }
        else if (sid == 24)
        {
            if (hh1_sounds_e)
            {
                sound_play_unit_speech_replace(sid, a, u, b, hh1_sound, hh1_name);
                f = false;
            }
        }
        else if (sid == 25)
        {
            if (hh2_sounds_e)
            {
                sound_play_unit_speech_replace(sid, a, u, b, hh2_sound, hh2_name);
                f = false;
            }
        }
    }
    if (f)((void (*)(WORD, int, int*, int))g_proc_00422D76)(sid, a, u, b);//original
}

PROC g_proc_00422D5F;
void sound_play_unit_speech_soft_replace(WORD sid, int a, int* u, int b, void* snd, char* name)
{
    def_name = (void*)*(int*)(SOUNDS_FILES_LIST + 8 + 24 * sid);
    def_sound = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);//save default
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)name);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)snd);
    ((void (*)(WORD, int, int*, int))g_proc_00422D5F)(sid, a, u, b);//original
    snd = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)def_sound);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)def_name);//restore default
}

//PROC g_proc_00422D5F;
void sound_play_unit_speech_soft(WORD sid, int a, int* u, int b)
{
    bool f = true;
    if (u != NULL)
    {

    }
    if (f)((void (*)(WORD, int, int*, int))g_proc_00422D5F)(sid, a, u, b);//original
}
//-------------files

PROC g_proc_0044F37D;
void main_menu_init(int a)
{
    if (remember_music != 101)
        *(DWORD*)VOLUME_MUSIC = remember_music;
    if (remember_sound != 101)
        *(DWORD*)VOLUME_SOUND = remember_sound;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM);//set volume
    remember_music = 101;
    remember_sound = 101;

    *(byte*)PLAYER_RACE = 0;//human cursor

    game_started = false;

    ((void (*)(int))g_proc_0044F37D)(a);
}

PROC g_proc_00418937;
void dispatch_die_unitdraw_update_1_man(int* u)
{
    ((void (*)(int*))g_proc_00418937)(u);//original
}

PROC g_proc_00451590;
void unit_kill_peon_change(int* u)
{
    ((void (*)(int*))g_proc_00451590)(u);//original
}

void def_stat(byte u, WORD hp, byte str, byte prc, byte arm, byte rng, byte gold, byte lum, byte oil, byte time)
{
    //change some unit stats (changes for ALL units of this type)

    /*
    to change vision and multiselectable you can use this construction
    char buf[] = "\x0\x0\x0\x0";//fix vision
    patch_setdword((DWORD*)buf, (DWORD)F_VISION6);
    PATCH_SET((char*)(UNIT_VISION_FUNCTIONS_TABLE + 4 * U_DEMON), buf);
    char buf2[] = "\x1";
    PATCH_SET((char*)(UNIT_MULTISELECTABLE + U_DEMON), buf2);
    */

    char buf2[] = "\x0\x0";
    buf2[0] = hp % 256;
    buf2[1] = hp / 256;
    PATCH_SET((char*)(UNIT_HP_TABLE + 2 * u), buf2);
    char buf[] = "\x0";
    buf[0] = str;
    PATCH_SET((char*)(UNIT_STRENGTH_TABLE + u), buf);
    buf[0] = prc;
    PATCH_SET((char*)(UNIT_PIERCE_TABLE + u), buf);
    buf[0] = arm;
    PATCH_SET((char*)(UNIT_ARMOR_TABLE + u), buf);
    if (rng != 0)
    {
        buf[0] = rng;
        PATCH_SET((char*)(UNIT_RANGE_TABLE + u), buf);
    }
    buf[0] = gold;
    PATCH_SET((char*)(UNIT_GOLD_TABLE + u), buf);
    buf[0] = lum;
    PATCH_SET((char*)(UNIT_LUMBER_TABLE + u), buf);
    buf[0] = oil;
    PATCH_SET((char*)(UNIT_OIL_TABLE + u), buf);
    buf[0] = time;
    PATCH_SET((char*)(UNIT_TIME_TABLE + u), buf);
}

void def_upgr(byte u, WORD gold, WORD lum, WORD oil)
{
    char buf2[] = "\x0\x0";
    buf2[0] = gold % 256;
    buf2[1] = gold / 256;
    PATCH_SET((char*)(UPGR_GOLD_TABLE + 2 * u), buf2);
    buf2[0] = lum % 256;
    buf2[1] = lum / 256;
    PATCH_SET((char*)(UPGR_LUMBER_TABLE + 2 * u), buf2);
    buf2[0] = oil % 256;
    buf2[1] = oil / 256;
    PATCH_SET((char*)(UPGR_OIL_TABLE + 2 * u), buf2);
}

PROC g_proc_0041F915;
int map_load(void* map, DWORD size)
{
    int original = ((int (*)(void*, DWORD))g_proc_0041F915)(map, size);

    byte lvl = *(byte*)LEVEL_OBJ;

    return original;
}

PROC g_proc_0042BB04;
int* map_create_unit(int x, int y, byte id, byte o)
{
    int* u = NULL;
    u = ((int* (*)(int, int, byte, byte))g_proc_0042BB04)(x, y, id, o);
    if (u != NULL)
    {
        if (ai_fixed)
        {
            if ((id == U_PEASANT) || (id == U_PEON))
            {
                set_stat(u, 255, S_NEXT_FIRE);
                set_stat(u, 2, S_NEXT_FIRE + 1);
            }
        }
    }
    return u;
}

PROC g_proc_00424745;//entering
PROC g_proc_004529C0;//grow struct
int goods_into_inventory(int* p)
{
    int tr = (*(int*)((uintptr_t)p + S_ORDER_UNIT_POINTER));
    if (tr != 0)
    {
        bool f = false;
        int* trg = (int*)tr;
        byte o = *((byte*)((uintptr_t)p + S_OWNER));
        byte id = *((byte*)((uintptr_t)p + S_ID));
        byte tid = *((byte*)((uintptr_t)trg + S_ID));
        byte pf = *((byte*)((uintptr_t)p + S_PEON_FLAGS));
        int pflag = *(int*)(UNIT_GLOBAL_FLAGS + id * 4);
        int tflag = *(int*)(UNIT_GLOBAL_FLAGS + tid * 4);
        int res = 100;
        if (pf & PEON_LOADED)
        {
            if (((pflag & IS_SHIP) != 0) && ((tflag & IS_OILRIG) == 0))
            {
                int r = get_val(REFINERY, o);
                if (r != 0)res = 125;
                else res = 100;
                change_res(o, 2, 1, res);
                add_total_res(o, 2, 1, res);
                f = true;
            }
            else
            {
                if (((tflag & IS_TOWNHALL) != 0) || ((tflag & IS_LUMBER) != 0))
                {
                    if (((tflag & IS_TOWNHALL) != 0))
                    {
                        pf |= PEON_IN_CASTLE;
                        set_stat(p, pf, S_PEON_FLAGS);
                    }
                    if (((pf & PEON_HARVEST_GOLD) != 0) && ((tflag & IS_TOWNHALL) != 0))
                    {
                        int r2 = get_val(TH2, o);
                        int r3 = get_val(TH3, o);
                        if (r3 != 0)res = 120;
                        else
                        {
                            if (r2 != 0)res = 110;
                            else res = 100;
                        }
                        pf &= ~PEON_HARVEST_GOLD;
                        change_res(o, 0, 1, res);
                        add_total_res(o, 0, 1, res);
                        f = true;
                    }
                    else
                    {
                        if (((pf & PEON_HARVEST_LUMBER) != 0))
                        {
                            int r = get_val(LUMBERMILL, o);
                            if (r != 0)res = 125;
                            else res = 100;
                            if (o == *(byte*)LOCAL_PLAYER)
                            {
                                if ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_DD)) != 0)
                                    res += 50;
                            }
                            pf &= ~PEON_HARVEST_LUMBER;
                            change_res(o, 1, 1, res);
                            add_total_res(o, 1, 1, res);
                            f = true;
                        }
                    }
                }
            }
        }
        if (f)
        {
            pf &= ~PEON_LOADED;
            set_stat(p, pf, S_PEON_FLAGS);
            ((void (*)(int*))F_GROUP_SET)(p);
            return 1;
        }
    }
    return 0;
    //return ((int(*)(int*))g_proc_00424745)(p);//original
}

PROC g_proc_0042479E;
void peon_into_goldmine(int* u)
{
    ((void (*)(int*))g_proc_0042479E)(u);//original
    /*
    byte lvl = *(byte*)LEVEL_OBJ;
    if (!((lvl >= LVL_XHUMAN1) && (lvl % 2 == 0)))
    {
        if (!((lvl == LVL_HUMAN3) || (lvl == LVL_HUMAN7)))
        {
            byte o = *((byte*)((uintptr_t)u + S_OWNER));
            if (*(byte*)(CONTROLER_TYPE + o) == C_COMP)
            {
                int* g = (int*)*((int*)((uintptr_t)u + S_ORDER_UNIT_POINTER));
                WORD r = *((WORD*)((uintptr_t)g + S_RESOURCES));
                if (r < 5)r++;
                set_stat(g, r, S_RESOURCES);
            }
        }
    }
    */
}

PROC g_proc_0042A466;
void briefing_check()
{
    *(byte*)(LEVEL_OBJ + 1) = 0;
    ((void (*)())g_proc_0042A466)();//original
}

PROC g_proc_00416930;
void player_race_mission_cheat()
{
    *(byte*)PLAYER_RACE = 0;//human = 0
    ((void (*)())g_proc_00416930)();//original
}

PROC g_proc_0042AC6D;
void player_race_mission_cheat2()
{
    ((void (*)())g_proc_0042AC6D)();//original
    *(byte*)PLAYER_RACE = 0;//human = 0
}

void sounds_ready_table_set(byte id, WORD snd)
{
    char buf[] = "\x0\x0";
    buf[0] = snd % 256;
    buf[1] = snd / 256;
    PATCH_SET((char*)(UNIT_SOUNDS_READY_TABLE + 2 * id), buf);
}

void sounds_tables()
{
    sounds_ready_table_set(U_DANATH, 263);
}

void show_message_from_tbl(int time, void* tbl, int str_id)
{
    char* msg = ((char* (*)(void*, int))F_GET_LINE_FROM_TBL)(tbl, str_id);
    show_message(time, msg);
}

char buttons_warrior[11 * BUTTON_SIZE + 1];
char buttons_hero[12 * BUTTON_SIZE + 1];
char buttons_prince[1 * BUTTON_SIZE + 1];
char buttons_barak[7 * BUTTON_SIZE + 1];
char buttons_smith[8 * BUTTON_SIZE + 1];
char buttons_stables[2 * BUTTON_SIZE + 1];
char buttons_aviary[3 * BUTTON_SIZE + 1];
char buttons_ace[11 * BUTTON_SIZE + 1];
char buttons_ballista[11 * BUTTON_SIZE + 1];
char buttons_archer[12 * BUTTON_SIZE + 1];
char buttons_knight[10 * BUTTON_SIZE + 1];
char buttons_paladin[13 * BUTTON_SIZE + 1];
char buttons_lumber[7 * BUTTON_SIZE + 1];
char buttons_mage[9 * BUTTON_SIZE + 1];
char buttons_th[4 * BUTTON_SIZE + 1];

void empty_mage_cast_teleport(int id)
{
    byte orig_mana = get_manacost(VISION);
    manacost(VISION, TP_MANACOST);
    ((void (*)(int))F_CAST_SPELL)(id);
    manacost(VISION, orig_mana);
}

void buttons_init_mage()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_MAGE + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 9; i++)
    {
        buttons_mage[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_spell_learned;
    void (*r1) (int) = empty_mage_cast_teleport;

    k = 4;
    //buttons_mage[BUTTON_SIZE * k + 0] = '\x4';//button id?
    //buttons_mage[BUTTON_SIZE * k + 1] = '\x0';//button id?
    //buttons_mage[BUTTON_SIZE * k + 2] = '\x65';//icon
    //buttons_mage[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_mage + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_mage + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_mage[BUTTON_SIZE * k + 12] = L_SLOW;//arg
    buttons_mage[BUTTON_SIZE * k + 13] = ORDER_SPELL_VISION;//unit id
    //buttons_mage[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    //buttons_mage[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    //buttons_mage[BUTTON_SIZE * k + 16] = '\x0';//flags?
    //buttons_mage[BUTTON_SIZE * k + 17] = '\x0';//flags?
    //buttons_mage[BUTTON_SIZE * k + 18] = '\x0';//flags?
    //buttons_mage[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_MAGE), (DWORD)9);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_MAGE + 4), (DWORD)buttons_mage);
}

void buttons_init_prince()
{
    int a = BUTTONS_CARDS + 8 * U_FOOTMAN + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 1; i++)
    {
        buttons_prince[i] = *(byte*)(a + i);
    }

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_KARGATH), (DWORD)1);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_KARGATH + 4), (DWORD)buttons_prince);
}

void buttons_init_barak()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_HBARRACK + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 6; i++)
    {
        buttons_barak[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_research_spells;
    void (*r1) (int) = empty_build_research_spell;

    k = 6;
    buttons_barak[BUTTON_SIZE * k + 0] = '\x8';//button id?
    buttons_barak[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_barak[BUTTON_SIZE * k + 2] = '\x9B';//icon
    buttons_barak[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_barak + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_barak + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_barak[BUTTON_SIZE * k + 12] = A_HASTE;//arg
    buttons_barak[BUTTON_SIZE * k + 13] = UG_SP_HASTE;//unit id
    buttons_barak[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_barak[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_barak[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_barak[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_barak[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_barak[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_HBARRACK), (DWORD)7);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_HBARRACK + 4), (DWORD)buttons_barak);
}

int empty_check_hero(byte id)
{
    byte lvl = *(byte*)LEVEL_OBJ;
    bool m = true;
    if ((lvl == LVL_HUMAN1) || (lvl == LVL_XHUMAN1))m = false;
    if ((lvl == LVL_HUMAN2) || (lvl == LVL_XHUMAN2))m = false;
    if ((lvl == LVL_HUMAN3) || (lvl == LVL_XHUMAN3))m = false;
    if ((lvl == LVL_HUMAN6) || (lvl == LVL_XHUMAN6))m = false;
    if ((lvl == LVL_HUMAN14) || (lvl == LVL_XHUMAN12))m = false;
    if (m)return 0;
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                if (*((byte*)((uintptr_t)p + S_OWNER)) == *(byte*)LOCAL_PLAYER)
                {
                    byte idd = *((byte*)((uintptr_t)p + S_ID));
                    bool f = idd == id;
                    if (f)
                    {
                        if (!check_unit_dead(p))
                        {
                            //return false if player already have that unit
                            return 0;
                        }
                    }
                    f = (idd == U_TOWN_HALL) || (idd == U_GREAT_HALL) ||
                        (idd == U_STRONGHOLD) || (idd == U_KEEP) ||
                        (idd == U_CASTLE) || (idd == U_FORTRESS);
                    if (f)
                    {
                        if (!check_unit_dead(p) && check_unit_complete(p))
                        {
                            if (*((byte*)((uintptr_t)p + S_BUILD_ORDER)) == 0)
                            {
                                if (*((byte*)((uintptr_t)p + S_BUILD_TYPE)) == id)
                                {
                                    if (*((WORD*)((uintptr_t)p + S_BUILD_PROGRES)) != 0)
                                    {
                                        //return false if player already building that unit
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
    return 1;
}

void buttons_init_th()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_TOWN_HALL + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 3; i++)
    {
        buttons_th[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_check_hero;
    void (*r1) (int) = empty_build;

    k = 3;
    buttons_th[BUTTON_SIZE * k + 0] = '\x2';//button id?
    buttons_th[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_th[BUTTON_SIZE * k + 2] = '\xBC';//icon
    buttons_th[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_th + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_th + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_th[BUTTON_SIZE * k + 12] = U_DANATH;//arg
    buttons_th[BUTTON_SIZE * k + 13] = U_DANATH;//unit id
    buttons_th[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_th[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_th[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_th[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_th[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_th[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_TOWN_HALL), (DWORD)4);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_TOWN_HALL + 4), (DWORD)buttons_th);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_KEEP), (DWORD)4);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_KEEP + 4), (DWORD)buttons_th);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_CASTLE), (DWORD)4);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_CASTLE + 4), (DWORD)buttons_th);
}

void buttons_init_smith()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_HSMITH + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 6; i++)
    {
        buttons_smith[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_research_spells;
    void (*r1) (int) = empty_build_research_spell;

    k = 6;
    buttons_smith[BUTTON_SIZE * k + 0] = '\x7';//button id?
    buttons_smith[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_smith[BUTTON_SIZE * k + 2] = '\xB6';//icon
    buttons_smith[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_smith + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_smith + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_smith[BUTTON_SIZE * k + 12] = L_SPELL_21;//arg
    buttons_smith[BUTTON_SIZE * k + 13] = UG_SP_52;//unit id
    buttons_smith[BUTTON_SIZE * k + 14] = '\x2';//string from tbl
    buttons_smith[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_smith[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_smith[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_smith[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_smith[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;
    buttons_smith[BUTTON_SIZE * k + 0] = '\x8';//button id?
    buttons_smith[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_smith[BUTTON_SIZE * k + 2] = '\x3';//icon
    buttons_smith[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_smith + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_smith + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_smith[BUTTON_SIZE * k + 12] = A_WIND;//arg
    buttons_smith[BUTTON_SIZE * k + 13] = UG_SP_WHIRLWIND;//unit id
    buttons_smith[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_smith[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_smith[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_smith[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_smith[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_smith[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_HSMITH), (DWORD)8);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_HSMITH + 4), (DWORD)buttons_smith);
}

void buttons_init_lumber()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_HLUMBER + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 6; i++)
    {
        buttons_lumber[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_research_spells;
    void (*r1) (int) = empty_build_research_spell;

    k = 6;
    buttons_lumber[BUTTON_SIZE * k + 0] = '\x8';//button id?
    buttons_lumber[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_lumber[BUTTON_SIZE * k + 2] = '\x2';//icon
    buttons_lumber[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_lumber + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_lumber + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_lumber[BUTTON_SIZE * k + 12] = A_DD;//arg
    buttons_lumber[BUTTON_SIZE * k + 13] = UG_SP_ROT;//unit id
    buttons_lumber[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_lumber[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_lumber[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_lumber[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_lumber[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_lumber[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_HLUMBER), (DWORD)7);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_HLUMBER + 4), (DWORD)buttons_lumber);
}

int empty_check_horse(byte s)
{
    return ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_BLOOD)) != 0) ? empty_research_spells(s) : 0;
}

void buttons_init_stables()
{
    int k = 0;
    int (*rr) (byte) = empty_research_spells;
    int (*rr2) (byte) = empty_check_horse;
    void (*r1) (int) = empty_build_research_spell;

    buttons_stables[BUTTON_SIZE * k + 0] = '\x0';//button id?
    buttons_stables[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_stables[BUTTON_SIZE * k + 2] = '\x82';//icon
    buttons_stables[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_stables + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_stables + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_stables[BUTTON_SIZE * k + 12] = A_BLOOD;//arg
    buttons_stables[BUTTON_SIZE * k + 13] = UG_SP_BLOODLUST;//unit id
    buttons_stables[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_stables[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_stables[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_stables[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_stables[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_stables[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;
    buttons_stables[BUTTON_SIZE * k + 0] = '\x0';//button id?
    buttons_stables[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_stables[BUTTON_SIZE * k + 2] = '\x83';//icon
    buttons_stables[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_stables + (BUTTON_SIZE * k + 4)), (DWORD)rr2);
    patch_setdword((DWORD*)(buttons_stables + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_stables[BUTTON_SIZE * k + 12] = A_RUNES;//arg
    buttons_stables[BUTTON_SIZE * k + 13] = UG_SP_RUNES;//unit id
    buttons_stables[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_stables[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_stables[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_stables[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_stables[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_stables[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_STABLES), (DWORD)2);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_STABLES + 4), (DWORD)buttons_stables);
}

void buttons_init_aviary()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_AVIARY + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 1; i++)
    {
        buttons_aviary[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_research_spells;
    void (*r1) (int) = empty_build_research_spell;

    k = 1;
    buttons_aviary[BUTTON_SIZE * k + 0] = '\x1';//button id?
    buttons_aviary[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_aviary[BUTTON_SIZE * k + 2] = '\x2';//icon
    buttons_aviary[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_aviary + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_aviary + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_aviary[BUTTON_SIZE * k + 12] = A_RAISE;//arg
    buttons_aviary[BUTTON_SIZE * k + 13] = UG_SP_RAISEDEAD;//unit id
    buttons_aviary[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_aviary[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_aviary[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_aviary[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_aviary[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_aviary[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;
    buttons_aviary[BUTTON_SIZE * k + 0] = '\x2';//button id?
    buttons_aviary[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_aviary[BUTTON_SIZE * k + 2] = '\x1';//icon
    buttons_aviary[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_aviary + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_aviary + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_aviary[BUTTON_SIZE * k + 12] = A_UNHOLY;//arg
    buttons_aviary[BUTTON_SIZE * k + 13] = UG_SP_ARMOR;//unit id
    buttons_aviary[BUTTON_SIZE * k + 14] = '\x2';//string from tbl
    buttons_aviary[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_aviary[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_aviary[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_aviary[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_aviary[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_AVIARY), (DWORD)3);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_AVIARY + 4), (DWORD)buttons_aviary);
}

void empty_ace_cast_spell(int id)
{
    byte orig_mana = get_manacost(RAISE_DEAD);
    manacost(RAISE_DEAD, BREATH_MANACOST);
    ((void (*)(int))F_CAST_SPELL)(id);
    manacost(RAISE_DEAD, orig_mana);
}

void buttons_init_ace()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_GRIFON + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 10; i++)
    {
        buttons_ace[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_spell_learned;
    void (*r1) (int) = empty_ace_cast_spell;

    k = 10;
    buttons_ace[BUTTON_SIZE * k + 0] = '\x8';//button id?
    buttons_ace[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_ace[BUTTON_SIZE * k + 2] = '\x1';//icon
    buttons_ace[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_ace + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_ace + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_ace[BUTTON_SIZE * k + 12] = L_RAISE;//arg
    buttons_ace[BUTTON_SIZE * k + 13] = ORDER_SPELL_RAISEDEAD;//unit id
    buttons_ace[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_ace[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_ace[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_ace[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_ace[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_ace[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_GRIFON), (DWORD)11);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_GRIFON + 4), (DWORD)buttons_ace);
}

void empty_ballista_cast_spell(int id)
{
    byte orig_mana = get_manacost(BLIZZARD);
    manacost(BLIZZARD, 120);
    ((void (*)(int))F_CAST_SPELL)(id);
    manacost(BLIZZARD, orig_mana);
}

void buttons_init_ballista()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_BALLISTA + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 10; i++)
    {
        buttons_ballista[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_spell_learned;
    void (*r1) (int) = empty_ballista_cast_spell;

    k = 10;
    buttons_ballista[BUTTON_SIZE * k + 0] = '\x8';//button id?
    buttons_ballista[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_ballista[BUTTON_SIZE * k + 2] = '\x3';//icon
    buttons_ballista[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_ballista + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_ballista + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_ballista[BUTTON_SIZE * k + 12] = L_WIND;//arg
    buttons_ballista[BUTTON_SIZE * k + 13] = ORDER_SPELL_BLIZZARD;//unit id
    buttons_ballista[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_ballista[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_ballista[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_ballista[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_ballista[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_ballista[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_BALLISTA), (DWORD)11);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_BALLISTA + 4), (DWORD)buttons_ballista);
}

int empty_check_horse_hp(byte)
{
    if (((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_BLOOD)) != 0) && ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_RUNES)) != 0))
    {
        int* uf = (int*)*(int*)LOCAL_UNITS_SELECTED;
        if (uf)
        {
            byte pf = *((byte*)((uintptr_t)uf + S_PEON_FLAGS));
            if ((pf & PEON_HARVEST_GOLD) == 0)return 1;
        }
    }
    return 0;
}

void empty_nothing(int id) {}

void buttons_init_knight()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_KNIGHT + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 9; i++)
    {
        buttons_knight[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_check_horse_hp;
    void (*r1) (int) = empty_nothing;

    k = 9;
    buttons_knight[BUTTON_SIZE * k + 0] = '\x5';//button id?
    buttons_knight[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_knight[BUTTON_SIZE * k + 2] = '\x83';//icon
    buttons_knight[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_knight + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_knight + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_knight[BUTTON_SIZE * k + 12] = '\x0';//arg
    buttons_knight[BUTTON_SIZE * k + 13] = '\x0';//unit id
    buttons_knight[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_knight[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_knight[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_knight[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_knight[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_knight[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_KNIGHT), (DWORD)10);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_KNIGHT + 4), (DWORD)buttons_knight);
}

void buttons_init_paladin()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_UTER + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 12; i++)
    {
        if (i < BUTTON_SIZE * 9)buttons_paladin[i] = *(byte*)(a + i);
        else buttons_paladin[i + BUTTON_SIZE] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_check_horse_hp;
    void (*r1) (int) = empty_nothing;

    k = 9;
    buttons_paladin[BUTTON_SIZE * k + 0] = '\x5';//button id?
    buttons_paladin[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_paladin[BUTTON_SIZE * k + 2] = '\x83';//icon
    buttons_paladin[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_paladin + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_paladin + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_paladin[BUTTON_SIZE * k + 12] = '\x0';//arg
    buttons_paladin[BUTTON_SIZE * k + 13] = '\x0';//unit id
    buttons_paladin[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_paladin[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_paladin[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_paladin[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_paladin[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_paladin[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_PALADIN), (DWORD)13);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_PALADIN + 4), (DWORD)buttons_paladin);
}

int empty_check_shield(byte)
{
    return ((*(DWORD*)(SPELLS_LEARNED + 4 * *(byte*)LOCAL_PLAYER) & (1 << L_HASTE)) != 0) ? 1 : 0;
}

void empty_use_shield(int id)
{
    bool f = false;
    int* uf = NULL;
    for (int i = 0; (i < 9) && (uf == NULL); i++)
    {
        uf = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (uf)
        {
            byte id = *((byte*)((uintptr_t)uf + S_ID));
            if (id != U_FOOTMAN)uf = NULL;
        }
    }
    if (uf)
    {
        f = *((byte*)((uintptr_t)uf + S_PEON_FLAGS)) & PEON_HARVEST_GOLD;
    }
    for (int i = 0; i < 9; i++)
    {
        uf = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (uf)
        {
            byte id = *((byte*)((uintptr_t)uf + S_ID));
            if (id == U_FOOTMAN)
            {
                byte pf = *((byte*)((uintptr_t)uf + S_PEON_FLAGS));
                if (!f)pf |= PEON_HARVEST_GOLD;
                else pf &= ~PEON_HARVEST_GOLD;
                set_stat(uf, pf, S_PEON_FLAGS);
            }
        }
    }
}

void buttons_init_warrior()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_FOOTMAN + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 10; i++)
    {
        buttons_warrior[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_check_shield;
    void (*r1) (int) = empty_use_shield;

    k = 10;
    buttons_warrior[BUTTON_SIZE * k + 0] = '\x8';//button id?
    buttons_warrior[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_warrior[BUTTON_SIZE * k + 2] = '\x9B';//icon
    buttons_warrior[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_warrior + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_warrior + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_warrior[BUTTON_SIZE * k + 12] = '\x0';//arg
    buttons_warrior[BUTTON_SIZE * k + 13] = '\x0';//unit id
    buttons_warrior[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_warrior[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_warrior[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_warrior[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_warrior[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_warrior[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_FOOTMAN), (DWORD)11);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_FOOTMAN + 4), (DWORD)buttons_warrior);
}

void empty_cast_shield(int id)
{
    int* uf = (int*)*(int*)LOCAL_UNITS_SELECTED;
    if (uf)
    {
        byte id = *((byte*)((uintptr_t)uf + S_ID));
        if (id == U_DANATH)
        {
            byte mp = *((byte*)((uintptr_t)uf + S_MANA));
            if (mp >= 255)
            {
                mp -= 255;
                byte x = *((byte*)((uintptr_t)uf + S_X));
                byte y = *((byte*)((uintptr_t)uf + S_Y));
                set_region(x - 5, y - 5, x + 5, y + 5);
                find_all_alive_units(ANY_MEN);
                sort_stat(S_ID, U_BALLISTA, CMP_NEQ);
                sort_in_region();
                for (int i = 0; i < 16; i++)
                {
                    if (!check_ally(i, *(byte*)LOCAL_PLAYER))sort_stat(S_OWNER, i, CMP_NEQ);
                }
                sort_self(uf);
                set_stat_all(S_SHIELD, 666);
                set_stat(uf, mp, S_MANA);
                ((void (*)(WORD, WORD, byte))F_SPELL_SOUND_XY)(x * 32 + 16, y * 32 + 16, SS_ARMOR);
            }
        }
    }
}

void buttons_init_hero()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_DANATH + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 10; i++)
    {
        buttons_hero[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_true;
    void (*r1) (int) = empty_cast_shield;
    void (*r2) (int) = empty_nothing;

    k = 10;
    buttons_hero[BUTTON_SIZE * k + 0] = '\x7';//button id?
    buttons_hero[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_hero[BUTTON_SIZE * k + 2] = '\x4';//icon
    buttons_hero[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_hero + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_hero + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_hero[BUTTON_SIZE * k + 12] = '\x0';//arg
    buttons_hero[BUTTON_SIZE * k + 13] = '\x0';//unit id
    buttons_hero[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_hero[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_hero[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_hero[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_hero[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_hero[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;
    buttons_hero[BUTTON_SIZE * k + 0] = '\x8';//button id?
    buttons_hero[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_hero[BUTTON_SIZE * k + 2] = '\x51';//icon
    buttons_hero[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_hero + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_hero + (BUTTON_SIZE * k + 8)), (DWORD)r2);
    buttons_hero[BUTTON_SIZE * k + 12] = '\x0';//arg
    buttons_hero[BUTTON_SIZE * k + 13] = '\x0';//unit id
    buttons_hero[BUTTON_SIZE * k + 14] = '\x2';//string from tbl
    buttons_hero[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_hero[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_hero[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_hero[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_hero[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_DANATH), (DWORD)12);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_DANATH + 4), (DWORD)buttons_hero);
}

int empty_check_archer(byte)
{
    int k = 0;
    for (int i = 0; i < 9; i++)
    {
        int* uf = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (uf)
        {
            byte id = *((byte*)((uintptr_t)uf + S_ID));
            //if ((id == U_ARCHER) || (id == U_RANGER))k++;
            if (id == U_RANGER)k++;
        }
    }
    if (k == 1)
    {
        int* uf = (int*)*(int*)LOCAL_UNITS_SELECTED;
        if (uf)
        {
            byte id = *((byte*)((uintptr_t)uf + S_ID));
            //if ((id == U_ARCHER) || (id == U_RANGER))
            if (id == U_RANGER)
            {
                byte pf = *((byte*)((uintptr_t)uf + S_PEON_FLAGS));
                if ((pf & PEON_HARVEST_GOLD) == 0)return 1;
            }
        }
    }
    return 0;
}

int empty_check_archer2(byte)
{
    int k = 0;
    for (int i = 0; i < 9; i++)
    {
        int* uf = (int*)*(int*)(LOCAL_UNITS_SELECTED + 4 * i);
        if (uf)
        {
            byte id = *((byte*)((uintptr_t)uf + S_ID));
            //if ((id == U_ARCHER) || (id == U_RANGER))k++;
            if (id == U_RANGER)k++;
        }
    }
    if (k == 1)
    {
        int* uf = (int*)*(int*)LOCAL_UNITS_SELECTED;
        if (uf)
        {
            byte id = *((byte*)((uintptr_t)uf + S_ID));
            //if ((id == U_ARCHER) || (id == U_RANGER))
            if (id == U_RANGER)
            {
                byte pf = *((byte*)((uintptr_t)uf + S_PEON_FLAGS));
                if ((pf & PEON_HARVEST_GOLD) == 0)return 0;
                else return 1;
            }
        }
    }
    return 0;
}

void empty_archer_potion(int id)
{
    int* uf = (int*)*(int*)LOCAL_UNITS_SELECTED;
    if (uf)
    {
        byte l = *(byte*)LOCAL_PLAYER;
        if (cmp_res(l, 0, 0xB0, 0x4, 0, 0, CMP_BIGGER_EQ))//1200 gold
        {
            change_res(l, 3, 1, 1200);
            byte pf = *((byte*)((uintptr_t)uf + S_PEON_FLAGS));
            pf &= ~PEON_HARVEST_GOLD;
            set_stat(uf, pf, S_PEON_FLAGS);
            ((void (*)())F_STATUS_REDRAW)();//status redraw
        }
    }
}

int empty_check_archer_hp(byte)
{
    if (*(DWORD*)(GB_SCOUTING + *(byte*)LOCAL_PLAYER) != 0)
    {
        int* uf = (int*)*(int*)LOCAL_UNITS_SELECTED;
        if (uf)
        {
            byte pf = *((byte*)((uintptr_t)uf + S_PEON_FLAGS));
            if ((pf & PEON_HARVEST_LUMBER) == 0)return 1;
        }
    }
    return 0;
}

void buttons_init_archer()
{
    int k = 0;
    int a = BUTTONS_CARDS + 8 * U_ARCHER + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 9; i++)
    {
        buttons_archer[i] = *(byte*)(a + i);
    }

    int (*rr) (byte) = empty_check_archer;
    int (*rr2) (byte) = empty_check_archer2;
    void (*r1) (int) = empty_cast_spell;
    void (*r2) (int) = empty_archer_potion;
    int (*rr3) (byte) = empty_check_archer_hp;
    void (*r3) (int) = empty_nothing;

    k = 9;
    buttons_archer[BUTTON_SIZE * k + 0] = '\x6';//button id?
    buttons_archer[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_archer[BUTTON_SIZE * k + 2] = '\x85';//icon
    buttons_archer[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_archer + (BUTTON_SIZE * k + 4)), (DWORD)rr3);
    patch_setdword((DWORD*)(buttons_archer + (BUTTON_SIZE * k + 8)), (DWORD)r3);
    buttons_archer[BUTTON_SIZE * k + 12] = '\x0';//arg
    buttons_archer[BUTTON_SIZE * k + 13] = '\x0';//unit id
    buttons_archer[BUTTON_SIZE * k + 14] = '\x3';//string from tbl
    buttons_archer[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_archer[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_archer[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_archer[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_archer[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;
    buttons_archer[BUTTON_SIZE * k + 0] = '\x7';//button id?
    buttons_archer[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_archer[BUTTON_SIZE * k + 2] = '\x2';//icon
    buttons_archer[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_archer + (BUTTON_SIZE * k + 4)), (DWORD)rr2);
    patch_setdword((DWORD*)(buttons_archer + (BUTTON_SIZE * k + 8)), (DWORD)r2);
    buttons_archer[BUTTON_SIZE * k + 12] = '\x0';//arg
    buttons_archer[BUTTON_SIZE * k + 13] = '\x0';//unit id
    buttons_archer[BUTTON_SIZE * k + 14] = '\x2';//string from tbl
    buttons_archer[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_archer[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_archer[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_archer[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_archer[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;
    buttons_archer[BUTTON_SIZE * k + 0] = '\x8';//button id?
    buttons_archer[BUTTON_SIZE * k + 1] = '\x0';//button id?
    buttons_archer[BUTTON_SIZE * k + 2] = '\x1';//icon
    buttons_archer[BUTTON_SIZE * k + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_archer + (BUTTON_SIZE * k + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_archer + (BUTTON_SIZE * k + 8)), (DWORD)r1);
    buttons_archer[BUTTON_SIZE * k + 12] = '\x0';//arg
    buttons_archer[BUTTON_SIZE * k + 13] = ORDER_SPELL_BLOODLUST;//unit id
    buttons_archer[BUTTON_SIZE * k + 14] = '\x1';//string from tbl
    buttons_archer[BUTTON_SIZE * k + 15] = '\x0';//string from tbl
    buttons_archer[BUTTON_SIZE * k + 16] = '\x0';//flags?
    buttons_archer[BUTTON_SIZE * k + 17] = '\x0';//flags?
    buttons_archer[BUTTON_SIZE * k + 18] = '\x0';//flags?
    buttons_archer[BUTTON_SIZE * k + 19] = '\x0';//flags?
    k++;

    //patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_ARCHER), (DWORD)12);
    //patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_ARCHER + 4), (DWORD)buttons_archer);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_RANGER), (DWORD)12);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_RANGER + 4), (DWORD)buttons_archer);
}

char ballistabliz_name[] = "RedMist\\bullets\\ballistabliz.wav\x0";
void* ballistabliz_sound = NULL;
char bliz_name[] = "RedMist\\bullets\\bliz.wav\x0";
void* bliz_sound = NULL;
char arrow_name[] = "RedMist\\bullets\\parrow.wav\x0";
void* arrow_sound = NULL;
char breath_name[] = "RedMist\\speech\\ace\\breath.wav\x0";
void* breath_sound = NULL;

void magic_bliz(int* u)
{
    //00410780 (unit amount)
    byte caster = *((byte*)((uintptr_t)u + S_ID));
    if (caster == U_MAGE)
    {
        byte mp = *((byte*)((uintptr_t)u + S_MANA));
        if (mp >= get_manacost(BLIZZARD))
        {
            mp -= get_manacost(BLIZZARD);
            set_stat(u, mp, S_MANA);
            for (int i = 0; i < 9; i++)((void (*)(int*, byte))0x00410780)(u, 10);
            sound_play_from_file(0, (DWORD)bliz_name, bliz_sound);
        }
        else set_stat(u, ORDER_STOP, S_NEXT_ORDER);
    }
    else if (caster == U_BALLISTA)
    {
        byte mp = *((byte*)((uintptr_t)u + S_MANA));
        if (mp >= 120)
        {
            mp -= 120;
            set_stat(u, mp, S_MANA);
            for (int i = 0; i < 8; i++)((void (*)(int*, byte))0x00410780)(u, 12);
            sound_play_from_file(0, (DWORD)ballistabliz_name, ballistabliz_sound);
            if (mp < 120)set_stat(u, ORDER_STOP, S_NEXT_ORDER);
        }
        else set_stat(u, ORDER_STOP, S_NEXT_ORDER);
    }
    else((void (*)(int*))0x00442BE0)(u);//orig bliz
}

void magic_blood(int* u)
{
    byte caster = *((byte*)((uintptr_t)u + S_ID));
    if ((caster == U_ARCHER) || (caster == U_RANGER))
    {
        byte pf = *((byte*)((uintptr_t)u + S_PEON_FLAGS));
        if ((pf & PEON_HARVEST_GOLD) == 0)
        {
            int* t = (int*)*((int*)((uintptr_t)u + S_ORDER_UNIT_POINTER));
            if (t)
            {
                byte tid = *((byte*)((uintptr_t)t + S_ID));
                if ((tid != U_PEASANT) && (tid != U_PEON))
                {
                    int pflag = *(int*)(UNIT_GLOBAL_FLAGS + tid * 4);
                    if (pflag & IS_FLESHY)
                    {
                        pf |= PEON_HARVEST_GOLD;
                        set_stat(u, pf, S_PEON_FLAGS);
                        set_stat(t, 5, S_PEON_BUILD);
                        set_stat(t, 255, S_ANIMATION_TIMER);
                        sound_play_from_file(0, (DWORD)arrow_name, arrow_sound);
                        set_stat(u, ORDER_STOP, S_NEXT_ORDER);
                        ((void (*)())F_STATUS_REDRAW)();//status redraw
                    }
                }
            }
        }
    }
    else((void (*)(int*))0x00442CF0)(u);//orig blood
    //set_stat(u, ORDER_STOP, S_NEXT_ORDER);
}

void magic_raise(int* u)
{
    byte caster = *((byte*)((uintptr_t)u + S_ID));
    if (caster == U_GRIFON)
    {
        byte mp = *((byte*)((uintptr_t)u + S_MANA));
        if (mp >= BREATH_MANACOST)
        {
            mp -= BREATH_MANACOST;
            set_stat(u, mp, S_MANA);

            WORD x1 = *((WORD*)((uintptr_t)u + S_X)) * 32 + 16;
            WORD y1 = *((WORD*)((uintptr_t)u + S_Y)) * 32 + 16;
            WORD x2 = *((WORD*)((uintptr_t)u + S_ORDER_X)) * 32 + 16;
            WORD y2 = *((WORD*)((uintptr_t)u + S_ORDER_Y)) * 32 + 16;
            float dx = ((int)x2 - (int)x1) / (float)14.0;
            float dy = ((int)y2 - (int)y1) / (float)14.0;
            short ddx = (short)(dx * 4);
            short ddy = (short)(dy * 4);
            for (int i = 0; i < 10; i++)
            {
                for (int j = 0; (j <= i) && (j <= 7); j++)
                {
                    int* b = bullet_create(
                        x1 + ddx - (3 + 3 * i) + (((int (*)())F_NET_RANDOM)() % (6 + 6 * i)),
                        y1 + ddy - (3 + 3 * i) + (((int (*)())F_NET_RANDOM)() % (6 + 6 * i)),
                        B_LIGHT_FIRE);
                    if (b)
                    {
                        char buf[] = "\x0";
                        buf[0] = 4;
                        PATCH_SET((char*)((uintptr_t)b + 55), buf);//55 bullet damage
                        buf[0] = 180 + 6 * i + (((int (*)())F_NET_RANDOM)() % 12);//ticks
                        PATCH_SET((char*)((uintptr_t)b + 56), buf);//56 bullet life (WORD)
                        patch_setdword((DWORD*)((uintptr_t)b + 48), (DWORD)u);//caster = me
                    }
                }
                ddx = (short)(dx * (5 + i));
                ddy = (short)(dy * (5 + i));
            }

            sound_play_from_file(0, (DWORD)breath_name, breath_sound);
            set_stat(u, ORDER_STOP, S_NEXT_ORDER);
        }
        else set_stat(u, ORDER_STOP, S_NEXT_ORDER);
    }
    else((void (*)(int*))0x00442D80)(u);//orig raise
    //set_stat(u, ORDER_STOP, S_NEXT_ORDER);
}

void magic_viz(int* u)
{
    byte caster = *((byte*)((uintptr_t)u + S_ID));
    if (caster == U_MAGE)
    {
        byte mp = *((byte*)((uintptr_t)u + S_MANA));
        if (mp >= TP_MANACOST)
        {
            byte o = *((byte*)((uintptr_t)u + S_OWNER));
            byte x = *((byte*)((uintptr_t)u + S_ORDER_X));
            byte y = *((byte*)((uintptr_t)u + S_ORDER_Y));
            set_region(x - 4, y - 4, x + 4, y + 4);
            find_all_alive_units(ANY_UNITS);
            sort_stat(S_OWNER, o, CMP_EQ);
            sort_in_region();
            if (units > 0)
            {
                if (unit_move(x, y, u))
                {
                    ((void (*)(int*, byte))F_SPELL_SOUND_UNIT)(u, SS_THUNDER);
                    //sound_play_from_file(0, (DWORD)bliz_name, bliz_sound);
                    mp -= TP_MANACOST;
                    set_stat(u, mp, S_MANA);
                }
            }
        }
        set_stat(u, ORDER_STOP, S_NEXT_ORDER);
    }
    else((void (*)(int*))0x00442490)(u);//orig viz
}

void magic_actions_init()
{
    void (*r1) (int*) = magic_bliz;
    void (*r2) (int*) = magic_blood;
    void (*r3) (int*) = magic_raise;
    void (*r4) (int*) = magic_viz;
    char buf[] = "\x0\x0\x0\x0";
    patch_setdword((DWORD*)buf, (DWORD)r1);
    PATCH_SET((char*)(GF_SPELLS_FUNCTIONS + 4 * ORDER_SPELL_BLIZZARD), buf);
    patch_setdword((DWORD*)buf, (DWORD)r2);
    PATCH_SET((char*)(GF_SPELLS_FUNCTIONS + 4 * ORDER_SPELL_BLOODLUST), buf);
    patch_setdword((DWORD*)buf, (DWORD)r3);
    PATCH_SET((char*)(GF_SPELLS_FUNCTIONS + 4 * ORDER_SPELL_RAISEDEAD), buf);
    patch_setdword((DWORD*)buf, (DWORD)r4);
    PATCH_SET((char*)(GF_SPELLS_FUNCTIONS + 4 * ORDER_SPELL_VISION), buf);
}

void buttons_init_all()
{
    buttons_init_prince();
    buttons_init_warrior();
    buttons_init_hero();
    buttons_init_barak();
    buttons_init_smith();
    buttons_init_stables();
    buttons_init_aviary();
    buttons_init_ace();
    buttons_init_ballista();
    buttons_init_archer();
    buttons_init_knight();
    buttons_init_paladin();
    buttons_init_lumber();
    buttons_init_mage();
    buttons_init_th();

    magic_actions_init();

    DWORD f = *(DWORD*)(0x004A3D58 + 12);
    patch_setdword((DWORD*)(0x004A3D58 + 16 * U_BALLISTA + 12), f);
}

#define SPELLS_LEARN_BITS_TABLE 0x004CFFCC

#define BLDG_FINISH_SPELL 0x004948C8
DWORD build_finish_spell_orig;
void build_finish_spell(int* u, int a)
{
    byte bor = *((byte*)((uintptr_t)u + S_BUILD_TYPE));
    DWORD orig_bit = *(DWORD*)(SPELLS_LEARN_BITS_TABLE + 4 * bor);
    byte id = *((byte*)((uintptr_t)u + S_ID));
    if ((id == U_HSMITH) && (bor == UG_SP_ROT))
    {
        patch_setdword((DWORD*)(SPELLS_LEARN_BITS_TABLE + 4 * bor), (DWORD)(1 << L_SPELL_21));
    }
    ((void (*)(int*, int))build_finish_spell_orig)(u, a);//original
    patch_setdword((DWORD*)(SPELLS_LEARN_BITS_TABLE + 4 * bor), orig_bit);
}

#define BLDG_COST_SPELL 0x004948B8
DWORD build_cost_spell_orig;
void build_cost_spell(int* u, byte type)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    if ((id == U_HSMITH) && (type == UG_SP_52))
    {
        type = UG_SP_ROT;
    }
    ((void (*)(int*, byte))build_cost_spell_orig)(u, type);//original
}

PROC g_proc_00455F9C;
int build_start_build(int* u, byte type, byte ordr)
{
    int original = 0;
    bool orig = true;
    byte id = *((byte*)((uintptr_t)u + S_ID));
    if ((id == U_HSMITH) && (ordr == 1) && (type == UG_SP_52))
    {
        type = UG_SP_ROT;
        DWORD orig_bit = *(DWORD*)(SPELLS_LEARN_BITS_TABLE + 4 * type);
        patch_setdword((DWORD*)(SPELLS_LEARN_BITS_TABLE + 4 * type), (DWORD)(1 << L_SPELL_21));
        original = ((int (*)(int*, byte, byte))g_proc_00455F9C)(u, type, ordr);//original
        patch_setdword((DWORD*)(SPELLS_LEARN_BITS_TABLE + 4 * type), orig_bit);
        orig = false;
    }
    if (orig)return ((int (*)(int*, byte, byte))g_proc_00455F9C)(u, type, ordr);//original
    return original;
}

//004A3D58 portrait cards (icon 2b frame_type 2b str_id 4b function1 4b function2 4b)
PROC g_proc_00420F02;
void status_port_draw()
{
    //int* u = (int*)*(int*)0x004BDC78;//status unit
    int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    if (u)
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if (id == U_BALLISTA)
        {
            patch_setdword((DWORD*)0x00445BB8, (DWORD)0x4947A8);
            char buf[] = "\x6";
            PATCH_SET((char*)0x0044601C, buf);
        }
    }
    ((void (*)())g_proc_00420F02)();//original
    patch_setdword((DWORD*)0x00445BB8, (DWORD)0x00494794);
    char buf[] = "\x1";
    PATCH_SET((char*)0x0044601C, buf);
}

PROC g_proc_0044ACA3;
PROC g_proc_0044B9F2;
void button_description(int a, int b_strc)
{
    DWORD but = 0;
    byte orig_bliz = get_manacost(BLIZZARD);
    byte orig_raise = get_manacost(RAISE_DEAD);
    byte orig_viz = get_manacost(VISION);
    if (b_strc != 0)but = *(DWORD*)(b_strc + 0x26);
    if (but == (DWORD)(buttons_ballista + 10 * BUTTON_SIZE))
    {
        manacost(BLIZZARD, 120);
    }
    else if (but == (DWORD)(buttons_ace + 10 * BUTTON_SIZE))
    {
        manacost(RAISE_DEAD, BREATH_MANACOST);
    }
    else if (but == (DWORD)(buttons_mage + 4 * BUTTON_SIZE))
    {
        manacost(VISION, TP_MANACOST);
    }
    else if (but == (DWORD)(buttons_smith + 6 * BUTTON_SIZE))
    {
        buttons_smith[6 * BUTTON_SIZE + 13] = UG_SP_ROT;
    }
    ((void (*)(int, int))g_proc_0044ACA3)(a, b_strc);//original
    //if (but == (DWORD)(buttons_ballista + 11 * BUTTON_SIZE))
    manacost(BLIZZARD, orig_bliz);
    manacost(RAISE_DEAD, orig_raise);
    manacost(VISION, orig_viz);
    if (but == (DWORD)(buttons_smith + 6 * BUTTON_SIZE))
    {
        buttons_smith[6 * BUTTON_SIZE + 13] = UG_SP_52;
    }
}

unsigned char* tip_line1 = NULL;
unsigned char* tip_line2 = NULL;
unsigned char* tip_line3 = NULL;
unsigned char* tip_line4 = NULL;
int tip_status = 0;
int tip_w = 0;
int tip_h = 0;
int tip_wspeed = 6;
int tip_hspeed = 1;

unsigned char* text_from_tbl(void* tbl, int str_id)
{
    return ((unsigned char* (*)(void*, int))F_GET_LINE_FROM_TBL)(tbl, str_id);
}

unsigned char* text_from_tutorial(int str_id)
{
    if (str_id == -1)return NULL;
    else return text_from_tbl(tbl_tutorial, str_id);
}

//write your custom victory functions here
//-------------------------------------------------------------------------------
void v_human1(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        find_all_alive_units(U_KEEP);
        sort_stat(S_COLOR, P_ORANGE, CMP_EQ);
        if (units == 0)
        {
            if (*(byte*)GB_HORSES == 0)
            {
                unit_create(13, 10, U_KARGATH, *(byte*)LOCAL_PLAYER, 1);
                *(byte*)GB_HORSES = 1;
            }
            set_region(77, 88, 78, 89);
            find_all_alive_units(U_KARGATH);
            if (units == 0) lose(true);
            else
            {
                set_stat_all(S_COLOR, P_ORANGE);
                sort_in_region();
                if (units != 0)win(true);
            }
        }
        else viz_area_add(13, 11, 1 << P_RED, 2);
        WORD xx = 80;
        WORD yy = 89;
        find_all_alive_units(U_KARGATH);
        if (units != 0)
        {
            xx = *((WORD*)((uintptr_t)unit[0] + S_X));
            yy = *((WORD*)((uintptr_t)unit[0] + S_Y));
        }
        find_all_alive_units(ANY_MEN);
        sort_stat(S_OWNER, P_BLUE, CMP_EQ);
        sort_preplaced();
        sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_EQ);
        //sort_stat(S_ANIMATION, ANIM_STOP, CMP_EQ);
        for (int i = 0; i < units; i++)
        {
            struct GPOINT
            {
                WORD x;
                WORD y;
            } l;
            l.x = xx;
            l.y = yy;
            ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
        }
        if (*(byte*)LEVEL_OBJ == LVL_HUMAN1)
        {
            *(WORD*)(FOOD_LIMIT + 2 * P_WHITE) = 100;
            find_all_alive_units(U_DRAGONROOST);
            if (units != 0)
            {
                find_all_alive_units(U_DRAGON);
                sort_preplaced();
                if (units == 0)
                {
                    //unit_create(5, 92, U_DRAGON, P_WHITE, 1);
                    find_all_alive_units(U_DRAGONROOST);
                    building_start_build(unit[0], U_DRAGON, 0);
                }
                else
                {
                    for (int i = 0; i < units; i++)
                    {
                        struct GPOINT
                        {
                            WORD x;
                            WORD y;
                        } l;
                        l.x = xx;
                        l.y = yy;
                        ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
                    }
                }
            }
        }
    }
}

char arh_name[] = "RedMist\\arh.wav\x0";
void* arh_sound = NULL;

void v_human2(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        if ((get_val(TH1, *(byte*)LOCAL_PLAYER) >= 1) && (get_val(FARM, *(byte*)LOCAL_PLAYER) >= 10))call_default_kill();
    }
}

void v_human3(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        comps_vision(true);
        viz(P_RED, P_ORANGE, 1);
        set_region(94, 73, 95, 74);
        find_all_alive_units(U_HTRANSPORT);
        if (units == 0)lose(true);
        sort_in_region();
        if (units > 0)
        {
            call_default_kill();
        }
    }
}

void v_human4(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        set_region(0, 0, 36, 24);
        find_all_alive_units(U_KEEP);
        sort_in_region();
        if (units >= 1)
        {
            bool f = true;
            for (int i = 0; i < 8; i++)
            {
                if (i != *(byte*)LOCAL_PLAYER)
                {
                    if (get_val(SHIPYARD, i) != 0)f = false;
                    if (get_val(FOUNDRY, i) != 0)f = false;
                    if (get_val(REFINERY, i) != 0)f = false;
                    if (get_val(OIL_PLATFORM, i) != 0)f = false;
                    if (get_val(TANKER, i) != 0)f = false;
                    if (get_val(TRANSPORT, i) != 0)f = false;
                    if (get_val(DESTROYER, i) != 0)f = false;
                    if (get_val(BATTLESHIP, i) != 0)f = false;
                    if (get_val(SUBMARINE, i) != 0)f = false;
                }
            }
            if (f)win(true);
        }
    }
}

void v_human5(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        //if (!slot_alive(P_ORANGE))win(true);
        if ((get_val(TH1, P_ORANGE) == 0) && (get_val(TH2, P_ORANGE) == 0) && (get_val(TH3, P_ORANGE) == 0) &&
            (get_val(BARRACKS, P_ORANGE) == 0) && (get_val(SHIPYARD, P_ORANGE) == 0))win(true);
    }
}

void v_human6(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        byte l = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(l))lose(true);
        else
        {
            if (!check_opponents(l))
            {
                *(byte*)LEVEL_OBJ = LVL_HUMAN13;
                *(WORD*)LEVEL_ID = 0x52E0;
                win(true);
            }
        }
    }
}

void v_human7(bool rep_init)
{
    if (rep_init)
    {
        //ai_fix_plugin(true);
        //saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_human8(bool rep_init)
{
    if (rep_init)
    {
        //ai_fix_plugin(true);
        //saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_human9(bool rep_init)
{
    if (rep_init)
    {
        //ai_fix_plugin(true);
        //saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        win(true);
    }
}

char line_kills[] = "             ";
char g1_name[] = "RedMist\\g1.wav\x0";
void* g1_sound = NULL;
char g2_name[] = "RedMist\\g2.wav\x0";
void* g2_sound = NULL;
char g3_name[] = "RedMist\\g3.wav\x0";
void* g3_sound = NULL;

void v_human10(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
        tip_status = 0;
    }
    else
    {
        //your custom victory conditions
        byte l = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(l))
        {
            *(byte*)LEVEL_OBJ = LVL_HUMAN14;
            *(WORD*)LEVEL_ID = 0x52E2;
            lose(true);
        }
        else
        {
            if (get_val(KILLS_UNITS, l) >= 1500)
            {
                if (*(byte*)(GB_HORSES + 10) == 0)
                {
                    *(byte*)(GB_HORSES + 10) = 1;
                    sound_play_from_file(300, (DWORD)g3_name, g3_sound);
                }
            }
            if (get_val(KILLS_UNITS, l) >= 4000)
            {
                if (*(byte*)(GB_HORSES + 11) == 0)
                {
                    *(byte*)(GB_HORSES + 11) = 1;
                    sound_play_from_file(300, (DWORD)g1_name, g1_sound);
                }
            }
            if (get_val(KILLS_UNITS, l) >= 5000)
            {
                sound_play_from_file(300, (DWORD)g2_name, g2_sound);
                *(byte*)LEVEL_OBJ = LVL_HUMAN13;
                *(WORD*)LEVEL_ID = 0x52E0;
                win(true);
            }
        }

        if (*(byte*)(GB_HORSES + 2) == 0)
        {
            *(byte*)(GB_HORSES + 2) = 1;
            tile_remove_trees(58, 42);
            tile_remove_trees(52, 54);
            tile_remove_trees(53, 55);
            tile_remove_trees(52, 55);
            tile_remove_trees(53, 56);
            tile_remove_trees(52, 56);
            tile_remove_trees(53, 57);
            tile_remove_trees(51, 57);
            tile_remove_trees(52, 58);
            tile_remove_trees(50, 58);
            tile_remove_trees(51, 59);
            tile_remove_trees(49, 59);
            tile_remove_trees(49, 60);
            tile_remove_trees(50, 60);
            tile_remove_trees(50, 61);
            tile_remove_trees(50, 62);
        }

        if (tip_status == 0)tip_status = 3;
        sprintf(line_kills, "Kills: %04d", get_val(KILLS_UNITS, l));
        tip_line1 = (unsigned char*)line_kills;
        tip_line2 = NULL;
        tip_line3 = NULL;
        tip_line4 = NULL;

        ally(P_VIOLET, P_ORANGE, 1);
        ally(P_VIOLET, P_BLACK, 1);
        set_upgrade(SWORDS, P_VIOLET, 2);
        set_upgrade(ARMOR, P_VIOLET, 2);
        set_upgrade(ARROWS, P_VIOLET, 2);

        *(byte*)(CONTROLER_TYPE + P_VIOLET) = C_COMP;
        *(byte*)(STARTING_CONTROLER_TYPE + P_VIOLET) = C_COMP;
        if (*(byte*)GB_HORSES < 66)
        {
            *(byte*)GB_HORSES = *(byte*)GB_HORSES + 1;
        }
        else
        {
            *(byte*)GB_HORSES = 0;
            if (*(byte*)(GB_HORSES + 1) < 200)*(byte*)(GB_HORSES + 1) = *(byte*)(GB_HORSES + 1) + 1;
            byte unc = U_GRUNT;
            if (*(byte*)(GB_HORSES + 1) < 20)
            {
                if ((*(byte*)(GB_HORSES + 1) % 2) == 0)unc = U_GRUNT;
                else unc = U_TROLL;
            }
            else
            {
                unc = U_OGRE;
            }
            if (*(byte*)(GB_HORSES + 1) > 50)unit_create(56, 28, U_DK, P_VIOLET, *(byte*)(GB_HORSES + 1) / 8);
            unit_create(56, 28, unc, P_VIOLET, *(byte*)(GB_HORSES + 1));
            if (*(byte*)(GB_HORSES + 1) > 30)unit_create(56, 28, U_GOBLINS, P_VIOLET, *(byte*)(GB_HORSES + 1) / 8);
            find_all_alive_units(ANY_MEN);
            sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
            set_stat_all(S_COLOR, P_BLACK);
            for (int i = 0; i < units; i++)
            {
                struct GPOINT
                {
                    WORD x;
                    WORD y;
                } l;
                l.x = 56;//26;
                l.y = 28;//92;
                ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
            }
        }
    }
}

void v_human11(bool rep_init)
{
    if (rep_init)
    {
        //ai_fix_plugin(true);
        //saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_human12(bool rep_init)
{
    if (rep_init)
    {
        //ai_fix_plugin(true);
        //saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_human13(bool rep_init)
{
	if (rep_init)
	{
        //ai_fix_plugin(true);
        //saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        win(true);
	}
}

void v_human14(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        byte l = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(l))lose(true);
        else
        {
            if (!check_opponents(l))
            {
                find_all_alive_units(U_PEASANT);
                sort_stat(S_OWNER, P_RED, CMP_NEQ);
                if (units != 0)win(true);
            }
        }

        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_TERON);
            if (units == 0)
            {
                *(byte*)GB_HORSES = 1;
                sound_play_from_file(0, (DWORD)arh_name, arh_sound);
            }
        }
        else if (*(byte*)GB_HORSES == 1)
        {
            *(byte*)GB_HORSES = 2;
        }
        else if (*(byte*)GB_HORSES == 2)
        {
            *(byte*)GB_HORSES = 3;
        }
        else if (*(byte*)GB_HORSES == 3)
        {
            *(byte*)GB_HORSES = 4;
        }
        else if (*(byte*)GB_HORSES == 4)
        {
            *(byte*)GB_HORSES = 5;
        }
        else if (*(byte*)GB_HORSES == 5)
        {
            *(byte*)GB_HORSES = 6;
        }
        else if (*(byte*)GB_HORSES == 6)
        {
            //*(byte*)LEVEL_OBJ = LVL_HUMAN9;
            //*(WORD*)LEVEL_ID = 0x52D8;
            //win(true);
            *(byte*)LEVEL_OBJ = LVL_HUMAN10;
            *(WORD*)LEVEL_ID = 0x52DA;
            lose(true);
        }
	}
}

void v_xhuman1(bool rep_init)
{
    v_human1(rep_init);
}

void v_xhuman2(bool rep_init)
{
    v_human2(rep_init);
}

void v_xhuman3(bool rep_init)
{
    v_human3(rep_init);
}

void v_xhuman4(bool rep_init)
{
    v_human4(rep_init);
}

void v_xhuman5(bool rep_init)
{
    v_human5(rep_init);
}

void v_xhuman6(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        byte l = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(l))lose(true);
        else
        {
            if (!check_opponents(l))
            {
                *(byte*)LEVEL_OBJ = LVL_XHUMAN11;
                *(WORD*)LEVEL_ID = 0x53DA;
                win(true);
            }
        }
    }
}

void v_xhuman7(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xhuman8(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xhuman9(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xhuman10(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xhuman11(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
        win(true);
    }
}

void v_xhuman12(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        byte l = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(l))lose(true);
        else
        {
            if (!check_opponents(l))
            {
                find_all_alive_units(U_PEASANT);
                sort_stat(S_OWNER, P_RED, CMP_NEQ);
                if (units != 0)
                {
                    *(byte*)LEVEL_OBJ = LVL_HUMAN14;
                    *(WORD*)LEVEL_ID = 0x52E2;
                    win(true);
                }
            }
        }

        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_TERON);
            if (units == 0)
            {
                *(byte*)GB_HORSES = 1;
                sound_play_from_file(0, (DWORD)arh_name, arh_sound);
            }
        }
    }
}

char tutorial_name[] = "RedMist\\tutorial.wav\x0";
void* tutorial_sound = NULL;

void tutorial_lines(int l1, int l2, int l3, int l4)
{
    tip_status = 3;
    tip_line1 = text_from_tutorial(l1);
    tip_line2 = text_from_tutorial(l2);
    tip_line3 = text_from_tutorial(l3);
    tip_line4 = text_from_tutorial(l4);
    sound_play_from_file(0, (DWORD)tutorial_name, tutorial_sound);
}

void gamesound_play(WORD s, byte x, byte y)
{
    DWORD xy = x + 256 * 256 * y;
    ((void (*)(WORD, DWORD, byte, byte))F_GAMESOUND_XY)(s, xy, 1, 1);
}

void v_orc1(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        ut[0] = U_RUNESTONE;
        ua[0] = U_RUNESTONE;
        ut[1] = U_MINE;
        ua[1] = U_MINE;
        ut[2] = U_AVIARY;
        ua[2] = U_AVIARY;
        ut[3] = U_HFOUNDRY;
        ua[3] = U_HFOUNDRY;
        
        if (cmp_res(P_RED, 2, 0, 0, 0, 0, CMP_EQ))
        {
            find_all_alive_units(U_HTRANSPORT);
            if (units > 1)kill_all();
            else if (units == 0)
            {
                if (*(byte*)GB_HORSES >= 17)set_res(P_RED, 2, 10, 0, 0, 0);
            }
        }

        comps_vision(true);
        ally(P_RED, P_BLACK, 1);
        viz(P_RED, P_BLACK, 1);
        find_all_alive_units(ANY_UNITS);
        sort_stat(S_COLOR, P_ORANGE, CMP_EQ);
        set_stat_all(S_COLOR, P_RED);
        find_all_alive_units(U_MINE);
        set_stat_all(S_RESOURCES, 100);
        find_all_alive_units(ANY_UNITS);
        sort_stat(S_COLOR, P_BLACK, CMP_EQ);
        set_stat_all(S_COLOR, P_RED);
        set_res(P_BLACK, 0, 0, 0, 0, 0);
        find_all_alive_units(U_PEASANT);
        sort_stat(S_OWNER, P_RED, CMP_EQ);
        if (units == 0)
        {
            if (*(byte*)(GB_HORSES + 5) == 0)
            {
                unit_create(12, 92, U_PEASANT, P_RED, 1);
                if (w_sounds_e)sound_play_from_file(0, (DWORD)wr_name, wr_sound);
                else gamesound_play(224, 12, 92);
                find_all_alive_units(U_PEASANT);
                sort_stat(S_OWNER, P_RED, CMP_EQ);
                order_all(31, 68, ORDER_MOVE);
            }
        }
        else
        {
            sort_stat(S_PEON_TREE_CHOPS, 1, CMP_BIGGER);
            if (*(byte*)GB_HORSES < 8)set_stat_all(S_PEON_TREE_CHOPS, 1);
        }
        find_all_alive_units(U_DANATH);
        if (units == 0)
        {
            unit_create(12, 92, U_DANATH, P_RED, 1);
            sound_play_from_file(0, (DWORD)hp1_name, hp1_sound);
            find_all_alive_units(U_DANATH);
            set_stat_all(S_COLOR, P_GREEN);
            order_all(31, 68, ORDER_ATTACK_AREA);
        }
        find_all_alive_units(U_RANGER);
        if (units == 0)
        {
            unit_create(12, 92, U_RANGER, P_RED, 1);
            find_all_alive_units(U_RANGER);
            order_all(31, 68, ORDER_ATTACK_AREA);
        }
        
        if (*(byte*)GB_HORSES == 0)
        {
            tip_status = 0;
            *(byte*)GB_HORSES = 1;
        }
        else if (*(byte*)GB_HORSES == 1)
        {
            find_all_alive_units(U_DANATH);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units != 0)
            {
                find_all_alive_units(U_FARM);
                sort_stat(S_OWNER, P_RED, CMP_NEQ);
                give_all(P_RED);
                tip_status = 0;
                *(byte*)GB_HORSES = 2;
            }
            
        }
        else if (*(byte*)GB_HORSES == 2)
        {
            find_all_alive_units(U_SKELETON);
            sort_stat(S_OWNER, P_WHITE, CMP_EQ);
            set_region(22, 64, 40, 74);
            sort_in_region();
            if (units == 0)
            {
                tip_status = 0;
                *(byte*)GB_HORSES = 3;
            }
        }
        else if (*(byte*)GB_HORSES == 3)
        {
            viz_area_add(36, 83, 1 << P_RED, 9);
            find_all_alive_units(U_SKELETON);
            sort_stat(S_OWNER, P_WHITE, CMP_EQ);
            set_region(28, 78, 41, 89);
            sort_in_region();
            if (units == 0)
            {
                find_all_alive_units(U_DANATH);
                set_stat_all(S_MANA, 255);
                find_all_alive_units(U_BALLISTA);
                set_stat_all(S_MANA, 255);
                tip_status = 0;
                *(byte*)GB_HORSES = 4;
            }
        }
        else if (*(byte*)GB_HORSES == 4)
        {
            viz_area_add(60, 83, 1 << P_RED, 3);
            find_all_alive_units(U_DK);
            sort_stat(S_OWNER, P_WHITE, CMP_EQ);
            set_region(45, 76, 62, 95);
            sort_in_region();
            if (units == 0)
            {
                tip_status = 0;
                *(byte*)GB_HORSES = 5;
                find_all_alive_units(U_BALLISTA);
                set_stat_all(S_HP, 30);
            }
        }
        else if (*(byte*)GB_HORSES == 5)
        {
            viz_area_add(64, 65, 1 << P_RED, 9);
            viz_area_add(56, 60, 1 << P_RED, 9);
            find_all_alive_units(U_BALLISTA);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units != 0)
            {
                tip_status = 0;
                *(byte*)GB_HORSES = 6;
            }
        }
        else if (*(byte*)GB_HORSES == 6)
        {
            viz_area_add(64, 65, 1 << P_RED, 9);
            viz_area_add(56, 60, 1 << P_RED, 9);
            find_all_alive_units(U_BALLISTA);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units == 0)
            {
                unit_create(12, 92, U_BALLISTA, P_RED, 1);
                find_all_alive_units(U_BALLISTA);
                set_stat_all(S_HP, 25);
                order_all(31, 68, ORDER_ATTACK_AREA);
            }
            find_all_alive_units(U_OLUMBER);
            sort_stat(S_OWNER, P_WHITE, CMP_EQ);
            set_region(73, 56, 77, 60);
            sort_in_region();
            if (units == 0)
            {
                find_all_alive_units(U_BALLISTA);
                set_stat_all(S_MANA, 255);
                tip_status = 0;
                *(byte*)GB_HORSES = 7;
            }
            else
            {
                find_all_alive_units(U_TROLL);
                sort_stat(S_OWNER, P_WHITE, CMP_EQ);
                if (units < 20)unit_create(74, 57, U_TROLL, P_WHITE, 1);
            }
        }
        else if (*(byte*)GB_HORSES == 7)
        {
            viz_area_add(64, 65, 1 << P_RED, 9);
            viz_area_add(56, 60, 1 << P_RED, 9);
            viz_area_add(80, 57, 1 << P_RED, 9);
            find_all_alive_units(U_BALLISTA);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units == 0)
            {
                unit_create(12, 92, U_BALLISTA, P_RED, 1);
                find_all_alive_units(U_BALLISTA);
                set_stat_all(S_MANA, 255);
                order_all(31, 68, ORDER_ATTACK_AREA);
            }
            find_all_alive_units(U_OTOWER);
            sort_stat(S_OWNER, P_WHITE, CMP_EQ);
            set_region(80, 57, 83, 60);
            sort_in_region();
            if (units == 0)
            {
                *(byte*)GB_HORSES = 8;
            }
        }
        else if (*(byte*)GB_HORSES == 8)
        {
            viz_area_add(64, 65, 1 << P_RED, 9);
            viz_area_add(56, 60, 1 << P_RED, 9);
            viz_area_add(80, 57, 1 << P_RED, 9);
            find_all_alive_units(U_DRAGON);
            sort_stat(S_OWNER, P_WHITE, CMP_EQ);
            set_region(42, 54, 46, 58);
            sort_in_region();
            give_all(P_NEUTRAL);
            find_all_alive_units(U_DRAGON);
            sort_stat(S_OWNER, P_NEUTRAL, CMP_EQ);
            sort_stat(S_ORDER, ORDER_STOP, CMP_EQ);
            if (units != 0)
            {
                int* u = unit[0];
                find_all_alive_units(U_BALLISTA);
                sort_stat(S_OWNER, P_RED, CMP_EQ);
                if (units != 0)
                {
                    int* u2 = unit[0];
                    byte x = *((byte*)((uintptr_t)u2 + S_X));
                    byte y = *((byte*)((uintptr_t)u2 + S_Y));
                    give_order(u, x, y, ORDER_MOVE);
                    byte x1 = *((byte*)((uintptr_t)u + S_X));
                    byte y1 = *((byte*)((uintptr_t)u + S_Y));
                    set_region(x1 - 1, y1 - 1, x1 + 1, y1 + 1);
                    WORD xx = *((WORD*)((uintptr_t)u2 + S_DRAW_X)) / 32;
                    WORD yy = *((WORD*)((uintptr_t)u2 + S_DRAW_Y)) / 32;
                    if (in_region((byte)xx, (byte)yy, reg[0], reg[1], reg[2], reg[3]))
                    {
                        unit_kill(u);
                        unit_kill(u2);
                    }
                }
            }
            find_all_alive_units(U_BALLISTA);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units == 0)
            {
                find_all_alive_units(U_DRAGON);
                sort_stat(S_OWNER, P_NEUTRAL, CMP_EQ);
                kill_all();
                for (int i = 59; i < 63; i++)for (int j = 68; j < 73; j++)tile_remove_trees(i, j);
                tip_status = 0;
                *(byte*)GB_HORSES = 9;
            }
        }
        else if (*(byte*)GB_HORSES == 9)
        {
            viz_area_add(64, 76, 1 << P_RED, 9);
            viz_area_add(81, 46, 1 << P_RED, 9);
            find_all_alive_units(U_DRAGON);
            sort_stat(S_OWNER, P_WHITE, CMP_EQ);
            if (units == 0)
            {
                find_all_alive_units(U_RUNESTONE);
                set_region(89, 44, 90, 45);
                sort_in_region();
                kill_all();
                tip_status = 0;
                *(byte*)GB_HORSES = 10;
            }
        }
        else if (*(byte*)GB_HORSES == 10)
        {
            find_all_alive_units(U_OGRE);
            if (units == 0)
            {
                find_all_alive_units(U_CATAPULT);
                if (units == 0)
                {
                    find_all_alive_units(U_HARROWTOWER);
                    sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
                    give_all(P_RED);
                    find_all_alive_units(U_MAGE_TOWER);
                    sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
                    give_all(P_RED);
                    set_stat_all(S_HP, 2000);
                    tip_status = 0;
                    *(byte*)GB_HORSES = 11;
                }
            }
        }
        else if (*(byte*)GB_HORSES == 11)
        {
            viz_area_add(74, 8, 1 << P_RED, 9);
            if (true)
            {
                for (int i = 46; i < 49; i++)for (int j = 6; j < 15; j++)tile_remove_rocks(i, j);
                find_all_alive_units(U_FOOTMAN);
                sort_stat(S_OWNER, P_BLUE, CMP_EQ);
                sort_stat(S_ANIMATION, ANIM_STOP, CMP_EQ);
                for (int i = 0; i < units; i++)
                {
                    struct GPOINT
                    {
                        WORD x;
                        WORD y;
                    } l;
                    l.x = 72;
                    l.y = 8;
                    ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
                }
                //tip_status = 0;
                *(byte*)GB_HORSES = 12;
            }
        }
        else if (*(byte*)GB_HORSES == 12)
        {
            viz_area_add(74, 8, 1 << P_RED, 9);
            viz_area_add(42, 10, 1 << P_RED, 9);
            viz_area_add(32, 8, 1 << P_RED, 9);
            find_all_alive_units(U_FOOTMAN);
            sort_stat(S_OWNER, P_BLUE, CMP_EQ);
            if (units == 0)
            {
                find_all_alive_units(U_HFOUNDRY);
                sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
                give_all(P_RED);
                unit_create(55, 9, U_MAGE, P_RED, 4);
                find_all_alive_units(U_MAGE);
                set_stat_all(S_MANA, 255);
                tip_status = 0;
                *(byte*)GB_HORSES = 13;
            }
            find_all_alive_units(U_HARROWTOWER);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units == 0)
            {
                find_all_alive_units(U_FOOTMAN);
                sort_stat(S_OWNER, P_BLUE, CMP_EQ);
                kill_all();
            }
            find_all_alive_units(U_MAGE_TOWER);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            set_stat_all(S_HP, 2000);
        }
        else if (*(byte*)GB_HORSES == 13)
        {
            viz_area_add(74, 8, 1 << P_RED, 9);
            find_all_alive_units(U_MAGE);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units == 0)unit_create(55, 9, U_MAGE, P_RED, 1);
            find_all_alive_units(U_MAGE_TOWER);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            set_stat_all(S_HP, 2000);
            find_all_alive_units(ANY_UNITS);
            sort_stat(S_OWNER, P_BLUE, CMP_EQ);
            set_region(60, 0, 86, 13);
            sort_in_region();
            if (units == 0)
            {
                set_region(64, 37, 65, 42);
                find_all_alive_units(U_RUNESTONE);
                sort_in_region();
                kill_all();
                tip_status = 0;
                *(byte*)GB_HORSES = 14;
            }
        }
        else if (*(byte*)GB_HORSES == 14)
        {
            viz_area_add(38, 38, 1 << P_RED, 9);
            find_all_alive_units(U_MAGE);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units == 0)unit_create(55, 9, U_MAGE, P_RED, 1);
            find_all_alive_units(U_MAGE);
            sort_stat(S_OWNER, P_BLUE, CMP_EQ);
            set_region(32, 32, 61, 49);
            sort_in_region();
            if (units == 0)
            {
                tip_status = 0;
                *(byte*)GB_HORSES = 15;
            }
        }
        else if (*(byte*)GB_HORSES == 15)
        {
            find_all_alive_units(U_GRIFON);
            if (units != 0)
            {
                if (units > 1)kill_all();
                find_all_alive_units(U_MAGE);
                sort_stat(S_OWNER, P_RED, CMP_EQ);
                if (units != 0)((void (*)(int*, byte))F_SPELL_SOUND_UNIT)(unit[0], SS_THUNDER);
                remove_all();
            }
            else set_res(P_RED, 2, 10, 0, 0, 0);
            viz_area_add(90, 90, 1 << P_RED, 9);
            find_all_alive_units(U_MAGE_TOWER);
            sort_stat(S_OWNER, P_BLUE, CMP_EQ);
            if (units == 0)
            {
                set_res(P_RED, 2, 0, 0, 0, 0);
                set_region(45, 47, 46, 52);
                find_all_alive_units(U_RUNESTONE);
                sort_in_region();
                kill_all();
                find_all_alive_units(U_GRIFON);
                kill_all();
                tip_status = 0;
                *(byte*)GB_HORSES = 16;
            }
        }
        else if (*(byte*)GB_HORSES == 16)
        {
            find_all_alive_units(U_GRIFON);
            kill_all();
            find_all_alive_units(U_AVIARY);
            kill_all();
            viz_area_add(44, 56, 1 << P_RED, 9);
            viz_area_add(62, 17, 1 << P_RED, 3);
            find_all_alive_units(U_SHIPYARD);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            set_region(43, 51, 48, 55);
            sort_in_region();
            if (units != 0)
            {
                find_all_alive_units(U_PEASANT);
                remove_all();
                *(byte*)(GB_HORSES + 5) = 1;
                set_res(P_RED, 2, 10, 0, 0, 0);
            }
            find_all_alive_units(U_HDESTROYER);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units != 0)
            {
                unit_create(25, 54, U_HDESTROYER, P_BLUE, 1);
                unit_create(8, 41, U_HDESTROYER, P_BLUE, 1);
                unit_create(4, 15, U_HDESTROYER, P_BLUE, 1);
                tip_status = 0;
                *(byte*)GB_HORSES = 17;
            }
        }
        else if (*(byte*)GB_HORSES == 17)
        {
            find_all_alive_units(U_HDESTROYER);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units == 0)
            {
                find_all_alive_units(U_BATTLESHIP);
                sort_stat(S_OWNER, P_RED, CMP_EQ);
                if (units == 0)
                {
                    unit_create(42, 58, U_BATTLESHIP, P_RED, 1);
                }
            }
            find_all_alive_units(U_HTRANSPORT);
            set_region(2, 3, 3, 4);
            sort_in_region();
            if (units != 0)
            {
                find_all_alive_units(U_RUNESTONE);
                kill_all();
                tip_status = 0;
                *(byte*)GB_HORSES = 18;
            }
        }
        else if (*(byte*)GB_HORSES == 18)
        {
            viz_area_add(24, 6, 1 << P_RED, 9);
            find_all_alive_units(U_DANATH);
            set_region(8, 8, 9, 9);
            sort_in_region();
            if (units != 0)
            {
                tip_status = 0;
                *(byte*)GB_HORSES = 19;
            }
        }
        else if (*(byte*)GB_HORSES == 19)
        {
            //*(byte*)LEVEL_OBJ = 254 + LVL_HUMAN1;
            //*(WORD*)LEVEL_ID = 0x52C6;
            *(byte*)LEVEL_OBJ = LVL_XHUMAN12;
            *(WORD*)LEVEL_ID = 0x53DC;
            win(true);
        }

        if (tip_status == 0)
        {
            if (*(byte*)GB_HORSES == 1)
            {
                tutorial_lines(40, 41, -1, -1);
            }
            else if (*(byte*)GB_HORSES == 2)
            {
                tutorial_lines(42, 43, 1, 2);
            }
            else if (*(byte*)GB_HORSES == 3)
            {
                tutorial_lines(3, 4, 5, 22);
            }
            else if (*(byte*)GB_HORSES == 4)
            {
                tutorial_lines(6, 7, -1, -1);
            }
            else if (*(byte*)GB_HORSES == 5)
            {
                tutorial_lines(8, 9, 10, -1);
            }
            else if (*(byte*)GB_HORSES == 6)
            {
                tutorial_lines(11, 12, 13, -1);
            }
            else if (*(byte*)GB_HORSES == 7)
            {
                tutorial_lines(14, 15, 16, 17);
            }
            else if (*(byte*)GB_HORSES == 9)
            {
                tutorial_lines(18, 19, 20, 21);
            }
            else if (*(byte*)GB_HORSES == 10)
            {
                tutorial_lines(23, 24, 25, 26);
            }
            else if (*(byte*)GB_HORSES == 11)
            {
                tutorial_lines(27, 28, 29, -1);
            }
            else if (*(byte*)GB_HORSES == 12)
            {
                tutorial_lines(27, 28, 29, -1);
            }
            else if (*(byte*)GB_HORSES == 13)
            {
                tutorial_lines(44, 45, 46, -1);
            }
            else if (*(byte*)GB_HORSES == 14)
            {
                tutorial_lines(30, 31, 32, -1);
            }
            else if (*(byte*)GB_HORSES == 15)
            {
                tutorial_lines(33, 34, 35, -1);
            }
            else if (*(byte*)GB_HORSES == 16)
            {
                tutorial_lines(47, 48, -1, -1);
            }
            else if (*(byte*)GB_HORSES == 17)
            {
                tutorial_lines(49, 50, 51, -1);
            }
            else if (*(byte*)GB_HORSES == 18)
            {
                tutorial_lines(36, 37, 38, 39);
            }
        }
    }
}

void v_orc2(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc3(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc4(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc5(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc6(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc7(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc8(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc9(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc10(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc11(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc12(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc13(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_orc14(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc1(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc2(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc3(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc4(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc5(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc6(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc7(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc8(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc9(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc10(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc11(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_xorc12(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}

void v_custom(bool rep_init)
{
    if (rep_init)
    {
        //pathfind_fix(true);
        //ai_fix_plugin(true);
        //your initialize
    }
    else
    {
        //your custom victory conditions
    }
}
//-------------------------------------------------------------------------------

void (*triggers[])(bool) = { v_human1,v_orc1,v_human2,v_orc2,v_human3,v_orc3,v_human4,v_orc4,v_human5,v_orc5,v_human6,v_orc6,v_human7,v_orc7,v_human8,v_orc8,v_human9,v_orc9,v_human10,v_orc10,v_human11,v_orc11,v_human12,v_orc12,v_human13,v_orc13,v_human14,v_orc14,v_xhuman1,v_xorc1,v_xhuman2,v_xorc2,v_xhuman3,v_xorc3,v_xhuman4,v_xorc4,v_xhuman5,v_xorc5,v_xhuman6,v_xorc6,v_xhuman7,v_xorc7,v_xhuman8,v_xorc8,v_xhuman9,v_xorc9,v_xhuman10,v_xorc10,v_xhuman11,v_xorc11,v_xhuman12,v_xorc12 };

byte remember_lvl = 255;
void trig()
{
    vizs_n = 0;//no reveal map areas every time

    byte lvl = *(byte*)LEVEL_OBJ;
    remember_lvl = lvl;
    if (a_custom)
    {
        v_custom(false);
    }
    else
    {
        if ((lvl >= 0) && (lvl < 52))
            ((void (*)(bool))triggers[lvl])(false);
        else
            v_custom(false);
    }
	first_step = false;
    
    //global triggers
    wharf();
    m_devotion[0] = U_DANATH;
    brclik(true);
    A_rally = true;
    A_tower = true;
    repair_cat(true);
    fireball_dmg(70);
    manacost(FIREBALL, 60);
    autoheal(true);
    trigger_time(40);
}

void trig_init()
{
	first_step = true;
    byte lvl = *(byte*)LEVEL_OBJ;
    if (a_custom)
    {
        v_custom(true);
    }
    else
    {
        if ((lvl >= 0) && (lvl < 52))
            ((void (*)(bool))triggers[lvl])(true);
        else
            v_custom(true);
    }
}

void replace_def()
{
    //set all vars to default
    memset(ua, 255, sizeof(ua));
    memset(ut, 255, sizeof(ut));
    memset(m_devotion, 255, sizeof(m_devotion));
	memset(vizs_areas, 0, sizeof(vizs_areas));
	vizs_n = 0;
    ai_fixed = false;
	saveload_fixed = false;
    A_portal = false;
    A_autoheal = false;
    A_rally = false;
    A_tower = false;
}

void replace_common()
{
    //peon can build any buildings
    char ballbuildings[] = "\x0\x0";//d1 05
    PATCH_SET((char*)BUILD_ALL_BUILDINGS1, ballbuildings);
    char ballbuildings2[] = "\x0";//0a
    PATCH_SET((char*)BUILD_ALL_BUILDINGS2, ballbuildings2);
    char ballbuildings3[] = "\x0";//68
    PATCH_SET((char*)BUILD_ALL_BUILDINGS3, ballbuildings3);

    //any building can train any unit
    char ballunits[] = "\xeb";//0x74
    PATCH_SET((char*)BUILD_ALL_UNITS1, ballunits);
    char ballunits2[] = "\xA1\xBC\x47\x49\x0\x90\x90";//8b 04 85 bc 47 49 00
    PATCH_SET((char*)BUILD_ALL_UNITS2, ballunits2);

    //any building can make any research
    char allres[] = "\xB1\x1\x90";
    PATCH_SET((char*)BUILD_ALL_RESEARCH1, allres);
    PATCH_SET((char*)BUILD_ALL_RESEARCH2, allres);

    //allow all units cast all spells
    char allsp[] = "\x90\x90";
    PATCH_SET((char*)CAST_ALL_SPELLS, allsp);

    //show kills
    byte d = S_KILLS;
    char sdmg[] = "\x8a\x90\x82\x0\x0\x0\x8b\xfa";//units
    sdmg[2] = d;
    PATCH_SET((char*)SPEED_STAT_UNITS, sdmg);
    char sdmg2[] = "\x8a\x82\x82\x0\x0\x0\x90\x90\x90";//catas
    sdmg2[2] = d;
    PATCH_SET((char*)SPEED_STAT_CATS, sdmg2);
    char sdmg3[] = "\x8a\x88\x82\x0\x0\x0\x90\x90\x90";//archers
    sdmg3[2] = d;
    PATCH_SET((char*)SPEED_STAT_ARCHERS, sdmg3);
    char sdmg4[] = "\x8a\x82\x82\x0\x0\x0\x90\x90\x90";//berserkers
    sdmg4[2] = d;
    PATCH_SET((char*)SPEED_STAT_BERSERKERS, sdmg4);
    char sdmg5[] = "\x8a\x88\x82\x0\x0\x0\x90\x90\x90";//ships
    sdmg5[2] = d;
    PATCH_SET((char*)SPEED_STAT_SHIPS, sdmg5);

    char dmg_fix[] = "\xeb";
    PATCH_SET((char*)DMG_FIX, dmg_fix);

    draw_stats_fix(true);
}

void replace_back()
{
    //replace all to default
    comps_vision(false);
    repair_cat(false);
    trigger_time('\xc8');
    upgr(SWORDS, 2);
    upgr(ARMOR, 2);
    upgr(ARROWS, 1);
    upgr(SHIP_DMG, 5);
    upgr(SHIP_ARMOR, 5);
    upgr(CATA_DMG, 15);
    manacost(VISION, 70);
    manacost(HEAL, 6);
    manacost(GREATER_HEAL, 5);
    manacost(EXORCISM, 4);
    manacost(FIREBALL, 100);
    manacost(FLAME_SHIELD, 80);
    manacost(SLOW, 50);
    manacost(INVIS, 200);
    manacost(POLYMORPH, 200);
    manacost(BLIZZARD, 25);
    manacost(EYE_OF_KILROG, 70);
    manacost(BLOOD, 50);
    manacost(RAISE_DEAD, 50);
    manacost(COIL, 100);
    manacost(WHIRLWIND, 100);
    manacost(HASTE, 50);
    manacost(UNHOLY_ARMOR, 100);
    manacost(RUNES, 200);
    manacost(DEATH_AND_DECAY, 25);
    draw_stats_fix(false);
	ai_fix_plugin(false);
    autoheal(false);
}

void replace_trigger()
{
    replace_back();
    replace_def();
    replace_common();

    buttons_init_all();

    //replace original victory trigger
    char trig_jmp[] = "\x74\x1A";//74 0F
    PATCH_SET((char*)VICTORY_JMP, trig_jmp);
    char rep[] = "\xc7\x05\x38\x0d\x4c\x0\x30\x8C\x45\x0";
    void (*repf) () = trig;
    patch_setdword((DWORD*)(rep + 6), (DWORD)repf);
    PATCH_SET((char*)VICTORY_TRIGGER, rep);
	trig_init();
}

byte tileset_fog_color = 239;

void tilesets_change_fog(byte color)
{
    if (color == 0)color = 239;

    //border between fog and visible tiles
    int adr = (int)(*(int*)TILESET_POINTER_VR4);
    if (adr)for (int i = 0; i < 0x4000; i++)if (*(byte*)(adr + i) != 0)*(byte*)(adr + i) = color;

    //border between fog of war and full fog
    char buf[] = "\x0";
    buf[0] = color;
    for (int i = 0; i < 16; i++)PATCH_SET((char*)(0x004019E1 + 4 * i), buf);
    for (int i = 0; i < 16; i++)PATCH_SET((char*)(0x00401AC9 + 4 * i), buf);

    //fog of war and full fog and minimap
    tileset_fog_color = color;
}

PROC g_proc_00421E6F;
void tileset_draw_full_fog(int adr)
{
    ((void (*)(int))g_proc_00421E6F)(adr);//original
    int k = 0;
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            *(byte*)(adr + k + j) = tileset_fog_color;
        }
        k += m_screen_w;
    }
}

PROC g_proc_00421E81;
void tileset_draw_fog_of_war(int adr)
{
    ((void (*)(int))g_proc_00421E81)(adr);//original
    int k = 0;
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j += 2)
        {
            *(byte*)(adr + k + j + ((i % 2 != 0) ? 1 : 0)) = tileset_fog_color;
        }
        k += m_screen_w;
    }
}

PROC g_proc_00421242;
void tileset_draw_fog_minimap()
{
    *(byte*)0x004D6532 = tileset_fog_color;
    ((void (*)())g_proc_00421242)();//original
    *(byte*)0x004D6532 = 0;
}

void change_controler_stat()
{
    if (remember_lvl == LVL_ORC1)
    {
        *(byte*)(CONTROLER_TYPE + P_BLACK) = C_NOBODY;
        *(byte*)(STARTING_CONTROLER_TYPE + P_BLACK) = C_NOBODY;
    }
    else if ((remember_lvl == LVL_HUMAN1) || (remember_lvl == LVL_XHUMAN1))
    {
        *(byte*)(CONTROLER_TYPE + P_ORANGE) = C_NOBODY;
        *(byte*)(STARTING_CONTROLER_TYPE + P_ORANGE) = C_NOBODY;
    }
    else if ((remember_lvl == LVL_HUMAN10) || (remember_lvl == LVL_HUMAN13) || (remember_lvl == LVL_HUMAN14))
    {
        *(byte*)(CONTROLER_TYPE + P_VIOLET) = C_NOBODY;
        *(byte*)(STARTING_CONTROLER_TYPE + P_VIOLET) = C_NOBODY;
    }
}

PROC g_proc_0042049F;
void game_end_dialog(int a)
{
    change_controler_stat();
    ((void (*)(int))g_proc_0042049F)(a);//original
}

void set_rain(WORD amount, byte density, byte speed, byte size, byte sz_x, byte sz_y, bool snow, WORD thunder)
{
    can_rain = true;
    raindrops_amount = my_min(RAINDROPS, amount);
    raindrops_density = density;
    raindrops_speed = speed;
    raindrops_size = size;
    raindrops_size_x = my_max(raindrops_size, sz_x);
    raindrops_size_y = my_max(raindrops_size, sz_y);
    raindrops_align_x = raindrops_size - raindrops_size_x;
    raindrops_align_y = raindrops_size - raindrops_size_y;
    raindrops_snow = snow;
    raindrops_thunder = thunder;
    raindrops_thunder_timer = thunder;
    raindrops_thunder_gradient = THUNDER_GRADIENT;
}

PROC g_proc_0042A4A1;
int new_game(int a, int b, long c)
{
    if (remember_music != 101)
        *(DWORD*)VOLUME_MUSIC = remember_music;
    if (remember_sound != 101)
        *(DWORD*)VOLUME_SOUND = remember_sound;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM);//set volume
    remember_music = 101;
    remember_sound = 101;

    a_custom = b % 256;//custom game or campaign
	if (a_custom)*(byte*)LEVEL_OBJ=53;//remember custom obj
	else
	{
	if (*(byte*)LEVEL_OBJ==53)a_custom=1;//fix for when saved game loads custom get broken
	}
    replace_trigger();
    memset((void*)GB_HORSES, 0, 16 * sizeof(byte));
    int original = ((int (*)(int, int, long))g_proc_0042A4A1)(a, b, c);//original
    game_started = true;
    if ((*(byte*)LEVEL_OBJ == LVL_HUMAN1) || (*(byte*)LEVEL_OBJ == LVL_XHUMAN1)
        || (*(byte*)LEVEL_OBJ == LVL_HUMAN10) || (*(byte*)LEVEL_OBJ == LVL_ORC1))tilesets_change_fog(239);
    else tilesets_change_fog(210);
    tip_line1 = NULL;
    tip_line2 = NULL;
    tip_line3 = NULL;
    tip_line4 = NULL;
    tip_status = 0;
    tip_w = 0;
    tip_h = 0;

    for (int i = 0; i < RAINDROPS; i++)
    {
        raindrops[i].l = 0;
    }
    can_rain = false;
    byte lvl = *(byte*)LEVEL_OBJ;
    if (lvl == LVL_HUMAN10)set_rain(200, 4, 2, 8, 5, 6, true, 0);

    return original;
}

PROC g_proc_0041F7E4;
int load_game(int a)
{
    int original = ((int (*)(int))g_proc_0041F7E4)(a);//original
    replace_trigger();

    return original;
}

byte* ScreenPTR = NULL;
byte* getScreenPtr() {
    DWORD* r;
    r = (DWORD*)SCREEN_POINTER;
    if (r) { return (byte*)*r; }
    return NULL;
}

void draw_pixel(int x, int y, byte c, bool inval)
{
    if (ScreenPTR)
    {
        *(byte*)(ScreenPTR + x + m_screen_w * y) = c;
        if (inval)
            ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x, y);
    }
}

void draw_pixel_safe(int x, int y, byte c, bool inval)
{
    if (ScreenPTR)
    {
        if ((x >= m_minx) && (y >= m_miny) && (x <= m_maxx) && (y <= m_maxy))
        {
            *(byte*)(ScreenPTR + x + m_screen_w * y) = c;
            if (inval)
                ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x, y);
        }
    }
}

void draw_line(int x1, int y1, int x2, int y2, byte c, bool inval)
{
    int x, y, xdelta, ydelta, xdiff, ydiff, accum, sign;

    xdelta = x2 - x1;
    ydelta = y2 - y1;
    accum = 0;
    sign = 0;

    if (!xdelta && ydelta)
    { /* Special case: straight vertical line */
        x = x1;
        for (y = y1; y < (y1 + ydelta); ++y)
        {
            draw_pixel(x, y, c, true);
        }
    }
    else if (xdelta && !ydelta)
    { /* Special case: straight horisontal line */
        y = y1;
        for (x = x1; x <= x1 + xdelta; ++x)
        {
            draw_pixel(x, y, c, true);
        }
    }
    else
    {
        xdiff = (xdelta << 16) / ydelta;
        ydiff = (ydelta << 16) / xdelta;

        if (abs(xdiff) > abs(ydiff)) { /* horizontal-major */
            y = y1;
            if (xdelta < 0) { /* traversing negative x */
                for (x = x1; x >= x2; --x) {
                    draw_pixel(x, y, c, true);
                    accum += abs(ydiff);
                    while (accum >= (1 << 16)) {
                        ++y;
                        accum -= (1 << 16);
                    }
                }
            }
            else { /* traversing positive x */
                for (x = x1; x <= x2; ++x) {
                    draw_pixel(x, y, c, true);
                    accum += abs(ydiff);
                    while (accum >= (1 << 16)) {
                        ++y;
                        accum -= (1 << 16);
                    }
                }
            }
        }
        else if (abs(ydiff) > abs(xdiff)) { /* vertical major */
            sign = (xdelta > 0 ? 1 : -1);
            x = x1;
            for (y = y1; y <= y2; ++y) {
                draw_pixel(x, y, c, true);
                accum += abs(xdiff);
                while (accum >= (1 << 16)) {
                    x += sign;
                    accum -= (1 << 16);
                }
            }
        }
        else if (abs(ydiff) == abs(xdiff)) { /* 45 degrees */
            sign = (xdelta > 0 ? 1 : -1);
            x = x1;
            for (y = y1; y <= y2; ++y) {
                draw_pixel(x, y, c, true);
                x += sign;
            }
        }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x1, y1, x2, y2);
}

void draw_rect(int x1, int y1, int x2, int y2, byte c, bool o, bool inval)
{
    if (!ScreenPTR)return;
    int xmin = my_min(x1, x2);
    int xmax = my_max(x1, x2);
    int ymin = my_min(y1, y2);
    int ymax = my_max(y1, y2);
    byte* p = ScreenPTR + ymin * m_screen_w + xmin;
    if (o)
    {
        for (int i = xmax - xmin + 1; i > 0; i--)
        {
            *p++ = c;
        }
        p = ScreenPTR + ymin * m_screen_w + xmin;
        for (int i = ymax - ymin + 1; i > 0; i--)
        {
            *p = c;
            p += m_screen_w;
        }
        p = ScreenPTR + ymax * m_screen_w + xmin;
        for (int i = xmax - xmin + 1; i > 0; i--)
        {
            *p++ = c;
        }
        p = ScreenPTR + ymin * m_screen_w + xmax;
        for (int i = ymax - ymin + 1; i > 0; i--)
        {
            *p = c;
            p += m_screen_w;
        }
    }
    else
    {
        for (int j = ymax - ymin + 1; j > 0; j--)
        {
            for (int i = xmax - xmin + 1; i > 0; i--)
                *p++ = c;
            p += m_screen_w - xmax + xmin - 1;
        }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x1, y1, x2, y2);
}

int draw_char(int x, int y, byte c, unsigned char ch, byte bc, bool inval)
{
    if (!ScreenPTR)return 0;
    byte* p = ScreenPTR + y * m_screen_w + x;
    if (ch < ' ')ch = ' ';
    ch -= ' ';
    int chMfontwidthplus1 = ch * (1 + FONT_6PX_PROP_CHAR_WIDTH);
    int w = 1 + font_6px_prop[chMfontwidthplus1];
    if (!((c == 0) && (bc == 0)))
    {
        if (bc)
            for (int i = 1; i <= w; i++)
            {
                byte font_line = font_6px_prop[chMfontwidthplus1 + i];
                for (int j = 0; j < 8; j++)
                {
                    if (font_line & (1 << j))*p = c;
                    else *p = bc;
                    p += m_screen_w;
                    //                p++;
                }
                p -= m_screen_w * 8;
                p++;
                //            p+=m_screen_w-8;
            }
        else
            for (int i = 0; i < w; i++)
            {
                byte font_line = font_6px_prop[ch * (1 + FONT_6PX_PROP_CHAR_WIDTH) + 1 + i];
                for (int j = 0; j < 8; j++)
                {
                    if (font_line & (1 << j))*p = c;
                    p += m_screen_w;
                    //                p++;
                }
                p -= m_screen_w * 8;
                p++;
                //            p+=m_screen_w-8;
            }
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x + w, y + 8);
    return w;
}

int draw_text(int x, int y, byte c, unsigned char* ch, byte bc, byte cond, bool inval)
{
    int w = 0;
    int C = 0;
    while (ch[C] != 0)
    {
        w += draw_char(x + w, y, c, ch[C], bc, false);
        w -= cond;
        if (w < 0)w = 0;
        C++;
    }
    if (inval)
        ((void (*)(int, int, int, int))F_INVALIDATE)(x, y, x + w, y + 8);
    return w;
}

int draw_char_get_width(unsigned char ch)
{
    if (ch < ' ')ch = ' ';
    return 1 + font_6px_prop[(ch - ' ') * (1 + FONT_6PX_PROP_CHAR_WIDTH)];
}

int draw_text_get_width(unsigned char* ch, int s, byte cond)
{
    int w = 0;
    int C = 0;
    while (ch[C] != 0)
    {
        w += draw_char_get_width(ch[C]);
        w -= cond;
        if (w < 0)w = 0;
        C++;
    }
    return w * s;
}

void draw_tips()
{
    int k = 0;
    int w = 0;
    if (tip_line1 != NULL)
    {
        k++;
        int ww = draw_text_get_width(tip_line1, 1, 0);
        if (ww > w)w = ww;
    }
    if (tip_line2 != NULL)
    {
        k++;
        int ww = draw_text_get_width(tip_line2, 1, 0);
        if (ww > w)w = ww;
    }
    if (tip_line3 != NULL)
    {
        k++;
        int ww = draw_text_get_width(tip_line3, 1, 0);
        if (ww > w)w = ww;
    }
    if (tip_line4 != NULL)
    {
        k++;
        int ww = draw_text_get_width(tip_line4, 1, 0);
        if (ww > w)w = ww;
    }
    if (k > 0)
    {
        w += 3 + 2;//3 step + 2 border
        int h = 9 * k + 2 + 2;
        if (tip_status != 0)
        {
            if ((tip_w >= 4) && (tip_h >= 4))
            {
                draw_rect(m_minx + 2, m_miny + 2, m_minx + 2 + tip_w, m_miny + 2 + tip_h, 198, true, false);//border1
                draw_rect(m_minx + 2 + 1, m_miny + 2 + 1, m_minx + 2 - 1 + tip_w, m_miny + 2 - 1 + tip_h, 200, true, false);//border2
                draw_rect(m_minx + 2 + 2, m_miny + 2 + 2, m_minx + 2 - 2 + tip_w, m_miny + 2 - 2 + tip_h, 239, false, false);//rect
            }
        }
        if (tip_status == 1)
        {
            if (tip_w < w)tip_w += tip_wspeed;
            else tip_w = w;
            if (tip_h < h)tip_h += tip_hspeed;
            else tip_h = h;
            if ((tip_w == w) && (tip_h == h))tip_status = 2;
        }
        if (tip_status == 2)
        {
            if (tip_line1 != NULL)draw_text(m_minx + 2 + 3, m_miny + 2 + 3 + 9 * 0, 251, tip_line1, 239, 0, false);
            if (tip_line2 != NULL)draw_text(m_minx + 2 + 3, m_miny + 2 + 3 + 9 * 1, 251, tip_line2, 239, 0, false);
            if (tip_line3 != NULL)draw_text(m_minx + 2 + 3, m_miny + 2 + 3 + 9 * 2, 251, tip_line3, 239, 0, false);
            if (tip_line4 != NULL)draw_text(m_minx + 2 + 3, m_miny + 2 + 3 + 9 * 3, 251, tip_line4, 239, 0, false);
        }
        if (tip_status == 3)
        {
            if (tip_w > 0)tip_w -= tip_wspeed;
            else tip_w = 0;
            if (tip_h > 0)tip_h -= tip_hspeed;
            else tip_h = 0;
            if ((tip_w == 0) && (tip_h == 0))tip_status = 1;
        }
    }
}

void draw_rain()
{
    //set_rain_color(raindrops_color_r, raindrops_color_g, raindrops_color_b);
    for (int i = 0; i < raindrops_amount; i++)
    {
        if (raindrops[i].l != 0)
        {
            if ((raindrops[i].x1 >= m_minx) && (raindrops[i].y1 >= m_miny) && (raindrops[i].x2 <= m_maxx) && (raindrops[i].y2 <= m_maxy))
            {
                if (raindrops_snow)
                {
                    WORD cx = raindrops[i].x1 + (abs(raindrops[i].x2 - raindrops[i].x1) / 2);
                    WORD cy = raindrops[i].y1 + (abs(raindrops[i].y2 - raindrops[i].y1) / 2);
                    draw_pixel_safe(cx - 1, cy, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 1, cy, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx, cy - 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx, cy + 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx - 2, cy - 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx - 2, cy + 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 2, cy - 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 2, cy + 1, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx - 1, cy - 2, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 1, cy - 2, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx - 1, cy + 2, RAINDROPS_COLOR, false);
                    draw_pixel_safe(cx + 1, cy + 2, RAINDROPS_COLOR, false);
                }
                else draw_line(raindrops[i].x1, raindrops[i].y1, raindrops[i].x2, raindrops[i].y2, RAINDROPS_COLOR, false);
            }
        }
    }
    ((void (*)(int, int, int, int))F_INVALIDATE)(m_minx, m_miny, m_maxx, m_maxy);
}

#define GAME_MODE 0x004AE430
void drawing()
{
    ScreenPTR = getScreenPtr();
    if (*(byte*)GAME_MODE == 3)
    {
        if (can_rain)draw_rain();
        if (game_started)if ((*(byte*)LEVEL_OBJ == LVL_ORC1) || (*(byte*)LEVEL_OBJ == LVL_HUMAN10))draw_tips();
    }
    ((void (*)(int, int, int, int))F_INVALIDATE)(0, 0, m_screen_w, m_screen_h);
}

PROC g_proc_00421F57;
void draw_hook3()
{
    ((void (*)())g_proc_00421F57)();//original
    drawing();
}

void hook(int adr, PROC* p, char* func)
{
    *p = patch_call((char*)adr, func);
}

void common_hooks()
{
    hook(0x0045271B, &g_proc_0045271B, (char*)update_spells);
    hook(0x004522B9, &g_proc_004522B9, (char*)seq_run);

    hook(0x0041038E, &g_proc_0041038E, (char*)damage1);
    hook(0x00409F3B, &g_proc_00409F3B, (char*)damage2);
    hook(0x0040AF70, &g_proc_0040AF70, (char*)damage3);
    hook(0x0040AF99, &g_proc_0040AF99, (char*)damage4);
    hook(0x00410762, &g_proc_00410762, (char*)damage5);
    hook(0x004428AD, &g_proc_004428AD, (char*)damage6);

    hook(0x0043BAE1, &g_proc_0043BAE1, (char*)rc_snd);
    hook(0x0043B943, &g_proc_0043B943, (char*)rc_build_click);
    hook(0x0040DF71, &g_proc_0040DF71, (char*)bld_unit_create);
    hook(0x0040AFBF, &g_proc_0040AFBF, (char*)tower_find_attacker);
    hook(0x00451728, &g_proc_00451728, (char*)unit_kill_deselect);

    hook(0x00452939, &g_proc_00452939, (char*)grow_struct_count_tables);
    hook(0x0042479E, &g_proc_0042479E, (char*)peon_into_goldmine);
    hook(0x00424745, &g_proc_00424745, (char*)goods_into_inventory);
    hook(0x004529C0, &g_proc_004529C0, (char*)goods_into_inventory);
	hook(0x00451054, &g_proc_00451054, (char*)count_add_to_tables_load_game);
    hook(0x00438A5C, &g_proc_00438A5C, (char*)unset_peon_ai_flags);
    hook(0x00438985, &g_proc_00438985, (char*)unset_peon_ai_flags);
	
	hook(0x0040EEDD, &g_proc_0040EEDD, (char*)upgrade_tower);
    hook(0x00442E25, &g_proc_00442E25, (char*)create_skeleton);
    hook(0x00425D1C, &g_proc_00425D1C, (char*)cast_raise);
    hook(0x0042757E, &g_proc_0042757E, (char*)ai_spell);
    hook(0x00427FAE, &g_proc_00427FAE, (char*)ai_attack);
    hook(0x00427FFF, &g_proc_00427FFF, (char*)ai_attack_nearest_enemy);
	
    hook(0x0042049F, &g_proc_0042049F, (char*)game_end_dialog);
	hook(0x0042A4A1, &g_proc_0042A4A1, (char*)new_game);
	hook(0x0041F7E4, &g_proc_0041F7E4, (char*)load_game);

    hook(0x00421E6F, &g_proc_00421E6F, (char*)tileset_draw_full_fog);
    hook(0x00421E81, &g_proc_00421E81, (char*)tileset_draw_fog_of_war);
    hook(0x00421242, &g_proc_00421242, (char*)tileset_draw_fog_minimap);

    hook(0x0044ACA3, &g_proc_0044ACA3, (char*)button_description);
    hook(0x0044B9F2, &g_proc_0044B9F2, (char*)button_description);
    hook(0x00420F02, &g_proc_00420F02, (char*)status_port_draw);

    build_finish_spell_orig = *(DWORD*)BLDG_FINISH_SPELL;
    patch_setdword((DWORD*)BLDG_FINISH_SPELL, (DWORD)build_finish_spell);
    build_cost_spell_orig = *(DWORD*)BLDG_COST_SPELL;
    patch_setdword((DWORD*)BLDG_COST_SPELL, (DWORD)build_cost_spell);

    hook(0x00455F9C, &g_proc_00455F9C, (char*)build_start_build);
}

void files_hooks()
{
    files_init();

    hook(0x004542FB, &g_proc_004542FB, (char*)grp_draw_cross);
    hook(0x00454DB4, &g_proc_00454DB4, (char*)grp_draw_bullet);
    hook(0x00454BCA, &g_proc_00454BCA, (char*)grp_draw_unit);
    hook(0x00455599, &g_proc_00455599, (char*)grp_draw_building);
    hook(0x0043AE54, &g_proc_0043AE54, (char*)grp_draw_building_placebox);
    hook(0x0044538D, &g_proc_0044538D, (char*)grp_portrait_init);
    hook(0x004453A7, &g_proc_004453A7, (char*)grp_draw_portrait);
    hook(0x004452B0, &g_proc_004452B0, (char*)grp_draw_portrait_buttons);
    hook(0x0044A65C, &g_proc_0044A65C, (char*)status_get_tbl);
    hook(0x0044AC83, &g_proc_0044AC83, (char*)unit_hover_get_id);
    hook(0x0044AE27, &g_proc_0044AE27, (char*)unit_hover_get_tbl);
    hook(0x0044AE56, &g_proc_0044AE56, (char*)button_description_get_tbl);
    hook(0x0044B23D, &g_proc_0044B23D, (char*)button_hotkey_get_tbl);
    hook(0x004354C8, &g_proc_004354C8, (char*)objct_get_tbl_custom);
    hook(0x004354FA, &g_proc_004354FA, (char*)objct_get_tbl_campanign);
    hook(0x004300A5, &g_proc_004300A5, (char*)objct_get_tbl_briefing_task);
    hook(0x004300CA, &g_proc_004300CA, (char*)objct_get_tbl_briefing_title);
    hook(0x004301CA, &g_proc_004301CA, (char*)objct_get_tbl_briefing_text);
    hook(0x00430113, &g_proc_00430113, (char*)objct_get_briefing_speech);
    hook(0x0041F0F5, &g_proc_0041F0F5, (char*)finale_get_tbl);
    hook(0x0041F1E8, &g_proc_0041F1E8, (char*)finale_credits_get_tbl);
    hook(0x0041F027, &g_proc_0041F027, (char*)finale_get_speech);
    hook(0x00417E33, &g_proc_00417E33, (char*)credits_small_get_tbl);
    hook(0x00417E4A, &g_proc_00417E4A, (char*)credits_big_get_tbl);
    hook(0x0042968A, &g_proc_0042968A, (char*)act_get_tbl_small);
    hook(0x004296A9, &g_proc_004296A9, (char*)act_get_tbl_big);
    hook(0x0041C51C, &g_proc_0041C51C, (char*)netstat_get_tbl_nation);
    hook(0x00431229, &g_proc_00431229, (char*)rank_get_tbl);
    hook(0x004372EE, &g_proc_004372EE, (char*)pcx_load_menu);
    hook(0x00430058, &g_proc_00430058, (char*)pcx_load_briefing);
    hook(0x00429625, &g_proc_00429625, (char*)pcx_load_act);
    hook(0x00429654, &g_proc_00429654, (char*)pcx_load_act);
    hook(0x0041F004, &g_proc_0041F004, (char*)pcx_load_final);
    hook(0x00417DDB, &g_proc_00417DDB, (char*)pcx_load_credits);
    hook(0x0043169E, &g_proc_0043169E, (char*)pcx_load_statistic);
    hook(0x00462D4D, &g_proc_00462D4D, (char*)storm_file_load);
    hook(0x0041F9FD, &g_proc_0041F9FD, (char*)tilesets_load);
    
    hook(0x0041F97D, &g_proc_0041F97D, (char*)map_file_load);

    hook(0x0042A443, &g_proc_0042A443, (char*)act_init);

    hook(0x0044F37D, &g_proc_0044F37D, (char*)main_menu_init);

    hook(0x0043B16F, &g_proc_0043B16F, (char*)smk_play_sprintf_name);
    hook(0x0043B362, &g_proc_0043B362, (char*)smk_play_sprintf_blizzard);

    hook(0x00440F4A, &g_proc_00440F4A, (char*)music_play_get_install);
    hook(0x00440F5F, &g_proc_00440F5F, (char*)music_play_get_install);
    patch_call((char*)0x00440F41, (char*)music_play_sprintf_name);
    char buf[] = "\x90";
    PATCH_SET((char*)(0x00440F41 + 5), buf);//7 bytes call

    hook(0x00424A9C, &g_proc_00424A9C, (char*)storm_font_load);
    hook(0x00424AB2, &g_proc_00424AB2, (char*)storm_font_load);
    hook(0x004288B2, &g_proc_004288B2, (char*)storm_font_load);
    hook(0x00428896, &g_proc_00428896, (char*)storm_font_load);
    hook(0x0042887D, &g_proc_0042887D, (char*)storm_font_load);

    hook(0x00422D76, &g_proc_00422D76, (char*)sound_play_unit_speech);
    hook(0x00422D5F, &g_proc_00422D5F, (char*)sound_play_unit_speech_soft);
}

void capture_fix()
{
    char buf[] = "\xB0\x01\xF6\xC1\x02\x74\x02\xB0\x02\x50\x66\x8B\x7B\x18\x66\x8B\x6B\x1A\x8B\xD7\x8B\xF5\x29\xC2\x29\xC6\x8D\x43\x27\x31\xC9\x89\x44\x24\x24\x8A\x08\xC1\xE1\x02\x66\x8B\x81\x1C\xEE\x4C\x00\x66\x8B\x89\x1E\xEE\x4C\x00\x66\x01\xF8\x66\x01\xE9\x5D\x01\xE8\x01\xE9\x90\x90";
    PATCH_SET((char*)CAPTURE_BUG, buf);
}

void fireshield_cast_fix()
{
    char buf[] = "\x90\x90";
    PATCH_SET((char*)FIRE_CAST1, buf);
    char buf2[] = "\x90\x90\x90\x90\x90\x90";
    PATCH_SET((char*)FIRE_CAST2, buf2);
}

char save_folder[] = "Save\\RedMist\\";

void change_save_folder()
{
    patch_setdword((DWORD*)0x0043C07A, (DWORD)save_folder);
    patch_setdword((DWORD*)0x0043CE66, (DWORD)save_folder);
    patch_setdword((DWORD*)0x0043D2C1, (DWORD)save_folder);
    patch_setdword((DWORD*)0x0043D383, (DWORD)save_folder);
    patch_setdword((DWORD*)0x0043FB66, (DWORD)save_folder);
}

bool dll_called = false;

extern "C" __declspec(dllexport) void w2p_init()
{
    DWORD check_exe = *(DWORD*)0x48F2F0;
    if (check_exe == 0x4d444552)//REDM
    {
        //if (!dll_called)
        {
            m_screen_w = *(WORD*)SCREEN_SIZE_W;
            m_screen_h = *(WORD*)SCREEN_SIZE_H;
            m_added_width = m_screen_w - 640;
            m_added_height = m_screen_h - 480;
            m_align_x = m_added_width > 0 ? m_added_width / 2 : 0;
            m_align_y = m_added_height > 0 ? m_added_height / 2 : 0;
            m_maxx = m_minx + m_screen_w - 200;
            m_maxy = m_miny + m_screen_h - 40;

            hook(0x00421F57, &g_proc_00421F57, (char*)draw_hook3);

            common_hooks();
            files_hooks();

            hook(0x0041F915, &g_proc_0041F915, (char*)map_load);
            hook(0x0042BB04, &g_proc_0042BB04, (char*)map_create_unit);
            hook(0x00418937, &g_proc_00418937, (char*)dispatch_die_unitdraw_update_1_man);

            hook(0x0042A466, &g_proc_0042A466, (char*)briefing_check);
            hook(0x00416930, &g_proc_00416930, (char*)player_race_mission_cheat);
            hook(0x0042AC6D, &g_proc_0042AC6D, (char*)player_race_mission_cheat2);
            
            sounds_tables();

            //char buf[] = "\x90\x90\x90";
            //PATCH_SET((char*)0x0045158A, buf);//peon die with res sprite bug fix
            //hook(0x00451590, &g_proc_00451590, (char*)unit_kill_peon_change);

            //char buf_showpath[] = "\xB0\x1\x90\x90\x90";
            //PATCH_SET((char*)0x00416691, buf_showpath);
            hook(0x0045614E, &g_proc_0045614E, (char*)receive_cheat_single);

            *(byte*)(0x0049D93C) = 0;//remove text
            *(byte*)(0x0049DC24) = 0;//from main menu
            patch_call((char*)0x004886B3, (char*)0x0048CCA2);//remove fois HD text from main menu

            char buf_i[] = "\xE8\x1B\x31\x5\x0";
            PATCH_SET((char*)0x00428E08, buf_i);//fix fois install

            change_save_folder();
            capture_fix();
            fireshield_cast_fix();

            char buf_unload_check[] = "\x0\x0\x0\x0";
            PATCH_SET((char*)0x0048F2F0, buf_unload_check);//dll unload

            dll_called = true;
        }
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID)
{
    return TRUE;
}