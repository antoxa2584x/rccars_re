/* Which suspension branch does each wheel take, at 1 deg vs 2 deg? */
#include "rb.h"
#include "rbcar.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
extern int rb_dbg_susp;
extern const char *rb_dbg_path[RB_MAX_WHEELS];
static float PN[3];
static int w_sphere(void*c_,const float c[3],float r,rb_world_hit*h,int mx,int*n){
    (void)c_;(void)mx;*n=0;
    double d=(double)c[0]*PN[0]+(double)c[1]*PN[1]+(double)c[2]*PN[2];
    if(fabs(d)>r) return 0;
    h[0].point[0]=(float)(c[0]-d*PN[0]);h[0].point[1]=(float)(c[1]-d*PN[1]);
    h[0].point[2]=(float)(c[2]-d*PN[2]);h[0].surface=0;*n=1;return 1;}
static int w_seg(void*c_,const float a[3],const float b[3]){(void)c_;
    double da=(double)a[0]*PN[0]+(double)a[1]*PN[1]+(double)a[2]*PN[2];
    double db=(double)b[0]*PN[0]+(double)b[1]*PN[1]+(double)b[2]*PN[2];
    return (da>0.0)!=(db>0.0);}
static int w_wat(void*c_,int w,float*g){(void)c_;(void)w;(void)g;return 0;}
static int w_gnd(void*c_,float x,float z,float*y,float n[3]){(void)c_;
    if(fabsf(PN[1])<1e-6f) return 0;
    *y=-(PN[0]*x+PN[2]*z)/PN[1];n[0]=PN[0];n[1]=PN[1];n[2]=PN[2];return 1;}
static const rb_world W={w_sphere,w_seg,w_wat,w_gnd,0};

static void run(float deg,int frames){
    double r=deg*0.017453292;
    PN[0]=(float)sin(r);PN[1]=(float)cos(r);PN[2]=0.f;
    rb_car c; rbcar_init(&c,0,&W,0.f,0.f,0.f,0.f);
    rb_dbg_susp=1;
    printf("=== slope %.2f deg ===\n",deg);
    for(int k=0;k<frames;k++){
        for(int i=0;i<RB_MAX_WHEELS;i++) rb_dbg_path[i]=".";
        rbcar_step(&c,0.f,0.f,0.f,0,1.f/60.f);
        const float*up=&c.m[4];
        double dn=(double)up[0]*PN[0]+(double)up[1]*PN[1]+(double)up[2]*PN[2];
        double e=acos(fmax(-1.0,fmin(1.0,dn)))*57.295776;
        double w=sqrt((double)c.body.w[0]*c.body.w[0]+(double)c.body.w[1]*c.body.w[1]+(double)c.body.w[2]*c.body.w[2]);
        if(k<16)
            printf("%3d err=%6.2f |w|=%5.2f act=%d%d%d%d len=%5.1f %5.1f %5.1f %5.1f\n",
                k,e,w,c.hit[0].active,c.hit[1].active,c.hit[2].active,c.hit[3].active,
                c.wheel[0].len*1000,c.wheel[1].len*1000,c.wheel[2].len*1000,c.wheel[3].len*1000);
    }
}
int main(void){ run(1.0f,16); printf("\n"); run(2.0f,16); return 0; }
