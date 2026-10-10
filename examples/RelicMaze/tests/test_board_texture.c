#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board_texture.h"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);}}while(0)
static unsigned char *cell(unsigned char *p,int x,int y){return p+4*((y-1)*8+x-1);}
int main(void){
 RMGame g;unsigned char p[256],before[256];rm_init(&g);rm_board_texture(&g,p);
 for(int i=0;i<64;i++)CHECK(p[i*4+3]==255);
 CHECK(cell(p,1,1)[1]==204&&cell(p,1,1)[2]==107);
 CHECK(cell(p,3,5)[0]==168);CHECK(cell(p,5,1)[1]==189);
 memcpy(before,p,sizeof p);CHECK(rm_step(&g,-1,0)==0);rm_board_texture(&g,p);CHECK(!memcmp(p,before,sizeof p));
 for(int i=0;i<6;i++)CHECK(rm_step(&g,0,1)==1);
 rm_board_texture(&g,p);CHECK(g.relic_mask==2);CHECK(cell(p,1,1)[1]!=204);CHECK(cell(p,1,7)[1]==204);
 const char *rest="UURRRUURUUDDRRDDDDL";
 for(int i=0;rest[i];i++){char c=rest[i];CHECK(rm_step(&g,c=='R'?1:c=='L'?-1:0,c=='D'?1:c=='U'?-1:0)>0);}
 rm_board_texture(&g,p);CHECK(g.won);CHECK(cell(p,4,7)[0]==46);CHECK(cell(p,4,7)[1]==171);CHECK(cell(p,5,1)[1]!=189);CHECK(cell(p,5,5)[0]==168);
 puts("PASS: opaque original atlas, blocked/moved/collected states, crate/plate/gate and winning route");return 0;
}
