#include "col.h"
#include "rbcar.h"
#include <math.h>
#include <stdio.h>
#define SPAWN_X 1.09f
#define SPAWN_Z (-31.01f)
int main(void){
    col_t col; rb_car c; int i;
    col_load("/mnt/c/tools/rccars_vita/assets/beach_1.col",&col);
    float gy,n0,n1,n2; col_ground_at(&col,SPAWN_X,SPAWN_Z,1e30f,&gy,&n0,&n1,&n2);
    rbcar_init(&c,0,col_rb_world(&col),SPAWN_X,gy,SPAWN_Z,0.f);
    for(i=0;i<c.nwheels;i++){
        float p[3],r,up[3]={0,1,0},far[3]; int k;
        rb_wheel_frame(&c,i,0,p,&r,0,0);
        for(k=0;k<3;k++) far[k]=up[k]*(r*1.5f)+p[k];
        int seg = col_segment(&col,p,far);
        rb_world_hit h[8]; int nh=0;
        int sph = col_sphere(&col,far,r*1.5f,h,8,&nh);
        /* how deep is the wheel itself? */
        rb_world_hit h2[8]; int nh2=0;
        int own = col_sphere(&col,p,r,h2,8,&nh2);
        double d=0; if(nh2){ double dx=p[0]-h2[0].point[0],dy=p[1]-h2[0].point[1],dz=p[2]-h2[0].point[2];
                             d=r-sqrt(dx*dx+dy*dy+dz*dz); }
        printf("w%d centre=(%.3f %.3f %.3f) r=%.4f | upSeg=%d fatSphere=%d(n=%d) ownHit=%d overlap=%+.5f\n",
               i,p[0],p[1],p[2],r,seg,sph,nh,own,d);
        if(sph&&nh) printf("     fat hit pt=(%.3f %.3f %.3f) dist=%.4f\n",
             h[0].point[0],h[0].point[1],h[0].point[2],
             sqrt(pow(far[0]-h[0].point[0],2)+pow(far[1]-h[0].point[1],2)+pow(far[2]-h[0].point[2],2)));
    }
    return 0;
}
