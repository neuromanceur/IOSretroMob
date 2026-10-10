"""Validate the same level in all three implementations; solve independently with BFS."""
from pathlib import Path
from collections import deque
import re
R = Path(__file__).resolve().parents[1]

def get_map(filename, begin, end):
    txt = (R / filename).read_text()
    a = txt.index(begin)
    b = txt.index(end, a + len(begin))
    return re.findall(r'"([#.RTEG]{9})"', txt[a:b])

c = get_map('core/relic_maze.c', 'static const char *const level', 'static const int relic_x')
js = get_map('web/game.js', 'var MAP = [', 'var RELICS')
cs = get_map('unity33/Assets/Scripts/RelicMaze.cs', 'private static readonly string[] Map', 'private int px')
assert c == js == cs, (c, js, cs)
assert len(c) == 9 and all(len(row) == 9 for row in c)
assert c[5][5] == 'T' and c[7][4] == 'G' and c[7][6] == 'E'
relics = [(5,1),(1,7),(7,7)]
start = (1,1,3,5,0)
seen = {start}
queue = deque([(start, '')])
steps = [(0,-1,'U'), (0,1,'D'),(-1,0,'L'), (1,0,'R')]
solution = None
while queue:
    (px,py,bx,by,mask), path = queue.popleft()
    for dx,dy,letter in steps:
        tx,ty=px+dx,py+dy
        t = c[ty][tx]
        if t == '#' or (t == 'G' and (bx,by) != (5,5)):
            continue
        nbx,nby=bx,by
        if (tx,ty)==(bx,by):
            nbx,nby=bx+dx,by+dy
            if c[nby][nbx] in '#GE':
                continue
        m=mask
        if (tx,ty) in relics:
            m |= 1 << relics.index((tx,ty))
        newpath=path+letter
        if t=='E' and m == 7 and (nbx,nby) == (5,5):
            solution = newpath
            break
        key=(tx,ty,nbx,nby,m)
        if key not in seen:
            seen.add(key)
            queue.append((key,newpath))
    if solution is not None:
        break
assert solution == 'DDDDDDUURRRUURUUDDRRDDDDL', solution
print('PASS: 3 source maps identical, independently verified 25-step winning route')
