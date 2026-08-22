/* Same track, but with the app's variable dt. */
#include "col.h"
#include "rbcar.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define SPAWN_X 1.09f
#define SPAWN_Z (-31.01f)
static unsigned rng=12345u;
static float frand(void){ rng=rng*1103515245u+12345u; return (float)((rng>>16)&0x7fff)/32767.f; }
int main(int argc,char**argv){
    col_t col; rb_car c; int i;
    float base = (argc>1)?(float)atof(argv[1]):(1.f/60.f);
    int jitter = (argc>2)?atoi(argv[2]):0;
    col_load("/mnt/c/tools/rccars_vita/assets/beach_1.col",&col);
    float gy,n0,n1,n2; col_ground_at(&col,SPAWN_X,SPAWN_Z,1e30f,&gy,&n0,&n1,&n2);
    rbcar_init(&c,0,col_rb_world(&col),SPAWN_X,gy,SPAWN_Z,0.f);
    printf("dt base=%.4f jitter=%d  spawn y=%.3f\n",base,jitter,c.body.x[1]);
    float ymin=1e30f,ymax=-1e30f;
    for(i=0;i<600;i++){
        float dt = base*(jitter? (0.5f+1.5f*frand()) : 1.f);
        if(dt>0.1f) dt=0.1f;
        {
            float y_before=c.body.x[1];
            int stuck = rb_car_update_suspension(&c,dt,0,0);
            float lim = 0.f;
            /* what does the tick actually consume? */
            float used;
            rbcar_step(&c,1.0f,0.f,0.f,0,dt);
            used = 0.f; (void)used; (void)lim;
            if(i<6) printf("   f%d dt=%.5f stuckPre=%d dy=%+.6f v=(%.3f %.3f %.3f) act=%d%d%d%d\n",
                i,dt,stuck,c.body.x[1]-y_before,c.body.v[0],c.body.v[1],c.body.v[2],
                c.hit[0].active,c.hit[1].active,c.hit[2].active,c.hit[3].active);
        }
        if(c.body.x[1]<ymin) ymin=c.body.x[1];
        if(c.body.x[1]>ymax) ymax=c.body.x[1];
        if(i%15==0&&i<=300){
            const float*up=&c.m[4];
            double tilt=acos(fmax(-1.0,fmin(1.0,(double)up[1])))*57.295776;
            double sp=sqrt((double)c.body.v[0]*c.body.v[0]+(double)c.body.v[1]*c.body.v[1]+(double)c.body.v[2]*c.body.v[2]);
            printf("%3d pos=(%6.2f %7.3f %6.2f) vy=%+6.2f sp=%5.2f tilt=%6.1f act=%d%d%d%d len=%3d %3d %3d %3d\n",
              i,c.body.x[0],c.body.x[1],c.body.x[2],c.body.v[1],sp,tilt,
              c.hit[0].active,c.hit[1].active,c.hit[2].active,c.hit[3].active,
              (int)(c.wheel[0].len*1000),(int)(c.wheel[1].len*1000),
              (int)(c.wheel[2].len*1000),(int)(c.wheel[3].len*1000));
        }
    }
    printf("  y range: %.3f .. %.3f\n", ymin, ymax);
    return 0;
}
