/* Does the camera actually point at the car? Replicates main.c's construction. */
#include "rb.h"
#include "rbcar.h"
#include "cam.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define DEG (float)(M_PI/180.0)
static float PN[3]={0,1,0};
static int w_sphere(void*c_,const float c[3],float r,rb_world_hit*h,int mx,int*n){
    (void)c_;(void)mx;*n=0;
    double d=(double)c[0]*PN[0]+(double)c[1]*PN[1]+(double)c[2]*PN[2];
    if(fabs(d)>r) return 0;
    h[0].point[0]=(float)(c[0]-d*PN[0]);h[0].point[1]=(float)(c[1]-d*PN[1]);
    h[0].point[2]=(float)(c[2]-d*PN[2]);h[0].surface=0;*n=1;return 1;}
static int w_seg(void*c_,const float a[3],const float b[3]){(void)c_;
    return (a[1]>0.f)!=(b[1]>0.f);}
static int w_wat(void*c_,int w,const float*ct,float*g){(void)c_;(void)w;(void)ct;(void)g;return 0;}
static int w_gnd(void*c_,float x,float z,float cy,float*y,float n[3]){(void)c_;(void)cy;(void)x;(void)z;
    *y=0.f;n[0]=0;n[1]=1;n[2]=0;return 1;}
static const rb_world W={w_sphere,w_seg,w_wat,w_gnd,0};

int main(void){
    rb_car c; cam_t cm; int i;
    rbcar_init(&c,0,&W,0.f,0.f,0.f,0.f);
    cam_init(&cm,&c);
    printf("frame steer  camyaw  caryaw   dist  height |  aim-error(deg)  horiz-err  pitch-err\n");
    for(i=0;i<420;i++){
        float steer = (i<120)?0.f:1.0f;        /* hard left after 2 s */
        rbcar_step(&c,0.6f,0.f,steer,0,1.f/60.f);
        cam_update(&cm,&c,steer,1.f/60.f);
        {
            /* main.c's exact construction, orbit terms at rest (they spring to 0) */
            float h=(cm.yaw)*DEG;
            float ex=c.body.x[0]-sinf(h)*cm.dist;
            float ez=c.body.x[2]+cosf(h)*cm.dist;
            float ey=c.body.x[1]+cm.height;
            float vyaw=cm.yaw, vpitch=cam_pitch_deg(&cm);
            /* the view direction the renderer produces:
               world is rotated by -vyaw about Y then vpitch about X, so the
               camera looks along -Z of that frame */
            float cy=cosf(vyaw*DEG), sy=sinf(vyaw*DEG);
            float cp=cosf(vpitch*DEG), sp=sinf(vpitch*DEG);
            float vd[3];
            vd[0]= sy*cp; vd[1]=-sp; vd[2]=-cy*cp;
            float to[3]={c.body.x[0]-ex,c.body.x[1]-ey,c.body.x[2]-ez};
            float tl=sqrtf(to[0]*to[0]+to[1]*to[1]+to[2]*to[2]);
            to[0]/=tl;to[1]/=tl;to[2]/=tl;
            float dot=vd[0]*to[0]+vd[1]*to[1]+vd[2]*to[2];
            if(dot>1.f)dot=1.f; if(dot<-1.f)dot=-1.f;
            float aim=acosf(dot)/DEG;
            /* split into horizontal and vertical components */
            float hz_v=atan2f(vd[0],-vd[2])/DEG, hz_t=atan2f(to[0],-to[2])/DEG;
            float he=hz_v-hz_t; while(he>180)he-=360; while(he<-180)he+=360;
            float pv=asinf(-vd[1])/DEG, pt=asinf(-to[1])/DEG;
            if(i%40==0||i==120||i==125||i==140)
                printf("%4d %5.1f %7.2f %7.2f %6.3f %6.3f | %10.2f %10.2f %10.2f\n",
                    i,steer,cm.yaw,rbcar_yaw_deg(&c),cm.dist,cm.height,aim,he,pv-pt);
        }
    }
    return 0;
}
