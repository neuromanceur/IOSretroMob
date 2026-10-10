// Relic Maze -- original Unity 3.x-era IMGUI puzzle prototype.
// Uses only established early Unity concepts: MonoBehaviour, OnGUI, Texture2D,
// Input.GetKeyDown and GUI.Button. Compiling in actual Unity 3.3 is UNVERIFIED.
using UnityEngine;

public class RelicMaze : MonoBehaviour
{
    private static readonly string[] Map = new string[] {
        "#########", "#....R..#", "#.#.#.#.#", "#.......#",
        "#.#...#.#", "#....T..#", "#.#...#.#", "#R..G.ER#", "#########"
    };
    private int px, py, bx, by, mask, moves;
    private bool won;
    private Texture2D pixel;

    private void Start()
    {
        pixel = new Texture2D(1, 1, TextureFormat.RGBA32, false);
        pixel.SetPixel(0, 0, Color.white);
        pixel.Apply();
        ResetGame();
    }

    private void OnDestroy()
    {
        if (pixel != null) Destroy(pixel);
    }

    private void ResetGame()
    {
        px = 1; py = 1; bx = 3; by = 5;
        mask = 0; moves = 0; won = false;
    }

    private char Tile(int x, int y)
    {
        if (x < 0 || x >= 9 || y < 0 || y >= 9) return '#';
        return Map[y][x];
    }

    private bool OnPlate() { return bx == 5 && by == 5; }

    private int RelicCount()
    {
        return ((mask & 1) != 0 ? 1 : 0) +
               ((mask & 2) != 0 ? 1 : 0) +
               ((mask & 4) != 0 ? 1 : 0);
    }

    private void Step(int dx, int dy)
    {
        if (won || (Mathf.Abs(dx) + Mathf.Abs(dy)) != 1) return;
        int tx = px + dx;
        int ty = py + dy;
        char t = Tile(tx, ty);
        if (t == '#' || (t == 'G' && !OnPlate())) return;
        if (tx == bx && ty == by) {
            int nx = bx + dx;
            int ny = by + dy;
            char nt = Tile(nx, ny);
            if (nt == '#' || nt == 'G' || nt == 'E') return;
            bx = nx; by = ny;
        }
        px = tx; py = ty;
        moves++;
        if (tx == 5 && ty == 1) mask |= 1;
        if (tx == 1 && ty == 7) mask |= 2;
        if (tx == 7 && ty == 7) mask |= 4;
        if (t == 'E' && mask == 7 && OnPlate()) won = true;
    }

    private void Update()
    {
        if (Input.GetKeyDown(KeyCode.UpArrow) || Input.GetKeyDown(KeyCode.W)) Step(0, -1);
        if (Input.GetKeyDown(KeyCode.DownArrow) || Input.GetKeyDown(KeyCode.S)) Step(0, 1);
        if (Input.GetKeyDown(KeyCode.LeftArrow) || Input.GetKeyDown(KeyCode.A)) Step(-1, 0);
        if (Input.GetKeyDown(KeyCode.RightArrow) || Input.GetKeyDown(KeyCode.D)) Step(1, 0);
        if (Input.GetKeyDown(KeyCode.R)) ResetGame();
    }

    private void Box(float x, float y, float w, float h, Color c)
    {
        GUI.color = c;
        GUI.DrawTexture(new Rect(x, y, w, h), pixel);
        GUI.color = Color.white;
    }

    private void OnGUI()
    {
        if (pixel == null) return;
        float cw = (Screen.width - 12.0f) / 9.0f;
        float ch = (Screen.height - 158.0f) / 9.0f;
        float s = Mathf.Max(10.0f, Mathf.Min(cw, ch));
        float ox = (Screen.width - 9.0f*s) * 0.5f;
        float oy = 48.0f;
        Color stone = new Color(0.30f, 0.39f, 0.32f, 1.0f);
        Color wall = new Color(0.13f, 0.25f, 0.23f, 1.0f);
        Box(0, 0, Screen.width, Screen.height, new Color(0.08f, 0.16f, 0.13f, 1.0f));
        GUI.Label(new Rect(12, 7, Screen.width-24, 22), "RELIC MAZE  |  iPhone 2011");
        GUI.Label(new Rect(12, 26, Screen.width-24, 22), "Relics " + RelicCount() + "/3     Moves " + moves + (OnPlate() ? "   GATE OPEN" : ""));
        for (int y=0; y<9; y++) {
            for (int x=0; x<9; x++) {
                float sx=ox+x*s, sy=oy+y*s;
                char t=Tile(x,y);
                Box(sx,sy,s-1,s-1,t=='#'?wall:stone);
                Box(sx+2,sy+2,s-5,2,new Color(0.42f,0.49f,0.39f,1.0f));
                if(t=='#') Box(sx+4,sy+7,s-9,s-13,new Color(0.20f,0.31f,0.28f,1.0f));
                if(t=='T') Box(sx+6,sy+6,s-13,s-13,OnPlate()?Color.green:Color.cyan);
                if(t=='G') Box(sx+5,sy+4,s-11,s-9,OnPlate()?Color.green:new Color(0.65f,0.4f,0.2f,1.0f));
                if(t=='E') Box(sx+7,sy+6,s-14,s-13,Color.green);
                if(t=='R') {
                    int idx=(x==5 && y==1)?0:(x==1 && y==7)?1:2;
                    if((mask & (1 << idx)) == 0) Box(sx+9,sy+9,s-19,s-19,Color.yellow);
                }
            }
        }
        Box(ox+bx*s+4,oy+by*s+4,s-9,s-9,new Color(0.70f,0.43f,0.20f,1.0f));
        Box(ox+px*s+8,oy+py*s+7,s-17,s-13,Color.yellow);
        Box(ox+px*s+10,oy+py*s+11,s-21,4,Color.black);
        float cy = oy + 9*s + 8;
        float cx = Screen.width * 0.5f;
        if (GUI.Button(new Rect(cx-20,cy,40,35), "^")) Step(0,-1);
        if (GUI.Button(new Rect(cx-64,cy+38,40,35), "<")) Step(-1,0);
        if (GUI.Button(new Rect(cx-20,cy+38,40,35), "v")) Step(0,1);
        if (GUI.Button(new Rect(cx+24,cy+38,40,35), ">")) Step(1,0);
        if (GUI.Button(new Rect(7, Screen.height-36, 88, 30), "RESTART")) ResetGame();
        if (won) {
            Box(16,oy+3*s,Screen.width-32,62,new Color(0.15f,0.33f,0.24f,1.0f));
            GUI.Label(new Rect(25,oy+3*s+9,Screen.width-50,42), "VICTORY! " + moves + " moves");
        }
    }
}
