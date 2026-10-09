/* All pixels are requested through the guest's own EAGL/OpenGLES stack. */
#include "runtime.h"
static Obj context,window;
static U32 fb,rb,frames;
static int width,height;
static float angle;
static void (*genfb)(int,U32*),(*genrb)(int,U32*);
static void (*bindfb)(U32,U32),(*bindrb)(U32,U32);
static void (*attach)(U32,U32,U32,U32),(*getrb)(U32,U32,int*);
static U32 (*checkfb)(U32),(*error)(void);
static const char *(*getstr)(U32);
static void (*viewport)(int,int,int,int),(*matrix)(U32),(*identity)(void);
static void (*ortho)(float,float,float,float,float,float);
static void (*rotate)(float,float,float,float);
static void (*clearcolor)(float,float,float,float),(*clear)(U32);
#ifdef RM_DISABLE_DITHER
static void (*disable)(U32);
#endif
static void (*enableclient)(U32),(*vertex)(int,U32,int,const void*),(*color)(int,U32,int,const void*),(*texcoord)(int,U32,int,const void*),(*draw)(U32,int,int);
static Obj layerclass(Obj self,Sel cmd){(void)self;(void)cmd;return cls("CAEAGLLayer");}
static void render(Obj self,Sel cmd,Obj timer){
 (void)self;(void)cmd;(void)timer;
 static const float vertices[]={-0.7f,-0.6f,0,0.7f,-0.6f,0,0,0.7f,0};
 static const float colors[]={1,0,0,1,0,1,0,1,0,0,1,1};
 static const float uv[]={0,0,1,0,0.5f,1};
 ((int(*)(Obj,Sel,Obj))msg)(cls("EAGLContext"),sel("setCurrentContext:"),context);
 bindfb(0x8d40,fb);viewport(0,0,width,height);clearcolor(0.05f,0.05f,0.12f,1);clear(0x4000);
 matrix(0x1701);identity();ortho(-1,1,-1,1,-1,1);matrix(0x1700);identity();rotate(angle,0,0,1);
 enableclient(0x8074);enableclient(0x8076);vertex(3,0x1406,0,vertices);color(4,0x1406,0,colors);texcoord(2,0x1406,0,uv);
 if(!frames)logline("RM_BEFORE_GL_DRAWARRAYS\n");
 draw(4,0,3);if(!frames)logline("RM_AFTER_GL_DRAWARRAYS\n");bindrb(0x8d41,rb);
 if(!frames)logline("RM_BEFORE_PRESENT\n");
 int presented=((int(*)(Obj,Sel,U32))msg)(context,sel("presentRenderbuffer:"),0x8d41);
 frames++;angle+=10;if(angle>=360)angle-=360;
 if(frames<=3 || !(frames%30)){number("RM_DRAW_FRAME=",frames);number("RM_PRESENT=",presented);number("RM_GL_ERROR=",error());}
}
static void launched(Obj self,Sel cmd,Obj application){
 (void)cmd;(void)application;logline("RM_GLES_LAUNCH\n");
 void *g=api_dlopen("/System/Library/Frameworks/OpenGLES.framework/OpenGLES",2);number("RM_OPENGLES_HANDLE=",(U32)g);
 if(!g){logline("RM_GLES_FRAMEWORK_FAIL\n");return;}
#define LOAD(dst,symbol) dst=api_dlsym(g,symbol);if(!dst){logline("RM_MISSING " symbol "\n");return;}
 LOAD(genfb,"glGenFramebuffersOES");LOAD(genrb,"glGenRenderbuffersOES");LOAD(bindfb,"glBindFramebufferOES");LOAD(bindrb,"glBindRenderbufferOES");
 LOAD(attach,"glFramebufferRenderbufferOES");LOAD(getrb,"glGetRenderbufferParameterivOES");LOAD(checkfb,"glCheckFramebufferStatusOES");LOAD(error,"glGetError");LOAD(getstr,"glGetString");
 LOAD(viewport,"glViewport");LOAD(matrix,"glMatrixMode");LOAD(identity,"glLoadIdentity");LOAD(ortho,"glOrthof");LOAD(rotate,"glRotatef");LOAD(clearcolor,"glClearColor");LOAD(clear,"glClear");
 LOAD(enableclient,"glEnableClientState");LOAD(vertex,"glVertexPointer");LOAD(color,"glColorPointer");LOAD(texcoord,"glTexCoordPointer");LOAD(draw,"glDrawArrays");
 logline("RM_BEFORE_EAGL_CONTEXT\n");
 context=((Obj(*)(Obj,Sel,U32))msg)(send(cls("EAGLContext"),"alloc"),sel("initWithAPI:"),1);
 number("RM_EAGL_CONTEXT=",(U32)context);if(!context){logline("RM_EAGL_CONTEXT_FAIL\n");return;}
 int current=((int(*)(Obj,Sel,Obj))msg)(cls("EAGLContext"),sel("setCurrentContext:"),context);number("RM_EAGL_CURRENT=",current);if(!current)return;
#ifdef RM_DISABLE_DITHER
 LOAD(disable,"glDisable");disable(0x0bd0);logline("RM_DITHER_DISABLED\n");
#endif
 const char *version=getstr(0x1f02),*renderer=getstr(0x1f01);if(version){logline("RM_GL_VERSION=");logline(version);logline("\n");}if(renderer){logline("RM_GL_RENDERER=");logline(renderer);logline("\n");}
 Obj viewcls=allocclass(cls("UIView"),"RMGLView",0);
 Obj(*meta)(Obj)=api_dlsym((void*)-2,"object_getClass");addmethod(meta(viewcls),sel("layerClass"),layerclass,"#8@0:4");registerclass(viewcls);
 window=rectinit("UIWindow",(Rect){0,0,320,480});setobj(window,"setBackgroundColor:",send(cls("UIColor"),"blackColor"));
 Obj view=rectinit("RMGLView",(Rect){0,20,320,460});Obj layer=send(view,"layer");setint(layer,"setOpaque:",1);setobj(window,"addSubview:",view);send(window,"makeKeyAndVisible");
 genfb(1,&fb);genrb(1,&rb);bindfb(0x8d40,fb);bindrb(0x8d41,rb);
 logline("RM_BEFORE_RENDERBUFFER_STORAGE\n");
 int storage=((int(*)(Obj,Sel,U32,Obj))msg)(context,sel("renderbufferStorage:fromDrawable:"),0x8d41,layer);number("RM_RENDERBUFFER_STORAGE=",storage);if(!storage)return;
 getrb(0x8d41,0x8d42,&width);getrb(0x8d41,0x8d43,&height);number("RM_WIDTH=",width);number("RM_HEIGHT=",height);
 attach(0x8d40,0x8ce0,0x8d41,rb);U32 status=checkfb(0x8d40);number("RM_FRAMEBUFFER_STATUS=",status);if(status!=0x8cd5)return;
 render(self,0,0);
 ((Obj(*)(Obj,Sel,double,Obj,Sel,Obj,int))msg)(cls("NSTimer"),sel("scheduledTimerWithTimeInterval:target:selector:userInfo:repeats:"),0.1,self,sel("render:"),0,1);
 logline("RM_ANIMATION_TIMER_SCHEDULED\n");
}
int main(int argc,char **argv){if(!runtime("/private/var/tmp/retromobile-gles1.log"))return 2;return appmain(argc,argv,"RMGLESDelegate",launched,0,render);}
