#pragma bank 255
#include <string.h>
#include "td_guidance.h"
#include "td_game.h"
#include "actor.h"
#include "ui.h"
/* Original eight-pixel arrows and accents occupy unused tiles after the font.
 * The paused atlas owns only16..187, so normal guidance survives map visits. */
static const UBYTE td_arrows[]={
    0x18,0x18,0x3c,0x3c,0x7e,0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0,0,0,0,
    0,0,0x08,0x08,0x0c,0x0c,0x7e,0x7e,0x0c,0x0c,0x08,0x08,0,0,0,0,
    0x18,0x18,0x18,0x18,0x18,0x18,0x7e,0x7e,0x3c,0x3c,0x18,0x18,0,0,0,0,
    0,0,0x10,0x10,0x30,0x30,0x7e,0x7e,0x30,0x30,0x10,0x10,0,0,0,0,
    0,0,0x3e,0x3e,0x06,0x06,0x0a,0x0a,0x12,0x12,0x20,0x20,0,0,0,0,
    0,0,0x20,0x20,0x12,0x12,0x0a,0x0a,0x06,0x06,0x3e,0x3e,0,0,0,0,
    0,0,0x04,0x04,0x48,0x48,0x50,0x50,0x60,0x60,0x7c,0x7c,0,0,0,0,
    0,0,0x7c,0x7c,0x60,0x60,0x50,0x50,0x48,0x48,0x04,0x04,0,0,0,0,
    0,0x10,0,0x38,0,0xfe,0,0x7c,0,0x38,0,0x6c,0,0x44,0,0,
    0x10,0,0x28,0,0xc6,0,0x44,0,0x28,0,0x54,0,0x44,0,0,0,
    0xff,0,0,0,0xff,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0x10,0,0x18,0,0x1c,0,0x18,0,0x10,0,0,0,0
};
void td_guidance_init(void) BANKED {set_bkg_data(241,sizeof(td_arrows)/16,td_arrows);}
void td_guidance_label(char *dest,const char *label) BANKED {
    WORD du,dv;UWORD au,av;UBYTE n=0;
    if(actors[1].flags&ACTOR_FLAG_HIDDEN){while(n<20&&label[n])n++;memmove(dest,label,n);dest[n]=0;return;}
    du=(WORD)(actors[1].pos.x>>5)-(WORD)(td.u>>4);
    dv=(WORD)(actors[1].pos.y>>5)+12-(WORD)(td.v>>4);
    au=du<0?-du:du;av=dv<0?-dv:dv;
    if(!strcmp(label,"RETURN FERRY AT DOCK"))label="RETURN FERRY DOCK";
    else if(!strcmp(label,"GO TO FERRY TERMINAL"))label="FERRY TERMINAL";
    while(n<18&&label[n])n++;
    memmove(dest+2,label,n);dest[n+2]=0;dest[1]=' ';
    dest[0]=au<12&&av<12?'+':au&&av&&au<av*2&&av<au*2?
        (du>0?(dv>0?6:5):(dv>0?7:8)):au>av?(du>0?2:4):(dv>0?3:1);
}
void td_guidance_compact(char *dest,UBYTE guided) BANKED {
    UWORD value;UBYTE i,n=0;
    while(n<11&&dest[n])n++;
    for(i=n;i<11;i++)dest[i]=' ';
    if(guided){
        value=td.left>999?999:td.left;
        dest[11]=' ';dest[12]=value>=100?'0'+value/100:' ';
        dest[13]=value>=10?'0'+value/10%10:' ';dest[14]='0'+value%10;
        dest[15]='S';dest[16]=' ';
    }else{
        value=td.cash;dest[11]='$';
        for(i=0;i<5;i++){dest[16-i]='0'+value%10;value/=10;}
        for(i=12;i<16&&dest[i]=='0';i++)dest[i]=' ';
    }
    for(i=0;i<3;i++)dest[17+i]=i<td.wanted&&
        !(td.wanted_left<30&&(td.seconds&1))?9:10;
    dest[20]=0;
}
