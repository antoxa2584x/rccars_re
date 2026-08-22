/* Drive the car on the REAL beach_1.col, exactly as the app does. */
#include "col.h"
#include "rbcar.h"
#include "cam.h"
#include "rb_data.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define SPAWN_X   1.09f
#define SPAWN_Y   4.00f
#define SPAWN_Z (-31.01f)

int main(void){
    col_t col; rb_car c; cam_t cm; int i;
    if(!col_load("/mnt/c/tools/rccars_vita/assets/beach_1.col",&col)){
        printf("col_load FAILED\n"); return 1; }
    printf("col: %u tris grid %ux%u cell=%.3f min=(%.2f,%.2f)\n",
           col.ntris,col.nx,col.nz,col.cell,col.minx,col.minz);

    /* ground probe at the spawn, as main.c does */
    { float gy,n0,n1,n2;
      if(col_ground_at(&col,SPAWN_X,SPAWN_Z,1e30f,&gy,&n0,&n1,&n2))
          printf("ground at spawn: y=%.4f normal=(%.3f %.3f %.3f)\n",gy,n0,n1,n2);
      else printf("NO GROUND AT SPAWN\n");
    }
    /* sphere probe: does col_sphere see the ground at all? */
    { rb_world_hit h[8]; int nh=0; float p[3]={SPAWN_X,0,SPAWN_Z};
      float gy,n0,n1,n2; col_ground_at(&col,SPAWN_X,SPAWN_Z,1e30f,&gy,&n0,&n1,&n2);
      p[1]=gy+0.05f;
      int r=col_sphere(&col,p,0.0718f,h,8,&nh);
      printf("col_sphere at ground+0.05 r=0.0718: hit=%d n=%d", r, nh);
      if(nh) printf(" pt=(%.3f %.3f %.3f)",h[0].point[0],h[0].point[1],h[0].point[2]);
      printf("\n");
    }

    { float gy,n0,n1,n2; col_ground_at(&col,SPAWN_X,SPAWN_Z,1e30f,&gy,&n0,&n1,&n2);
      rbcar_init(&c,0,col_rb_world(&col),SPAWN_X,gy,SPAWN_Z,0.f); }
    cam_init(&cm,&c);
    printf("spawn: body y=%.4f (mount_y=%.4f len=%.4f r=%.4f)\n",
           c.body.x[1],c.wheel[0].mount[1],c.wheel[0].len,c.wheel[0].radius);

    for(i=0;i<420;i++){
        float thr = (i>120)?1.0f:0.0f;
        rbcar_step(&c,thr,0.f,0.f,0,1.0f/60.0f);
        if(i%60==0||i==119||i==121){
            const float *up=&c.m[4];
            double tilt=acos(fmax(-1.0,fmin(1.0,(double)up[1])))*57.295776;
            double sp=sqrt((double)c.body.v[0]*c.body.v[0]+(double)c.body.v[1]*c.body.v[1]+(double)c.body.v[2]*c.body.v[2]);
            printf("%3d pos=(%7.2f %7.3f %7.2f) sp=%5.2f tilt=%5.2f act=%d%d%d%d len=%.4f %.4f emb=%d\n",
                   i,c.body.x[0],c.body.x[1],c.body.x[2],sp,tilt,
                   c.hit[0].active,c.hit[1].active,c.hit[2].active,c.hit[3].active,
                   c.wheel[0].len,c.wheel[2].len,c.embedded);
        }
    }
    return 0;
}
