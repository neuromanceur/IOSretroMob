#ifndef RELIC_MAZE_H
#define RELIC_MAZE_H
#ifdef __cplusplus
extern "C" {
#endif

#define RM_WIDTH 9
#define RM_HEIGHT 9
#define RM_RELIC_COUNT 3

typedef struct {
    int player_x, player_y;
    int crate_x, crate_y;
    unsigned relic_mask;
    unsigned moves;
    int won;
} RMGame;

/* Static tile: '#':wall '.':floor 'R':relic 'T':plate 'G':gate 'E':exit */
char rm_tile(int x, int y);
void rm_init(RMGame *game);
int rm_plate_active(const RMGame *game);
int rm_gate_open(const RMGame *game);
int rm_relics_collected(const RMGame *game);
/* dx/dy are a single orthogonal step. Returns 0:blocked,1:moved,2:won. */
int rm_step(RMGame *game, int dx, int dy);

#ifdef __cplusplus
}
#endif
#endif
