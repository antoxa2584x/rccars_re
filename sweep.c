/* Tilt sweep on a perfect plane: at what slope does it lose stability? */
#include "rb.h"
#include "rbcar.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
int main(int argc,char**argv){
    float dt = (argc>1)?(float)atof(argv[1]):(1.f/60.f);
    int nsteps = (int)(10.f/dt);
    printf("dt=%.5f (%.0f Hz)\ntilt   worst tilt-err   worst |w|   verdict\n",dt,1.f/dt);
    for(int i=0;i<9;i++){
        float deg[9]={0.f,0.25f,0.5f,1.f,2.f,3.f,5.f,10.f,20.f};
        double r=deg[i]*0.017453292;
        PN[0]=(float)sin(r);PN[1]=(float)cos(r);PN[2]=0.f;
        rb_car c; rbcar_init(&c,0,&W,0.f,0.f,0.f,0.f);
        float werr=0.f,wmax=0.f,ferr=0.f,fw=0.f; int fn=0;
        for(int k=0;k<nsteps;k++){
            rbcar_step(&c,0.f,0.f,0.f,0,dt);
            const float*up=&c.m[4];
            double dn=(double)up[0]*PN[0]+(double)up[1]*PN[1]+(double)up[2]*PN[2];
            double e=acos(fmax(-1.0,fmin(1.0,dn)))*57.295776;
            double w=sqrt((double)c.body.w[0]*c.body.w[0]+(double)c.body.w[1]*c.body.w[1]+(double)c.body.w[2]*c.body.w[2]);
            if(e>werr)werr=(float)e; if(w>wmax)wmax=(float)w;
            if(k > nsteps-(int)(2.f/dt)){ ferr+=(float)e; fw+=(float)w; fn++; }
        }
        if(fn){ferr/=fn;fw/=fn;}
        printf("%5.2f   worst %6.2f deg /%6.2f    last 2s avg %6.2f deg /%6.2f   %s\n",
               deg[i],werr,wmax,ferr,fw,
               (ferr<3.f&&fw<1.f)?"settles":(ferr<werr*0.8f?"decaying":"LIMIT CYCLE"));
    }
    return 0;
}
