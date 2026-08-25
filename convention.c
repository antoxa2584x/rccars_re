/* Which way does the car actually travel, and where is the camera relative to it? */
#include "rb.h"
#include "rbcar.h"
#include "cam.h"
#include <math.h>
#include <stdio.h>
static float PN[3]={0,1,0};
static int w_sphere(void*c_,const float c[3],float r,rb_world_hit*h,int mx,int*n){
    (void)c_;(void)mx;*n=0; if(fabsf(c[1])>r) return 0;
    h[0].point[0]=c[0];h[0].point[1]=0;h[0].point[2]=c[2];h[0].surface=0;*n=1;return 1;}
static int w_seg(void*c_,const float a[3],const float b[3]){(void)c_;return (a[1]>0.f)!=(b[1]>0.f);}
static int w_wat(void*c_,int w,const float*ct,float*g){(void)c_;(void)w;(void)ct;(void)g;return 0;}
static int w_gnd(void*c_,float x,float z,float cy,float*y,float n[3]){(void)c_;(void)cy;(void)x;(void)z;
    *y=0;n[0]=0;n[1]=1;n[2]=0;return 1;}
static const rb_world W={w_sphere,w_seg,w_wat,w_gnd,0};
int main(void){
    rb_car c; cam_t cm; int i;
    rbcar_init(&c,0,&W,0.f,0.f,0.f,0.f);
    cam_init(&cm,&c);
    for(i=0;i<180;i++){ rbcar_step(&c,1.0f,0.f,0.f,0,1.f/60.f); cam_update(&cm,&c,0.f,1.f/60.f); }
    {
        const float *bz=&c.m[8];          /* body local +Z in world */
        float v[3]={c.body.v[0],c.body.v[1],c.body.v[2]};
        float vl=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
        float dotv = (v[0]*bz[0]+v[1]*bz[1]+v[2]*bz[2])/(vl>1e-6f?vl:1.f);
        /* mesh front is +Z (front wheels at z=+0.149) -> where does it point
           with and without the extra 180 about Y? */
        float nose_no180[3]  = { bz[0],  bz[1],  bz[2] };
        float nose_with180[3]= {-bz[0], -bz[1], -bz[2] };
        float toCar[3]={ c.body.x[0]-cm.pos[0], c.body.x[1]-cm.pos[1], c.body.x[2]-cm.pos[2] };
        float tl=sqrtf(toCar[0]*toCar[0]+toCar[1]*toCar[1]+toCar[2]*toCar[2]);
        printf("velocity      = (%+.3f %+.3f %+.3f)  |v|=%.3f\n",v[0],v[1],v[2],vl);
        printf("body +Z       = (%+.3f %+.3f %+.3f)\n",bz[0],bz[1],bz[2]);
        printf("  dot(v, +Z)  = %+.4f  -> car drives along %s\n",dotv,dotv>0?"+Z":"-Z");
        printf("wheel mounts  : front z=%+.3f  rear z=%+.3f  (mesh nose is +Z)\n",
               c.wheel[0].mount[2], c.wheel[2].mount[2]);
        printf("cam->car dir  = (%+.3f %+.3f %+.3f)\n",toCar[0]/tl,toCar[1]/tl,toCar[2]/tl);
        printf("  dot with +Z = %+.4f -> camera sits %s the body +Z end\n",
               (toCar[0]*bz[0]+toCar[1]*bz[1]+toCar[2]*bz[2])/tl,
               ((toCar[0]*bz[0]+toCar[2]*bz[2])/tl)>0?"BEHIND (correct)":"AHEAD OF");
        printf("\nrendered nose without 180 = (%+.3f %+.3f %+.3f)  dot(v)=%+.3f\n",
               nose_no180[0],nose_no180[1],nose_no180[2],
               (nose_no180[0]*v[0]+nose_no180[2]*v[2])/(vl>1e-6f?vl:1.f));
        printf("rendered nose WITH 180    = (%+.3f %+.3f %+.3f)  dot(v)=%+.3f\n",
               nose_with180[0],nose_with180[1],nose_with180[2],
               (nose_with180[0]*v[0]+nose_with180[2]*v[2])/(vl>1e-6f?vl:1.f));
    }
    return 0;
}
