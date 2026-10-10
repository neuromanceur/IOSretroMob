const assert = require('node:assert/strict');
const {Game, MAP} = require('./game.js');
const g = new Game();
assert.equal(MAP.length,9);
for (const row of MAP) assert.equal(row.length,9);
assert.equal(g.move(-1,0),0);
assert.equal(g.move(1,1),0);
const path='DDDDDDUURRRUURUUDDRRDDDDL';
let last=0;
for(const move of path){
  const moves={U:[0,-1],D:[0,1],L:[-1,0],R:[1,0]};
  last=g.move(...moves[move]);
  assert.notEqual(last,0,`stuck on move ${move}`);
}
assert.equal(last,2);
assert.equal(g.won,true);
assert.equal(g.plate(),true);
assert.equal(g.collected(),3);
assert.equal(g.moves,25);
g.reset();
assert.equal(g.moves,0);
console.log('PASS: browser engine, 25-move solution, reset, gates & relics');
