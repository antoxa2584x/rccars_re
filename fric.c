/* Flat ground, car given a pure sideways velocity: does lateral grip bite? */
#include "rb.h"
#include "rb_data.h"
#include "rbcar.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static int w_sphere(void *ctx,const float c[3],float r,rb_world_hit *h,int mx,int *n){
    (void)ctx;(void)mx;*n=0; if(fabsf(c[1])>r) return 0;
    h[0].point[0]=c[0];h[0].point[1]=0.f;h[0].point[2]=c[2];h[0].surface=0;*n=1;return 1;}
static int w_seg(void *ctx,const float a[3],const float b[3]){(void)ctx;return (a[1]>0.f)!=(b[1]>0.f);}
static int w_wat(void *ctx,int w,float *g){(void)ctx;(void)w;(void)g;return 0;}
static const rb_world W={w_sphere,w_seg,w_wat,0};
static void car_init(rb_car *c){
    rbcar_init(c, 0, &W, 0.f, 0.f, 0.f, 0.f);
}
int main(void){
    rb_car c; int i; float y[RB_STATE_N];
    car_init(&c);
    for(i=0;i<120;i++) rb_car_tick(&c,1.f/600.f);
    /* pure lateral (world X) velocity of 3 m/s; car faces +Z */
    rb_car_get_state(&c,y); y[7]=3.0f*c.body.mass; rb_car_set_state(&c,y);
    printf("start: vx=%.3f vz=%.3f\n", c.body.v[0], c.body.v[2]);
    for(i=0;i<180;i++){
        rb_car_tick(&c,1.f/60.f);
        if(i%30==0){
            /* what did the friction solve actually contribute? */
            float pts[RB_MAX_FORCES][3], f[RB_MAX_FORCES][3]; int n=0;
            int r = rb_contact_solve(&c,1.f/60.f,&n,pts,f);
            double fx=0,fz=0; int k;
            for(k=0;k<n;k++){fx+=f[k][0];fz+=f[k][2];}
            printf("%3d vx=%+.4f vz=%+.4f x=%+.3f | solve=%d nforces=%d sumF=(%.3f,%.3f) act=%d%d%d%d\n",
                   i,c.body.v[0],c.body.v[2],c.body.x[0],r,n,fx,fz,
                   c.hit[0].active,c.hit[1].active,c.hit[2].active,c.hit[3].active);
        }
    }
    return 0;
}
