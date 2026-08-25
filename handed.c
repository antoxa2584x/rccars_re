/* Stick right: which way does the car actually turn? */
#include "rb.h"
#include "rbcar.h"
#include <math.h>
#include <stdio.h>
static int w_sphere(void*c_,const float c[3],float r,rb_world_hit*h,int mx,int*n){
    (void)c_;(void)mx;*n=0; if(fabsf(c[1])>r) return 0;
    h[0].point[0]=c[0];h[0].point[1]=0;h[0].point[2]=c[2];h[0].surface=0;*n=1;return 1;}
static int w_seg(void*c_,const float a[3],const float b[3]){(void)c_;return (a[1]>0.f)!=(b[1]>0.f);}
static int w_wat(void*c_,int w,const float*ct,float*g){(void)c_;(void)w;(void)ct;(void)g;return 0;}
static int w_gnd(void*c_,float x,float z,float cy,float*y,float n[3]){(void)c_;(void)cy;(void)x;(void)z;
    *y=0;n[0]=0;n[1]=1;n[2]=0;return 1;}
static const rb_world W={w_sphere,w_seg,w_wat,w_gnd,0};
int main(void){
    rb_car c; int i;
    /* app yaw convention: forward = (sin yaw, 0, -cos yaw).
       yaw 0 -> -Z, yaw 90 -> +X. Seen from above with +X screen-right and
       -Z screen-up, INCREASING yaw sweeps up->right, i.e. a RIGHT turn. */
    printf("mesh: LEFT is +X (WHEEL_FRONT_LEFT x=+0.141), RIGHT is -X\n");
    printf("body: wheel[0] mount x=%%s, so index 0 sits on the mesh's %%s\n");
    rbcar_init(&c,0,&W,0.f,0.f,0.f,0.f);
    printf("body: wheel[0] mount x=%+.3f -> mesh %s side\n",
           c.wheel[0].mount[0], c.wheel[0].mount[0]>0?"LEFT":"RIGHT");
    for(int t=0;t<2;t++){
        float st = t? -1.0f : +1.0f;
        rbcar_init(&c,0,&W,0.f,0.f,0.f,0.f);
        float y0=0.f;
        for(i=0;i<240;i++){
            rbcar_step(&c,0.6f,0.f,st,0,1.f/60.f);
            if(i==60) y0=rbcar_yaw_deg(&c);
        }
        float y1=rbcar_yaw_deg(&c);
        float d=y1-y0; while(d>180)d-=360; while(d<-180)d+=360;
        printf("steer input %+.1f -> app yaw %+.1f deg over 3 s -> turns %s"
               "   (body steer angle %+.1f deg)\n",
               st,d,d>0?"RIGHT":"LEFT",c.steer);
    }
    return 0;
}
