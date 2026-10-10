/* Original Relic Maze SDK-free iPhone OS 3 frontend. MIT.
 * Same C game, UIKit controls, actual EAGL/ES1 requests. One measured textured
 * quad per input redraw; 7x7 playable interior in an original opaque 8x8 atlas.
 * No emulator special case, depth, blend, dither, SDK headers, or Unity runtime.
 */
#include "runtime.h"
#include "relic_maze.h"
#include "board_texture.h"
static RMGame game;
static Obj context, window, hud;
static U32 fb, rb, texture, frames;
static int width,height;
static unsigned char pixels[256];
static int (*syncfd)(int);
static int (*format)(char*,U32,const char*,...);
static void (*genfb)(int,U32*),(*genrb)(int,U32*),(*gentex)(int,U32*);
static void (*bindfb)(U32,U32),(*bindrb)(U32,U32),(*bindtex)(U32,U32);
static void (*attach)(U32,U32,U32,U32),(*getrb)(U32,U32,int*);
static U32 (*checkfb)(U32),(*error)(void);
static const char *(*getstr)(U32);
static void (*viewport)(int,int,int,int),(*matrix)(U32),(*identity)(void);
static void (*ortho)(float,float,float,float,float,float);
static void (*clearcolor)(float,float,float,float),(*clear)(U32);
static void (*disable)(U32),(*enable)(U32),(*parameter)(U32,U32,int);
static void (*image)(U32,int,int,int,int,int,U32,U32,const void*);
static void (*enableclient)(U32),(*vertex)(int,U32,int,const void*);
static void (*color)(int,U32,int,const void*),(*texcoord)(int,U32,int,const void*);
static void (*draw)(U32,int,int);
static Obj layerclass(Obj self,Sel cmd){(void)self;(void)cmd;return cls("CAEAGLLayer");}
static void flush_log(void){if(syncfd&&logfd>=0)syncfd(logfd);}
static void state(void){
 number("RELIC_PLAYER_X=",game.player_x);number("RELIC_PLAYER_Y=",game.player_y);
 number("RELIC_CRATE_X=",game.crate_x);number("RELIC_CRATE_Y=",game.crate_y);
 number("RELIC_MOVES=",game.moves);number("RELIC_MASK=",game.relic_mask);
 number("RELIC_PLATE=",rm_plate_active(&game));number("RELIC_WON=",game.won);
}
static void render(Obj self,Sel cmd,Obj unused){
 (void)self;(void)cmd;(void)unused;
 static const float xy[]={-.875f,-.44347826f,0, .875f,-.44347826f,0, .875f,.77391304f,0,
                         -.875f,-.44347826f,0, .875f,.77391304f,0, -.875f,.77391304f,0};
 static const float white[]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
 /* OpenGL v=0 is the first source row; put that row at the screen top. */
 static const float uv[]={0,.875f,.875f,.875f,.875f,0,0,.875f,.875f,0,0,0};
 ((int(*)(Obj,Sel,Obj))msg)(cls("EAGLContext"),sel("setCurrentContext:"),context);
 bindfb(0x8d40,fb);enable(0x0de1);bindtex(0x0de1,texture);
 rm_board_texture(&game,pixels);
 image(0x0de1,0,0x1908,8,8,0,0x1908,0x1401,pixels);
 number("RELIC_TEXTURE_ERROR=",error());
 viewport(0,0,width,height);clearcolor(.06f,.13f,.11f,1);clear(0x4000);
 matrix(0x1701);identity();ortho(-1,1,-1,1,-1,1);matrix(0x1700);identity();
 enableclient(0x8074);enableclient(0x8076);enableclient(0x8078);
 vertex(3,0x1406,0,xy);color(4,0x1406,0,white);texcoord(2,0x1406,0,uv);
 logline("RELIC_BEFORE_DRAW\n");draw(4,0,6);number("RELIC_DRAW_ERROR=",error());
 bindrb(0x8d41,rb);
 int presented=((int(*)(Obj,Sel,U32))msg)(context,sel("presentRenderbuffer:"),0x8d41);
 number("RELIC_PRESENT=",presented);number("RELIC_PRESENT_ERROR=",error());
 number("RELIC_FRAME=",++frames);state();
 char title[128];
 if(format){format(title,sizeof title,"Relic Maze  %u/3  %u moves%s",(unsigned)rm_relics_collected(&game),game.moves,game.won?"  WON":"");setobj(hud,"setText:",str(title));}
 flush_log();
}
static void tapped(Obj self,Sel cmd,Obj sender){
 (void)cmd;
 int direction=((int(*)(Obj,Sel))msg)(sender,sel("tag"));
 number("RELIC_TOUCH_DIRECTION=",direction);
 int result=0;
 if(direction==0)rm_init(&game);
 else result=rm_step(&game,direction==3?-1:direction==4?1:0,direction==1?-1:direction==2?1:0);
 number("RELIC_STEP_RESULT=",result);render(self,0,0);
}
static void button(Obj self,const char *title,int tag,Rect r){
 Obj b=((Obj(*)(Obj,Sel,int))msg)(cls("UIButton"),sel("buttonWithType:"),1);
 frame(b,r);setint(b,"setTag:",tag);
 ((void(*)(Obj,Sel,Obj,U32))msg)(b,sel("setTitle:forState:"),str(title),0);
 ((void(*)(Obj,Sel,Obj,Sel,U32))msg)(b,sel("addTarget:action:forControlEvents:"),self,sel("tapped:"),64);
 setobj(window,"addSubview:",b);
}
static void launched(Obj self,Sel cmd,Obj application){
 (void)cmd;logline("RELIC_UIKIT_LAUNCHED\n");
 /* Keep this input-driven game awake without a redraw/animation timer. */
 setint(application,"setIdleTimerDisabled:",1);
 number("RELIC_IDLE_DISABLED=",((int(*)(Obj,Sel))msg)(application,sel("isIdleTimerDisabled"))!=0);
 rm_init(&game);flush_log();
 void *g=api_dlopen("/System/Library/Frameworks/OpenGLES.framework/OpenGLES",2);
 number("RELIC_OPENGLES_HANDLE=",(U32)g);if(!g)return;
#define LOAD(dst,symbol) dst=api_dlsym(g,symbol);if(!dst){logline("RELIC_MISSING " symbol "\n");flush_log();return;}
 LOAD(genfb,"glGenFramebuffersOES");LOAD(genrb,"glGenRenderbuffersOES");LOAD(bindfb,"glBindFramebufferOES");LOAD(bindrb,"glBindRenderbufferOES");
 LOAD(attach,"glFramebufferRenderbufferOES");LOAD(getrb,"glGetRenderbufferParameterivOES");LOAD(checkfb,"glCheckFramebufferStatusOES");LOAD(error,"glGetError");LOAD(getstr,"glGetString");
 LOAD(viewport,"glViewport");LOAD(matrix,"glMatrixMode");LOAD(identity,"glLoadIdentity");LOAD(ortho,"glOrthof");LOAD(clearcolor,"glClearColor");LOAD(clear,"glClear");
 LOAD(enableclient,"glEnableClientState");LOAD(vertex,"glVertexPointer");LOAD(color,"glColorPointer");LOAD(texcoord,"glTexCoordPointer");LOAD(draw,"glDrawArrays");
 LOAD(disable,"glDisable");LOAD(enable,"glEnable");LOAD(gentex,"glGenTextures");LOAD(bindtex,"glBindTexture");LOAD(parameter,"glTexParameteri");LOAD(image,"glTexImage2D");
 context=((Obj(*)(Obj,Sel,U32))msg)(send(cls("EAGLContext"),"alloc"),sel("initWithAPI:"),1);
 number("RELIC_EAGL_CONTEXT=",(U32)context);if(!context){flush_log();return;}
 int current=((int(*)(Obj,Sel,Obj))msg)(cls("EAGLContext"),sel("setCurrentContext:"),context);
 number("RELIC_EAGL_CURRENT=",current);if(!current){flush_log();return;}
 disable(0x0bd0);disable(0x0be2);disable(0x0b71);logline("RELIC_NO_DITHER_BLEND_DEPTH\n");
 const char *s=getstr(0x1f02);if(s){logline("RELIC_GL_VERSION=");logline(s);logline("\n");}
 s=getstr(0x1f01);if(s){logline("RELIC_GL_RENDERER=");logline(s);logline("\n");}
 Obj v=allocclass(cls("UIView"),"RelicMazeGLView",0);
 Obj(*meta)(Obj)=api_dlsym((void*)-2,"object_getClass");
 if(!v||!meta||!addmethod(meta(v),sel("layerClass"),layerclass,"#8@0:4")){logline("RELIC_VIEW_CLASS_FAIL\n");flush_log();return;}
 registerclass(v);setint(application,"setStatusBarHidden:",1);
 window=rectinit("UIWindow",(Rect){0,0,320,480});setobj(window,"setBackgroundColor:",send(cls("UIColor"),"blackColor"));
 Obj view=rectinit("RelicMazeGLView",(Rect){0,20,320,460});Obj layer=send(view,"layer");setint(layer,"setOpaque:",1);setobj(window,"addSubview:",view);
 hud=rectinit("UILabel",(Rect){8,24,245,30});setobj(hud,"setTextColor:",send(cls("UIColor"),"whiteColor"));setobj(hud,"setBackgroundColor:",send(cls("UIColor"),"clearColor"));
 Obj font=((Obj(*)(Obj,Sel,float))msg)(cls("UIFont"),sel("systemFontOfSize:"),14);setobj(hud,"setFont:",font);setobj(window,"addSubview:",hud);
 button(self,"Reset",0,(Rect){258,24,56,30});button(self,"Up",1,(Rect){130,366,60,42});
 button(self,"Down",2,(Rect){130,414,60,42});button(self,"Left",3,(Rect){60,414,60,42});button(self,"Right",4,(Rect){200,414,60,42});
 send(window,"makeKeyAndVisible");
 genfb(1,&fb);genrb(1,&rb);bindfb(0x8d40,fb);bindrb(0x8d41,rb);
 int storage=((int(*)(Obj,Sel,U32,Obj))msg)(context,sel("renderbufferStorage:fromDrawable:"),0x8d41,layer);
 number("RELIC_RENDERBUFFER_STORAGE=",storage);if(!storage){flush_log();return;}
 getrb(0x8d41,0x8d42,&width);getrb(0x8d41,0x8d43,&height);number("RELIC_WIDTH=",width);number("RELIC_HEIGHT=",height);
 attach(0x8d40,0x8ce0,0x8d41,rb);U32 status=checkfb(0x8d40);number("RELIC_FBO_STATUS=",status);if(status!=0x8cd5){flush_log();return;}
 gentex(1,&texture);bindtex(0x0de1,texture);parameter(0x0de1,0x2801,0x2600);parameter(0x0de1,0x2800,0x2600);parameter(0x0de1,0x2802,0x812f);parameter(0x0de1,0x2803,0x812f);
 render(self,0,0);logline("RELIC_READY_FOR_TOUCH\n");flush_log();
}
int main(int argc,char **argv){
 if(!runtime("/private/var/tmp/relic-maze.log"))return 2;
 syncfd=api_dlsym((void*)-2,"fsync");format=api_dlsym((void*)-2,"snprintf");flush_log();
 return appmain(argc,argv,"RelicMazeDelegate",launched,tapped,0);
}
