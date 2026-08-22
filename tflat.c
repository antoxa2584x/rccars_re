/* Transcribed model on a verified-flat patch of the real track. */
#include "col.h"
#include "rbcar.h"
#include "cam.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
    col_t col; rb_car c; cam_t cm; int i;
    float sx = (argc>1)?(float)atof(argv[1]):-1.64f;
    float sz = (argc>2)?(float)atof(argv[2]):40.37f;
    float steer = (argc>3)?(float)atof(argv[3]):0.f;
    float thr0  = (argc>4)?(float)atof(argv[4]):1.f;
    col_load("/mnt/c/tools/rccars_vita/assets/beach_1.col",&col);
    float gy,n0,n1,n2;
    if(!col_ground_at(&col,sx,sz,1e30f,&gy,&n0,&n1,&n2)){printf("no ground\n");return 1;}
    rbcar_init(&c,0,col_rb_world(&col),sx,gy,sz,0.f);
    cam_init(&cm,&c);
    { const char*v=getenv("RB_V0");
      if(v){ float y[RB_STATE_N]; rb_car_get_state(&c,y);
             y[9]=(float)atof(v)*c.body.mass; rb_car_set_state(&c,y);
             printf("initial vz=%.2f m/s\n",(float)atof(v)); } }
    printf("spawn (%.2f, %.3f, %.2f) normal=(%.3f %.3f %.3f) body y=%.4f steer=%.2f\n",
           sx,gy,sz,n0,n1,n2,c.body.x[1],steer);
    float wmax=0.f, tmax=0.f;
    for(i=0;i<900;i++){
        float thr = (i>60)?thr0:0.0f;
        rbcar_step(&c,thr,0.f,steer,0,1.0f/60.0f);
        cam_update(&cm,&c,steer,1.0f/60.0f);
        {
            const float*up=&c.m[4];
            double tilt=acos(fmax(-1.0,fmin(1.0,(double)up[1])))*57.295776;
            double w=sqrt((double)c.body.w[0]*c.body.w[0]+(double)c.body.w[1]*c.body.w[1]+(double)c.body.w[2]*c.body.w[2]);
            if(w>wmax)wmax=w; if(tilt>tmax)tmax=tilt;
            if(i%100==0||i==899)
              printf("%3d pos=(%7.2f %7.3f %7.2f) sp=%5.2f tilt=%6.2f |w|=%5.2f act=%d%d%d%d len=%3d %3d %3d %3d cam=%.2f\n",
                i,c.body.x[0],c.body.x[1],c.body.x[2],
                sqrt((double)c.body.v[0]*c.body.v[0]+(double)c.body.v[1]*c.body.v[1]+(double)c.body.v[2]*c.body.v[2]),
                tilt,w,c.hit[0].active,c.hit[1].active,c.hit[2].active,c.hit[3].active,
                (int)(c.wheel[0].len*1000),(int)(c.wheel[1].len*1000),
                (int)(c.wheel[2].len*1000),(int)(c.wheel[3].len*1000),cm.dist);
        }
    }
    printf("  thr=%.2f worst tilt=%.2f deg  worst |w|=%.2f rad/s\n",thr0,tmax,wmax);
    return 0;
}
