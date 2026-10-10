/*
 * Relic Maze — experimental native iPhone OS 3.x front end (Objective-C MRC).
 * Requires an authentic compatible iPhoneOS SDK/toolchain; NOT compiled here.
 * Draws only opaque untextured GL ES 1.1 triangles, no depth/blend/dither.
 * Copyright 2026 IOSretroMob contributors. MIT.
 */
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <math.h>
#import "../core/relic_maze.h"

@interface RMGLView : UIView {
    EAGLContext *_context;
    GLuint _fbo, _color;
    BOOL _ready;
    RMGame _game;
}
- (void)drawGame;
@end

@implementation RMGLView
+ (Class)layerClass { return [CAEAGLLayer class]; }

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        rm_init(&_game);
        self.multipleTouchEnabled = NO;
        CAEAGLLayer *layer = (CAEAGLLayer *)self.layer;
        layer.opaque = YES;
        layer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
            kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
        _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
        _fbo = 0; _color = 0; _ready = NO;
    }
    return self;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    if (!_context) return;
    [EAGLContext setCurrentContext:_context];
    if (!_fbo) glGenFramebuffersOES(1, &_fbo);
    if (!_color) glGenRenderbuffersOES(1, &_color);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, _fbo);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, _color);
    if (![_context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer *)self.layer]) return;
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                GL_RENDERBUFFER_OES, _color);
    _ready = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) == GL_FRAMEBUFFER_COMPLETE_OES;
    if (_ready) [self drawGame];
}

static void quad(float x, float y, float w, float h, float r, float g, float b) {
    GLfloat v[18] = {
        x,y,0, x+w,y,0, x+w,y+h,0,
        x,y,0, x+w,y+h,0, x,y+h,0
    };
    GLfloat c[24];
    int i;
    for (i=0;i<6;i++) { c[i*4]=r; c[i*4+1]=g; c[i*4+2]=b; c[i*4+3]=1.0f; }
    glVertexPointer(3,GL_FLOAT,0,v);
    glColorPointer(4,GL_FLOAT,0,c);
    /* Explicit 3-vertex calls match the existing narrow VGP diagnostic form. */
    glDrawArrays(GL_TRIANGLES,0,3);
    glDrawArrays(GL_TRIANGLES,3,3);
}

- (void)drawGame {
    int x,y,i;
    GLfloat w,h,size,ox,oy,pady;
    if (!_ready) return;
    [EAGLContext setCurrentContext:_context];
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, _fbo);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, _color);
    w = (GLfloat)self.bounds.size.width;
    h = (GLfloat)self.bounds.size.height;
    size = fminf((w-12.0f)/9.0f,(h-150.0f)/9.0f);
    if (size < 4.0f) return;
    ox = (w-9.0f*size)*0.5f; oy=41.0f;
    pady=oy+9.0f*size+13.0f;
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    glDisable(GL_DITHER); glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrthof(0,w,h,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    glEnableClientState(GL_VERTEX_ARRAY);glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glClearColor(0.06f,0.13f,0.11f,1.0f);glClear(GL_COLOR_BUFFER_BIT);
    /* Upper HUD: three colored markers and open-plate indicator. */
    for(i=0;i<3;i++)quad(ox+8+i*23,11,15,14,
        (_game.relic_mask&(1u<<i))?0.92f:0.23f,
        (_game.relic_mask&(1u<<i))?0.70f:0.27f,0.26f);
    quad(w-40,11,23,14,rm_plate_active(&_game)?0.25f:0.23f,
         rm_plate_active(&_game)?0.81f:0.31f,0.49f);
    for(y=0;y<9;y++) for(x=0;x<9;x++) {
        float sx=ox+x*size, sy=oy+y*size;
        char t=rm_tile(x,y);
        float f=((x+y)&1)?0.30f:0.33f;
        if(t=='#') {
            quad(sx,sy,size-1,size-1,0.13f,0.24f,0.22f);
            quad(sx+3,sy+5,size-7,size-10,0.22f,0.34f,0.29f);
        } else {
            quad(sx,sy,size-1,size-1,f,f+0.08f,f+0.01f);
        }
        if(t=='T')quad(sx+5,sy+5,size-11,size-11,
            rm_plate_active(&_game)?0.29f:0.24f,
            rm_plate_active(&_game)?0.85f:0.43f,0.56f);
        if(t=='G')quad(sx+4,sy+4,size-9,size-9,
            rm_gate_open(&_game)?0.18f:0.72f,
            rm_gate_open(&_game)?0.67f:0.36f,0.26f);
        if(t=='E')quad(sx+6,sy+6,size-13,size-13,0.20f,0.80f,0.57f);
        if(t=='R') {
            int bit=(x==5&&y==1)?0:(x==1&&y==7)?1:2;
            if(!(_game.relic_mask&(1u<<bit)))
                quad(sx+9,sy+9,size-19,size-19,0.97f,0.74f,0.23f);
        }
    }
    quad(ox+_game.crate_x*size+4,oy+_game.crate_y*size+4,
         size-9,size-9,0.66f,0.39f,0.20f);
    quad(ox+_game.player_x*size+9,oy+_game.player_y*size+7,
         size-19,size-14,0.97f,0.80f,0.42f);
    /* On-screen D-pad. Top center, then left/down/right. */
    quad(w/2-17,pady,34,30,0.26f,0.53f,0.47f);
    quad(w/2-55,pady+35,34,30,0.26f,0.53f,0.47f);
    quad(w/2-17,pady+35,34,30,0.26f,0.53f,0.47f);
    quad(w/2+21,pady+35,34,30,0.26f,0.53f,0.47f);
    if(_game.won) {
        quad(ox+10,oy+size*3,size*9-20,size*2,0.05f,0.37f,0.29f);
        quad(ox+20,oy+size*3+12,size*9-40,size*2-24,0.93f,0.69f,0.22f);
    }
    glFlush();
    [_context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [touches anyObject];
    CGPoint p = [touch locationInView:self];
    float w=self.bounds.size.width, h=self.bounds.size.height;
    float s=fminf((w-12.0f)/9.0f,(h-150.0f)/9.0f);
    float ox=(w-9.0f*s)*0.5f, oy=41.0f, pady=oy+9*s+13.0f;
    int dx=0,dy=0;
    (void)event;
    if (p.y < 35.0f) {
        rm_init(&_game);
    } else if (p.y >= oy && p.y < oy+9*s && p.x>=ox && p.x<ox+9*s) {
        int tx=(int)((p.x-ox)/s), ty=(int)((p.y-oy)/s);
        dx=tx-_game.player_x;dy=ty-_game.player_y;
    } else if (p.y>=pady && p.y<pady+33 && p.x>=w/2-17 && p.x<=w/2+17) {
        dy=-1;
    } else if (p.y>=pady+35 && p.y<pady+69) {
        if(p.x>=w/2-55 && p.x<=w/2-21) dx=-1;
        else if(p.x>=w/2-17 && p.x<=w/2+17) dy=1;
        else if(p.x>=w/2+21 && p.x<=w/2+55) dx=1;
    }
    if((dx==0 && (dy==1||dy==-1)) || (dy==0 && (dx==1||dx==-1)))
        rm_step(&_game,dx,dy);
    [self drawGame];
}

- (void)dealloc {
    if(_context) {
        [EAGLContext setCurrentContext:_context];
        if(_fbo) glDeleteFramebuffersOES(1,&_fbo);
        if(_color) glDeleteRenderbuffersOES(1,&_color);
        [EAGLContext setCurrentContext:nil];
        [_context release];
    }
    [super dealloc];
}
@end

@interface RMAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *_window;
}
@end
@implementation RMAppDelegate
- (void)applicationDidFinishLaunching:(UIApplication *)application {
    [application setStatusBarHidden:YES];
    _window=[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    RMGLView *view=[[RMGLView alloc] initWithFrame:[_window bounds]];
    [_window addSubview:view];
    [view release];
    [_window makeKeyAndVisible];
}
- (void)dealloc { [_window release]; [super dealloc]; }
@end

int main(int argc, char **argv) {
    NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];
    int result=UIApplicationMain(argc,argv,nil,@"RMAppDelegate");
    [pool release];
    return result;
}
