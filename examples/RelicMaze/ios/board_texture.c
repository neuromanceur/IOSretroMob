#include "board_texture.h"
void rm_board_texture(const RMGame *g,unsigned char rgba[256]){
 for(int y=0;y<8;y++)for(int x=0;x<8;x++){
  int tx=x+1,ty=y+1;char t=rm_tile(tx,ty);
  unsigned char r=84,gc=104,b=87;
  if((tx+ty)&1){r=77;gc=97;b=79;}
  if(t=='#'){r=33;gc=61;b=56;}
  if(t=='T'){r=rm_plate_active(g)?74:61;gc=rm_plate_active(g)?217:110;b=143;}
  if(t=='G'){r=rm_gate_open(g)?46:184;gc=rm_gate_open(g)?171:92;b=66;}
  if(t=='E'){r=51;gc=204;b=145;}
  if(t=='R'){
   int bit=tx==5&&ty==1?0:tx==1&&ty==7?1:2;
   if(!(g->relic_mask&(1u<<bit))){r=247;gc=189;b=59;}
  }
  if(tx==g->crate_x&&ty==g->crate_y){r=168;gc=99;b=51;}
  if(tx==g->player_x&&ty==g->player_y){r=247;gc=204;b=107;}
  int i=4*(y*8+x);rgba[i]=r;rgba[i+1]=gc;rgba[i+2]=b;rgba[i+3]=255;
 }
}
