#include "runtime.h"
static Obj window;
static int red;
static void tapped(Obj self,Sel cmd,Obj sender){
 (void)self;(void)cmd;(void)sender;red=!red;
 setobj(window,"setBackgroundColor:",send(cls("UIColor"),red?"redColor":"blueColor"));
 logline(red?"RM_TOUCH_RED\n":"RM_TOUCH_BLUE\n");
}
static void launched(Obj self,Sel cmd,Obj application){
 (void)cmd;(void)application;logline("RM_DID_FINISH_LAUNCHING\n");
 window=rectinit("UIWindow",(Rect){0,0,320,480});
 setobj(window,"setBackgroundColor:",send(cls("UIColor"),"blueColor"));
 Obj label=rectinit("UILabel",(Rect){20,60,280,80});
 setobj(label,"setText:",str("RETROMOBILE TEST"));
 setobj(label,"setTextColor:",send(cls("UIColor"),"whiteColor"));
 setobj(label,"setBackgroundColor:",send(cls("UIColor"),"clearColor"));
 setint(label,"setTextAlignment:",1);setobj(window,"addSubview:",label);
 Obj button=((Obj(*)(Obj,Sel,int))msg)(cls("UIButton"),sel("buttonWithType:"),1);
 frame(button,(Rect){60,190,200,80});
 ((void(*)(Obj,Sel,Obj,U32))msg)(button,sel("setTitle:forState:"),str("CHANGE COLOR"),0);
 ((void(*)(Obj,Sel,Obj,Sel,U32))msg)(button,sel("addTarget:action:forControlEvents:"),self,sel("tapped:"),64);
 setobj(window,"addSubview:",button);send(window,"makeKeyAndVisible");
 logline("RM_WINDOW_REQUESTED\n");
}
int main(int argc,char **argv){
 if(!runtime("/private/var/tmp/retromobile-uikit.log"))return 2;
 return appmain(argc,argv,"RMUIKitDelegate",launched,tapped,0);
}
