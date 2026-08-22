/* Tilted plane: does the body take up the slope attitude, and does it hold? */
#include "rb.h"
#include "rb_data.h"
#include "rbcar.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float TILT_DEG = 10.0f;
static float PN[3];   /* plane normal */

static void plane_init(float deg){
    double r = deg*0.017453292;
    PN[0] = (float)sin(r); PN[1] = (float)cos(r); PN[2] = 0.0f;
    TILT_DEG = deg;
}
/* plane through the origin with normal PN */
static int w_sphere(void *ctx,const float c[3],float r,rb_world_hit *h,int mx,int *n){
    (void)ctx;(void)mx; *n=0;
    double d = (double)c[0]*PN[0] + (double)c[1]*PN[1] + (double)c[2]*PN[2];
    if (fabs(d) > r) return 0;
    h[0].point[0]=(float)(c[0]-d*PN[0]);
    h[0].point[1]=(float)(c[1]-d*PN[1]);
    h[0].point[2]=(float)(c[2]-d*PN[2]);
    h[0].surface=0; *n=1; return 1;
}
static int w_seg(void *ctx,const float a[3],const float b[3]){
    (void)ctx;
    double da=(double)a[0]*PN[0]+(double)a[1]*PN[1]+(double)a[2]*PN[2];
    double db=(double)b[0]*PN[0]+(double)b[1]*PN[1]+(double)b[2]*PN[2];
    return (da>0.0)!=(db>0.0);
}
static int w_wat(void *ctx,int w,float *g){(void)ctx;(void)w;(void)g;return 0;}
/* plane through the origin: y such that the point is on the plane */
static int w_ground(void *ctx,float x,float z,float *y,float n[3]){
    (void)ctx;
    /* PN.x*x + PN.y*y + PN.z*z = 0  ->  y = -(PN.x*x + PN.z*z)/PN.y */
    if (fabsf(PN[1]) < 1e-6f) return 0;
    *y = -(PN[0]*x + PN[2]*z)/PN[1];
    n[0]=PN[0]; n[1]=PN[1]; n[2]=PN[2];
    return 1;
}
static const rb_world W={w_sphere,w_seg,w_wat,w_ground,0};

static void car_init(rb_car *c){
    rbcar_init(c, 0, &W, 0.f, 0.f, 0.f, 0.f);
}

int main(void){
    rb_car c; int i;
    for (int t = 0; t < 2; t++) {
        float deg = t ? 10.0f : 0.0f;
        plane_init(deg);
        car_init(&c);
        for(i=0;i<5*60;i++) rb_car_tick(&c,1.0f/60.0f);
        {
            /* body up axis vs world up, and vs the plane normal */
            const float *up=&c.m[4];
            double au = acos(fmax(-1.0,fmin(1.0,(double)up[1])))*57.295776;
            double an = acos(fmax(-1.0,fmin(1.0,(double)up[0]*PN[0]+(double)up[1]*PN[1]+(double)up[2]*PN[2])))*57.295776;
            double sp = sqrt((double)c.body.v[0]*c.body.v[0]+(double)c.body.v[1]*c.body.v[1]+(double)c.body.v[2]*c.body.v[2]);
            printf("tilt %4.1f deg: body-up vs world-up = %5.2f deg, vs plane normal = %5.2f deg, speed=%.4f, slid x=%+.3f\n",
                   deg, au, an, sp, c.body.x[0]);
            printf("   wheel len: %.4f %.4f %.4f %.4f   active: %d%d%d%d\n",
                   c.wheel[0].len,c.wheel[1].len,c.wheel[2].len,c.wheel[3].len,
                   c.hit[0].active,c.hit[1].active,c.hit[2].active,c.hit[3].active);
        }
    }
    return 0;
}
