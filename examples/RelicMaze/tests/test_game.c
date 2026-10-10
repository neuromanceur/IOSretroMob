#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "relic_maze.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1);} } while (0)

int main(void) {
    RMGame g, copy;
    size_t i;
    const char *solution = "DDDDDDUURRRUURUUDDRRDDDDL";
    int result=0;
    rm_init(&g);
    CHECK(g.player_x==1 && g.player_y==1 && g.crate_x==3 && g.crate_y==5);
    CHECK(rm_tile(-1,0)=='#' && rm_tile(999,0)=='#');
    CHECK(rm_step(&g,-1,0)==0 && g.moves==0);
    CHECK(rm_step(&g,1,1)==0);
    CHECK(!rm_plate_active(&g));
    for (i=0; i<strlen(solution); ++i) {
        char c=solution[i];
        int dx = c=='R'?1:c=='L'?-1:0;
        int dy = c=='D'?1:c=='U'?-1:0;
        result=rm_step(&g,dx,dy);
        CHECK(result!=0);
    }
    CHECK(result==2);
    CHECK(g.won && rm_plate_active(&g));
    CHECK(rm_relics_collected(&g)==3);
    CHECK(g.moves==strlen(solution));
    copy=g;
    CHECK(rm_step(&g,1,0)==0);
    CHECK(memcmp(&copy,&g,sizeof(g))==0);
    rm_init(&g);
    CHECK(g.relic_mask==0 && !g.won && g.moves==0);
    puts("PASS: Relic Maze C99 engine + complete 25-move winning route");
    return 0;
}
