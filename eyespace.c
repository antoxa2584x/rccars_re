/* Build the EXACT GL view matrix main.c produces and see where the car lands. */
#include "rb.h"
#include "rbcar.h"
#include "cam.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define DEG (float)(M_PI/180.0)
static int w_sphere(void*c_,const float c[3],float r,rb_world_hit*h,int mx,int*n){
    (void)c_;(void)mx;*n=0; if(fabsf(c[1])>r) return 0;
    h[0].point[0]=c[0];h[0].point[1]=0;h[0].point[2]=c[2];h[0].surface=0;*n=1;return 1;}
static int w_seg(void*c_,const float a[3],const float b[3]){(void)c_;return (a[1]>0.f)!=(b[1]>0.f);}
static int w_wat(void*c_,int w,float*g){(void)c_;(void)w;(void)g;return 0;}
static int w_gnd(void*c_,float x,float z,float*y,float n[3]){(void)c_;(void)x;(void)z;
    *y=0;n[0]=0;n[1]=1;n[2]=0;return 1;}
static const rb_world W={w_sphere,w_seg,w_wat,w_gnd,0};

/* column-major 4x4, GL style */
static void ident(float*m){memset(m,0,64);m[0]=m[5]=m[10]=m[15]=1;}
static void mul(float*o,const float*a,const float*b){ /* o = a*b */
    float t[16];
    for(int c=0;c<4;c++)for(int r=0;r<4;r++){
        float s=0; for(int k=0;k<4;k++) s+=a[k*4+r]*b[c*4+k];
        t[c*4+r]=s;}
    memcpy(o,t,64);}
static void rotY(float*m,float deg){ident(m);float c=cosf(deg*DEG),s=sinf(deg*DEG);
    m[0]=c;m[8]=s;m[2]=-s;m[10]=c;}
static void rotX(float*m,float deg){ident(m);float c=cosf(deg*DEG),s=sinf(deg*DEG);
    m[5]=c;m[9]=-s;m[6]=s;m[10]=c;}
static void trans(float*m,float x,float y,float z){ident(m);m[12]=x;m[13]=y;m[14]=z;}
static void xform(float*o,const float*m,const float*p){
    for(int r=0;r<3;r++) o[r]=m[0*4+r]*p[0]+m[1*4+r]*p[1]+m[2*4+r]*p[2]+m[3*4+r];}

int main(void){
    rb_car c; cam_t cm; int i;
    rbcar_init(&c,0,&W,0.f,0.f,0.f,0.f);
    cam_init(&cm,&c);
    printf(" turn   car eye-space (x,y,z)      right-marker eye x   verdict\n");
    for(i=0;i<300;i++){
        float st=(i<60)?0.f:1.0f;                     /* stick right */
        rbcar_step(&c,0.6f,0.f,st,0,1.f/60.f);
        cam_update(&cm,&c,st,1.f/60.f);
        if(i%60!=0) continue;
        {
            float h=cm.yaw*DEG;
            float ex=c.body.x[0]+sinf(h)*cm.dist;
            float ez=c.body.x[2]+cosf(h)*cm.dist;
            float ey=c.body.x[1]+cm.height;
            float vyaw=cm.yaw, vpitch=cam_pitch_deg(&cm);
            float V[16],A[16],B[16],C[16];
            rotX(A,vpitch); rotY(B,-vyaw); trans(C,-ex,-ey,-ez);
            ident(V); mul(V,A,B); mul(V,V,C);
            /* car centre, and a marker 1 m off the car's RIGHT (body -X) */
            float pc[3]={c.body.x[0],c.body.x[1],c.body.x[2]};
            float rmark[3];
            for(int k=0;k<3;k++) rmark[k]=pc[k]-c.m[0+k]*1.0f;  /* -X = right */
            float ec[3],er[3];
            xform(ec,V,pc); xform(er,V,rmark);
            printf("%5.1f  (%+7.3f %+7.3f %+7.3f)   %+8.3f          %s / %s\n",
                st,ec[0],ec[1],ec[2],er[0],
                ec[2]<0?"in front":"BEHIND CAMERA",
                er[0]>ec[0]?"right on screen-right":"RIGHT ON SCREEN-LEFT");
        }
    }
    return 0;
}
