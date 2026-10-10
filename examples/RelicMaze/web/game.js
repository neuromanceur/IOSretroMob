/* Relic Maze -- original retro puzzle, no external dependencies. MIT. */
(function (root) {
  "use strict";
  var MAP = [
    "#########",
    "#....R..#",
    "#.#.#.#.#",
    "#.......#",
    "#.#...#.#",
    "#....T..#",
    "#.#...#.#",
    "#R..G.ER#",
    "#########"
  ];
  var RELICS = [[5,1],[1,7],[7,7]];
  function Game() { this.reset(); }
  Game.prototype.reset = function () {
    this.x=1; this.y=1; this.bx=3; this.by=5;
    this.mask=0; this.moves=0; this.won=false;
  };
  Game.prototype.tile = function (x,y) {
    return y<0 || y>=9 || x<0 || x>=9 ? '#' : MAP[y].charAt(x);
  };
  Game.prototype.plate = function () { return this.bx===5 && this.by===5; };
  Game.prototype.collected = function () {
    return (this.mask&1)+((this.mask>>1)&1)+((this.mask>>2)&1);
  };
  Game.prototype.move = function (dx,dy) {
    if (this.won || Math.abs(dx)+Math.abs(dy)!==1) return 0;
    var x=this.x+dx, y=this.y+dy, t=this.tile(x,y);
    if (t==='#' || (t==='G' && !this.plate())) return 0;
    if (x===this.bx && y===this.by) {
      var nx=this.bx+dx, ny=this.by+dy, bt=this.tile(nx,ny);
      if (bt==='#' || bt==='G' || bt==='E') return 0;
      this.bx=nx; this.by=ny;
    }
    this.x=x; this.y=y; this.moves++;
    for (var i=0;i<RELICS.length;i++) {
      if (x===RELICS[i][0] && y===RELICS[i][1]) this.mask|=(1<<i);
    }
    if (this.tile(x,y)==='E' && this.mask===7 && this.plate()) {
      this.won=true; return 2;
    }
    return 1;
  };
  var api={Game:Game,MAP:MAP,RELICS:RELICS};
  if (typeof module!=='undefined' && module.exports) module.exports=api;
  root.RelicMaze=api;
})(typeof window!=='undefined'?window:globalThis);
