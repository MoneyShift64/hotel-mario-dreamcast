#include "frontend.h"
static int valid(int h) { return h >= 1 && h <= 8 && h != 6; }
int frontend_next_hotel(int hotel, int direction, int progress) {
    if (!valid(progress))
        progress = 1;
    if (!valid(hotel) || hotel > progress)
        hotel = 1;
    for (int i = 0; i < 8; i++) {
        hotel += direction < 0 ? -1 : 1;
        if (hotel < 1)
            hotel = progress;
        if (hotel > progress)
            hotel = 1;
        if (valid(hotel))
            return hotel;
    }
    return 1;
}
void frontend_choose_hotel(player_t *p, int hotel) {
    if (!valid(hotel) || hotel > p->progress)
        return;
    p->hotel = hotel;
    p->stage = hotel == p->progress ? p->unk_18 : 0;
    if (p->stage < 0)
        p->stage = 0;
    if (p->stage > (hotel == 8 ? 14 : 9))
        p->stage = hotel == 8 ? 14 : 9;
    if (!(p->flags & 4))
        p->unk_10 = 0;
    p->got_bonus = 1;
}
void frontend_record_progress(player_t best[2], const player_t current[2]) {
    for (int i = 0; i < 2; i++) {
        int progress = current[i].progress, stage = current[i].unk_18;
        if (!valid(progress))
            continue;
        int last = progress == 8 ? 14 : 9;
        if (stage < 0)
            stage = 0;
        if (stage > last)
            stage = last;
        if (progress > best[i].progress ||
            (progress == best[i].progress && stage > best[i].unk_18)) {
            best[i].progress = progress;
            best[i].unk_18 = stage;
        }
    }
}

/* gh_save(0), YES branch: five lives and a fresh score/coin count. */
void frontend_continue_player(player_t *player) {
    player->lives = 5;
    player->score = 0;
    player->coins = 0;
    player->power = 0;
}

/* loading_screen(1): original enemy/arrival text and byte offsets in screen_a0. */
const char *frontend_arrival(int hotel,int *x) {
    switch(hotel) {
    case 1:*x=62;return "MORTON AND HIS NUTTY GROUP";
    case 2:*x=91;return "ROY AND HIS HENCHMEN";
    case 3:*x=42;return "LARRY AND HIS UNDERGROUND PALS";
    case 4:*x=50;return "LEMMY AND HIS FLYING FRIENDS";
    case 5:*x=36;return "LUDWIG AND HIS SPOOKY SIDEKICKS";
    case 7:*x=76;return "WENDY AND HER BRAT PACK";
    case 8:*x=101;return "BOWSER AND COMPANY";
    default:*x=0;return "";
    }
}
/* loading_screen(2): original calendar messages; g/m encode !/? in this font. */
const char *frontend_ready(int month,int day,int *x) {
    if((month==1&&day<6)||(month==12&&day>26)){*x=117;return "HAPPY NEW YEARg";}
    if(month==2&&day==17){*x=94;return "ITS HOLLIES BIRTHDAYg";}
    if(month==2&&day>=12&&day<=14){*x=90;return "HAPPY VALENTINES DAYg";}
    if(month==5&&day>=7&&day<=9){*x=62;return "DO NOT FORGET MOTHERS DAYg";}
    if(month==5&&day>=10&&day<30){*x=97;return "SCHOOLS ALMOST OUTg";}
    if(month==6&&day>=17&&day<=20){*x=102;return "HAPPY FATHERS DAYg";}
    if(month==7&&day<5){*x=87;return "HAPPY FOURTH OF JULYg";}
    if(month==8&&day>=15){*x=57;return "READY TO GO BACK TO SCHOOLm";}
    if(month==10&&day>=29){*x=127;return "BOOOOOOOOOggg";}
    if(month==11&&day>=17&&day<=25){*x=37;return "HAPPY TURKEY DAYg GOBBLE GOBBLEg";}
    if(month==12&&day>10&&day<=26){*x=122;return "HAPPY HOLIDAYSg";}
    *x=139;return "HERE WE GOg";
}
