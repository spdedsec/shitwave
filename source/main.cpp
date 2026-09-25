#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct Vec2 { double x{}, y{}; };
static Vec2 operator+(Vec2 a, Vec2 b) { return {a.x+b.x,a.y+b.y}; }
static Vec2 operator-(Vec2 a, Vec2 b) { return {a.x-b.x,a.y-b.y}; }
static Vec2 operator*(Vec2 a, double s) { return {a.x*s,a.y*s}; }
static double len(Vec2 a) { return std::sqrt(a.x*a.x+a.y*a.y); }
static Vec2 norm(Vec2 a) { double l=len(a); return l>1e-9 ? a*(1.0/l) : Vec2{}; }
static double clampd(double v,double a,double b){return std::max(a,std::min(b,v));}

constexpr int W=1280,H=720;
constexpr double PI=3.14159265358979323846;

struct Bullet { Vec2 p,v; double life=1.2; double r=4; };
struct Enemy { Vec2 p,v; int hp=1,maxhp=1,type=0; double r=12; int value=100; };
struct Particle { Vec2 p,v; double life=0.45,maxLife=0.45,r=3; unsigned long color=0; };
struct Pickup { Vec2 p; int type=0; double life=8.0; };

struct Game {
    Vec2 player{W/2.0,H/2.0};
    int hp=5,maxHp=5;
    int score=0,wave=1,kills=0;
    double time=0, shootCd=0, waveTime=15, spawnCd=0, dashCd=0, dashFlash=0;
    double combo=0, comboTimer=0;
    bool over=false, paused=false;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<Particle> particles;
    std::vector<Pickup> pickups;
    std::mt19937 rng{std::random_device{}()};

    double rnd(double a,double b){ std::uniform_real_distribution<double>d(a,b); return d(rng); }
    int rndi(int a,int b){ std::uniform_int_distribution<int>d(a,b); return d(rng); }

    void reset(){
        player={W/2.0,H/2.0}; hp=maxHp=5; score=kills=0; wave=1; time=0; waveTime=15; spawnCd=0.2; shootCd=0; dashCd=0; dashFlash=0; combo=0; comboTimer=0; over=paused=false;
        bullets.clear(); enemies.clear(); particles.clear(); pickups.clear();
    }

    void burst(Vec2 p, unsigned long c, int n=10, double speed=90){
        for(int i=0;i<n;i++){
            double a=rnd(0,PI*2), s=rnd(speed*0.35,speed);
            Particle q; q.p=p; q.v={std::cos(a)*s,std::sin(a)*s}; q.life=q.maxLife=rnd(0.18,0.55); q.r=rnd(2,5); q.color=c; particles.push_back(q);
        }
    }

    void spawnEnemy(){
        double side=rnd(0,4); Vec2 p;
        if(side<1) p={rnd(-30,W+30),-25}; else if(side<2) p={rnd(-30,W+30),H+25}; else if(side<3) p={-25,rnd(-20,H+20)}; else p={W+25,rnd(-20,H+20)};
        int roll=rndi(0,99);
        Enemy e; e.p=p;
        if(wave>=4 && roll<14){ e.type=1; e.hp=e.maxhp=4+wave/4; e.r=18; e.value=450; }
        else if(wave>=3 && roll<32){ e.type=2; e.hp=e.maxhp=1; e.r=8; e.value=180; }
        else { e.type=0; e.hp=e.maxhp=1; e.r=12; e.value=100; }
        e.v={0,0}; enemies.push_back(e);
    }

    void killEnemy(size_t i){
        Enemy e=enemies[i];
        int mult=1 + int(combo/5);
        score += e.value*mult; kills++; combo++; comboTimer=2.4;
        if(rndi(0,99)<8){ Pickup p; p.p=e.p; p.type=rndi(0,1); pickups.push_back(p); }
        unsigned long c=0; burst(e.p, c, 16, e.type==1?150:110);
        enemies.erase(enemies.begin()+i);
    }

    void hurt(){
        if(over) return;
        hp--; combo=0; comboTimer=0; burst(player,0,22,140);
        if(hp<=0) over=true;
    }

    void update(double dt, bool up,bool down,bool left,bool right, double mx,double my,bool fire,bool dash){
        if(paused || over) return;
        time+=dt; dashCd=std::max(0.0,dashCd-dt); dashFlash=std::max(0.0,dashFlash-dt); shootCd=std::max(0.0,shootCd-dt); spawnCd-=dt; waveTime-=dt; comboTimer-=dt;
        if(comboTimer<=0) combo=0;

        Vec2 dir{double(right)-double(left),double(down)-double(up)}; dir=norm(dir);
        double speed=265;
        if(dash && dashCd<=0 && len(dir)>0.1){ player=player+dir*150; player.x=clampd(player.x,18,W-18); player.y=clampd(player.y,62,H-18); dashCd=1.0; dashFlash=0.12; burst(player,0,8,80); }
        player=player+dir*speed*dt; player.x=clampd(player.x,18,W-18); player.y=clampd(player.y,62,H-18);

        if(fire && shootCd<=0){
            Vec2 target{mx,my}; Vec2 aim=norm(target-player); if(len(aim)>0){
                bullets.push_back({player+aim*22,aim*720,1.0,4});
                shootCd=0.105;
                burst(player+aim*18,0,2,35);
            }
        }

        if(waveTime<=0){ wave++; waveTime=std::max(7.0,15.0-wave*0.35); burst(player,0,26,170); }
        int maxEnemies=8+wave*3;
        if(spawnCd<=0 && int(enemies.size())<maxEnemies){ spawnEnemy(); spawnCd=std::max(0.18,0.82-wave*0.035); }

        for(auto &e:enemies){
            Vec2 to=player-e.p; double d=len(to); double s=(e.type==1?45:(e.type==2?125:72))+wave*2.0; e.v=norm(to)*s; e.p=e.p+e.v*dt;
            if(d<e.r+14 && d>0){ e.p=e.p+norm(e.p-player)*30*dt; }
        }
        for(auto &b:bullets){ b.p=b.p+b.v*dt; b.life-=dt; }
        bullets.erase(std::remove_if(bullets.begin(),bullets.end(),[](const Bullet&b){return b.life<=0||b.p.x<-30||b.p.x>W+30||b.p.y<40||b.p.y>H+30;}),bullets.end());

        for(size_t i=0;i<bullets.size();){ bool gone=false;
            for(size_t j=0;j<enemies.size() && !gone;j++){
                double rr=bullets[i].r+enemies[j].r; Vec2 d=bullets[i].p-enemies[j].p;
                if(d.x*d.x+d.y*d.y<rr*rr){ enemies[j].hp--; burst(bullets[i].p,0,4,70); bullets.erase(bullets.begin()+i); gone=true; if(enemies[j].hp<=0) killEnemy(j); }
            }
            if(!gone)i++;
        }

        for(size_t i=0;i<enemies.size();){ Vec2 d=enemies[i].p-player; double rr=enemies[i].r+15;
            if(d.x*d.x+d.y*d.y<rr*rr){ hurt(); enemies.erase(enemies.begin()+i); } else i++; }

        for(size_t i=0;i<pickups.size();){ pickups[i].life-=dt; Vec2 d=pickups[i].p-player;
            if(len(d)<27){ if(pickups[i].type==0) hp=std::min(maxHp,hp+1); else shootCd=std::min(shootCd,0.01); burst(pickups[i].p,0,15,90); pickups.erase(pickups.begin()+i); }
            else if(pickups[i].life<=0) pickups.erase(pickups.begin()+i); else i++; }
        for(auto &p:particles){ p.p=p.p+p.v*dt; p.v=p.v*0.94; p.life-=dt; }
        particles.erase(std::remove_if(particles.begin(),particles.end(),[](const Particle&p){return p.life<=0;}),particles.end());
    }
};

struct Palette { unsigned long bg,grid,white,muted,red,red2,yellow,cyan,black; };
static unsigned long color(Display*d,const char*n){ Colormap cm=DefaultColormap(d,DefaultScreen(d)); XColor c; if(!XParseColor(d,cm,n,&c)||!XAllocColor(d,cm,&c)) return WhitePixel(d,DefaultScreen(d)); return c.pixel; }

static void rect(Display*d,Window w,GC gc,unsigned long c,int x,int y,int ww,int hh){XSetForeground(d,gc,c);XFillRectangle(d,w,gc,x,y,ww,hh);} 
static void circle(Display*d,Window w,GC gc,unsigned long c,int x,int y,int r){XSetForeground(d,gc,c);XFillArc(d,w,gc,x-r,y-r,2*r,2*r,0,360*64);} 
static void line(Display*d,Window w,GC gc,unsigned long c,int x1,int y1,int x2,int y2){XSetForeground(d,gc,c);XDrawLine(d,w,gc,x1,y1,x2,y2);} 
static void txt(Display*d,Window w,GC gc,unsigned long c,int x,int y,const std::string&s){XSetForeground(d,gc,c);XDrawString(d,w,gc,x,y,s.c_str(),(int)s.size());}
int main(){
    Display*d=XOpenDisplay(nullptr); if(!d) return 2; int screen=DefaultScreen(d);
    Window root=RootWindow(d,screen); XSetWindowAttributes wa{}; wa.background_pixel=BlackPixel(d,screen); wa.event_mask=ExposureMask|KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|FocusChangeMask|StructureNotifyMask;
    Window win=XCreateWindow(d,root,0,0,W,H,0,DefaultDepth(d,screen),InputOutput,DefaultVisual(d,screen),CWBackPixel|CWEventMask,&wa);
    XStoreName(d,win,"SHITWAVE — C++ 2D Game"); Atom wmDelete=XInternAtom(d,"WM_DELETE_WINDOW",False); XSetWMProtocols(d,win,&wmDelete,1); XMapWindow(d,win);
    GC gc=XCreateGC(d,win,0,nullptr); XFontStruct* font=XLoadQueryFont(d,"9x15"); if(font) XSetFont(d,gc,font->fid);
    Palette p{color(d,"#0d0d0d"),color(d,"#1d1d1d"),color(d,"#f2f2ed"),color(d,"#7e7e78"),color(d,"#ff3b30"),color(d,"#ff6b61"),color(d,"#ffd166"),color(d,"#55d6be"),color(d,"#050505")};
    bool keys[4]={},fire=false,dash=false; double mx=W/2,my=H/2; Game g;
    auto keyIndex=[&](KeySym k)->int{ if(k==XK_w||k==XK_Up)return 0; if(k==XK_s||k==XK_Down)return 1; if(k==XK_a||k==XK_Left)return 2; if(k==XK_d||k==XK_Right)return 3; return -1; };
    timespec last{}; clock_gettime(CLOCK_MONOTONIC,&last); bool running=true;
    while(running){
        while(XPending(d)){ XEvent e; XNextEvent(d,&e);
            if(e.type==ClientMessage && (Atom)e.xclient.data.l[0]==wmDelete) running=false;
            else if(e.type==FocusOut){ for(bool&k:keys)k=false; fire=dash=false;}
                        else if(e.type==KeyPress){ KeySym k=XLookupKeysym(&e.xkey,0); int i=keyIndex(k); if(i>=0)keys[i]=true; if(k==XK_Escape) running=false; if(k==XK_p||k==XK_P) g.paused=!g.paused; if(k==XK_r||k==XK_R){if(g.over)g.reset();} if(k==XK_space) dash=true; }
            else if(e.type==KeyRelease){ if(XEventsQueued(d,QueuedAfterReading)>0){XEvent ne; XPeekEvent(d,&ne); if(ne.type==KeyPress && ne.xkey.keycode==e.xkey.keycode){continue;}} KeySym k=XLookupKeysym(&e.xkey,0); int i=keyIndex(k); if(i>=0)keys[i]=false; if(k==XK_space)dash=false; }
            else if(e.type==ButtonPress){if(e.xbutton.button==Button1)fire=true;}
            else if(e.type==ButtonRelease){if(e.xbutton.button==Button1)fire=false;}
            else if(e.type==MotionNotify){mx=clampd(e.xmotion.x,0,W);my=clampd(e.xmotion.y,50,H);}
        }
        timespec now{}; clock_gettime(CLOCK_MONOTONIC,&now); double dt=(now.tv_sec-last.tv_sec)+(now.tv_nsec-last.tv_nsec)/1e9; last=now; dt=clampd(dt,0,0.033);
        g.update(dt,keys[0],keys[1],keys[2],keys[3],mx,my,fire,dash);

        rect(d,win,gc,p.bg,0,0,W,H); // canvas
        for(int x=0;x<W;x+=40) line(d,win,gc,p.grid,x,56,x,H);
        for(int y=56;y<H;y+=40) line(d,win,gc,p.grid,0,y,W,y);
        rect(d,win,gc,p.black,0,0,W,56); line(d,win,gc,p.white,0,55,W,55);
        txt(d,win,gc,p.white,18,24,"SHITWAVE"); txt(d,win,gc,p.muted,18,42,"SURVIVE THE BULLSHIT");
        txt(d,win,gc,p.white,315,24,("SCORE "+std::to_string(g.score)).c_str()); txt(d,win,gc,p.red,315,42,("WAVE "+std::to_string(g.wave)).c_str());
        txt(d,win,gc,p.white,500,24,("KILLS "+std::to_string(g.kills)).c_str());
        std::string hpbar="HP "; for(int i=0;i<g.maxHp;i++)hpbar+=(i<g.hp?"■":"·"); txt(d,win,gc,p.red2,650,24,hpbar);
        txt(d,win,gc,p.muted,930,24,"WASD MOVE  •  LMB SHOOT  •  SPACE DASH  •  P PAUSE");

        // Wave meter
        int meter=(int)(std::max(0.0,std::min(1.0,g.waveTime/15.0))*250); rect(d,win,gc,p.grid,980,34,250,7); rect(d,win,gc,p.red,980,34,meter,7);

        for(const auto&q:g.pickups){ unsigned long c=q.type==0?p.red:p.cyan; circle(d,win,gc,c,(int)q.p.x,(int)q.p.y,9); circle(d,win,gc,p.bg,(int)q.p.x,(int)q.p.y,4); if(q.type==0)txt(d,win,gc,p.white,(int)q.p.x-3,(int)q.p.y+4,"+"); else txt(d,win,gc,p.white,(int)q.p.x-3,(int)q.p.y+4,"R"); }
        for(const auto&b:g.bullets) circle(d,win,gc,p.white,(int)b.p.x,(int)b.p.y,(int)b.r);
        for(const auto&e:g.enemies){ unsigned long c=e.type==1?p.yellow:(e.type==2?p.cyan:p.red); circle(d,win,gc,c,(int)e.p.x,(int)e.p.y,(int)e.r); circle(d,win,gc,p.bg,(int)e.p.x,(int)e.p.y,std::max(2,(int)e.r-5)); }
        for(const auto&q:g.particles){ double a=clampd(q.life/q.maxLife,0,1); int rr=std::max(1,(int)(q.r*a)); circle(d,win,gc,q.color ? q.color : p.white,(int)q.p.x,(int)q.p.y,rr); }
        // player
        circle(d,win,gc,p.white,(int)g.player.x,(int)g.player.y,15); circle(d,win,gc,p.bg,(int)g.player.x,(int)g.player.y,8);
        double ang=std::atan2(my-g.player.y,mx-g.player.x); line(d,win,gc,p.white,(int)g.player.x,(int)g.player.y,(int)(g.player.x+std::cos(ang)*28),(int)(g.player.y+std::sin(ang)*28));
        if(g.dashFlash>0) circle(d,win,gc,p.cyan,(int)g.player.x,(int)g.player.y,22);
        if(g.combo>=2) txt(d,win,gc,p.yellow,(int)g.player.x-35,(int)g.player.y-28,"x"+std::to_string((int)g.combo));

        if(!g.over && !g.paused && g.time<4){ txt(d,win,gc,p.white,460,340,"MOVE. SHOOT. DON'T GET FUCKED."); txt(d,win,gc,p.muted,490,364,"your mouse is the barrel"); }
        if(g.paused){ rect(d,win,gc,p.black,360,250,560,190); line(d,win,gc,p.red,360,250,920,250); txt(d,win,gc,p.white,570,300,"PAUSED"); txt(d,win,gc,p.muted,470,338,"P to unpause • ESC to bail"); }
        if(g.over){ rect(d,win,gc,p.black,300,210,680,300); line(d,win,gc,p.red,300,210,980,210); txt(d,win,gc,p.red,465,270,"YOU FUCKED UP."); txt(d,win,gc,p.white,472,315,"SCORE  "+std::to_string(g.score)); txt(d,win,gc,p.white,472,342,"WAVE   "+std::to_string(g.wave)); txt(d,win,gc,p.white,472,369,"KILLS  "+std::to_string(g.kills)); txt(d,win,gc,p.muted,425,420,"R = run it back    ESC = quit"); }

        XFlush(d); usleep(16000);
    }
    if(font) XFreeFont(d,font);
    XFreeGC(d,gc);
    XDestroyWindow(d,win);
    XCloseDisplay(d);
    return 0;
}
