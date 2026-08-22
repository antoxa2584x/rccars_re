#include "col.h"
#include "rbcar.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define SPAWN_X 1.09f
#define SPAWN_Z (-31.01f)
int main(void){
    col_t col; rb_car c; int i;
    col_load("/mnt/c/tools/rccars_vita/assets/beach_1.col",&col);
    float gy,n0,n1,n2; col_ground_at(&col,SPAWN_X,SPAWN_Z,1e30f,&gy,&n0,&n1,&n2);
    rbcar_init(&c,0,col_rb_world(&col),SPAWN_X,gy,SPAWN_Z,0.f);
    printf("Ibody_inv diag = %.2f %.2f %.2f  (Ixx=%.5f Iyy=%.5f Izz=%.5f)\n",
        c.body.ibody_inv[0],c.body.ibody_inv[5],c.body.ibody_inv[10],
        1/c.body.ibody_inv[0],1/c.body.ibody_inv[5],1/c.body.ibody_inv[10]);
    for(i=0;i<12;i++){
        float tau[3], fs[3];
        rb_car_update_suspension(&c,1.f/60.f,0,0);
        rb_collide(&c,0.f,RB_CONTACT_TOL,0,-1,c.hit);
        rb_car_susp_torque(&c,1.f/60.f,tau);
        rb_car_susp_force(&c,1.f/60.f,fs);
        double tm=sqrt((double)tau[0]*tau[0]+(double)tau[1]*tau[1]+(double)tau[2]*tau[2]);
        double fm=sqrt((double)fs[0]*fs[0]+(double)fs[1]*fs[1]+(double)fs[2]*fs[2]);
        /* angular accel this torque implies */
        float al[3]; rb_mat3_mul_vec3(c.body.iinv,tau,al);
        double am=sqrt((double)al[0]*al[0]+(double)al[1]*al[1]+(double)al[2]*al[2]);
        double wm=sqrt((double)c.body.w[0]*c.body.w[0]+(double)c.body.w[1]*c.body.w[1]+(double)c.body.w[2]*c.body.w[2]);
        printf("%2d |tau|=%8.3f Nm |F_susp|=%7.2f N -> |alpha|=%9.1f rad/s^2  |w|=%7.2f rad/s  len=%3d %3d %3d %3d\n",
            i,tm,fm,am,wm,
            (int)(c.wheel[0].len*1000),(int)(c.wheel[1].len*1000),
            (int)(c.wheel[2].len*1000),(int)(c.wheel[3].len*1000));
        rb_car_tick(&c,1.f/60.f);
    }
    return 0;
}
