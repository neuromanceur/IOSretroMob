#include "relic_maze.h"

static const char *const level[RM_HEIGHT] = {
    "#########",
    "#....R..#",
    "#.#.#.#.#",
    "#.......#",
    "#.#...#.#",
    "#....T..#",
    "#.#...#.#",
    "#R..G.ER#",
    "#########"
};

static const int relic_x[RM_RELIC_COUNT] = {5, 1, 7};
static const int relic_y[RM_RELIC_COUNT] = {1, 7, 7};

char rm_tile(int x, int y) {
    if (x < 0 || y < 0 || x >= RM_WIDTH || y >= RM_HEIGHT) return '#';
    return level[y][x];
}

void rm_init(RMGame *g) {
    if (!g) return;
    g->player_x = 1; g->player_y = 1;
    g->crate_x = 3; g->crate_y = 5;
    g->relic_mask = 0; g->moves = 0; g->won = 0;
}

int rm_plate_active(const RMGame *g) {
    return g && rm_tile(g->crate_x, g->crate_y) == 'T';
}

int rm_gate_open(const RMGame *g) {
    return rm_plate_active(g);
}

int rm_relics_collected(const RMGame *g) {
    unsigned m;
    int n=0;
    if (!g) return 0;
    m = g->relic_mask;
    while (m) { n += (int)(m & 1u); m >>= 1; }
    return n;
}

int rm_step(RMGame *g, int dx, int dy) {
    int x, y, bx, by, i;
    char tile;
    if (!g || g->won) return 0;
    if ((dx != 0 && dy != 0) || (dx == 0 && dy == 0) ||
        dx < -1 || dx > 1 || dy < -1 || dy > 1) return 0;
    x = g->player_x + dx; y = g->player_y + dy;
    tile = rm_tile(x, y);
    if (tile == '#' || (tile == 'G' && !rm_gate_open(g))) return 0;
    if (x == g->crate_x && y == g->crate_y) {
        bx = g->crate_x + dx; by = g->crate_y + dy;
        tile = rm_tile(bx, by);
        if (tile == '#' || tile == 'G' || tile == 'E') return 0;
        g->crate_x = bx; g->crate_y = by;
    }
    g->player_x = x; g->player_y = y;
    ++g->moves;
    for (i=0; i<RM_RELIC_COUNT; ++i) {
        if (x == relic_x[i] && y == relic_y[i]) g->relic_mask |= 1u << i;
    }
    if (rm_tile(x,y) == 'E' && g->relic_mask == 7u && rm_gate_open(g)) {
        g->won = 1;
        return 2;
    }
    return 1;
}
