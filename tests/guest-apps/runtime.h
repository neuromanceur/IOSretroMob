/* Own SDK-free guest diagnostics. MIT; used with S5LBox by j0shua-SYSON. */
typedef unsigned int U32;
typedef void *Obj;
typedef void *Sel;
typedef struct { float x,y,w,h; } Rect;
extern void *api_dlopen(const char *, int);
extern void *api_dlsym(void *, const char *);
static Obj (*cls)(const char *);
static Sel (*sel)(const char *);
static void *msg;
static Obj (*allocclass)(Obj,const char *,U32);
static void (*registerclass)(Obj);
static int (*addmethod)(Obj,Sel,void *,const char *);
static int (*fopen_)(const char *,int,...);
static int (*write_)(int,const void *,U32);
static int (*getpid_)(void);
static void (*exit_)(int);
static int logfd=-1;
static U32 len(const char *s){U32 n=0;while(s[n])n++;return n;}
static void logline(const char *s){if(write_){write_(1,s,len(s));if(logfd>=0)write_(logfd,s,len(s));}}
static void number(const char *s,U32 v){char b[12];U32 i;logline(s);for(i=0;i<8;i++)b[i]="0123456789abcdef"[(v>>(28-i*4))&15];b[8]='\n';b[9]=0;logline(b);}
static Obj send(Obj o,const char*s){return ((Obj(*)(Obj,Sel))msg)(o,sel(s));}
static Obj str(const char *s){return ((Obj(*)(Obj,Sel,const char*))msg)(cls("NSString"),sel("stringWithUTF8String:"),s);}
static void setobj(Obj o,const char*s,Obj v){((void(*)(Obj,Sel,Obj))msg)(o,sel(s),v);}
static void setint(Obj o,const char*s,int v){((void(*)(Obj,Sel,int))msg)(o,sel(s),v);}
static void frame(Obj o,Rect r){((void(*)(Obj,Sel,Rect))msg)(o,sel("setFrame:"),r);}
static Obj rectinit(const char*c,Rect r){return ((Obj(*)(Obj,Sel,Rect))msg)(send(cls(c),"alloc"),sel("initWithFrame:"),r);}
static int runtime(const char *logpath){
 void *sys=api_dlopen("/usr/lib/libSystem.B.dylib",2);
 write_=api_dlsym(sys,"write");fopen_=api_dlsym(sys,"open");getpid_=api_dlsym(sys,"getpid");exit_=api_dlsym(sys,"exit");
 if(fopen_)logfd=fopen_(logpath,0x209,0644);
 logline("RM_MAIN_ENTER\n");if(getpid_)number("RM_PID=",getpid_());
 void *objc=api_dlopen("/usr/lib/libobjc.A.dylib",2);
 cls=api_dlsym(objc,"objc_getClass");sel=api_dlsym(objc,"sel_registerName");msg=api_dlsym(objc,"objc_msgSend");
 allocclass=api_dlsym(objc,"objc_allocateClassPair");registerclass=api_dlsym(objc,"objc_registerClassPair");addmethod=api_dlsym(objc,"class_addMethod");
 void *f=api_dlopen("/System/Library/Frameworks/Foundation.framework/Foundation",2);
 void *u=api_dlopen("/System/Library/Frameworks/UIKit.framework/UIKit",2);
 number("RM_FOUNDATION_HANDLE=",(U32)f);number("RM_UIKIT_HANDLE=",(U32)u);
 if(!f||!u||!cls||!sel||!msg||!allocclass||!registerclass||!addmethod){logline("RM_RUNTIME_FAIL\n");return 0;}
 return 1;
}
static int appmain(int argc,char **argv,const char *name,void *launch,void *tap,void *render){
 Obj pool=send(send(cls("NSAutoreleasePool"),"alloc"),"init");(void)pool;
 Obj delegate=allocclass(cls("NSObject"),name,0);
 if(!delegate){logline("RM_CLASS_FAIL\n");return 3;}
 if(!addmethod(delegate,sel("applicationDidFinishLaunching:"),launch,"v12@0:4@8")){logline("RM_METHOD_FAIL\n");return 4;}
 if(tap)addmethod(delegate,sel("tapped:"),tap,"v12@0:4@8");
 if(render)addmethod(delegate,sel("render:"),render,"v12@0:4@8");
 registerclass(delegate);logline("RM_DELEGATE_REGISTERED\n");
 int(*main_)(int,char**,Obj,Obj)=api_dlsym((void*)-2,"UIApplicationMain");
 if(!main_){logline("RM_UIAPPLICATIONMAIN_MISSING\n");return 5;}
 logline("RM_UIAPPLICATIONMAIN\n");return main_(argc,argv,0,str(name));
}
