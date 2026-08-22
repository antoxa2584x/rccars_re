/* Find spots where the ground under the car's four wheel positions is flat. */
#include "col.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define HT 0.141f      /* half-track */
#define HB 0.149f      /* half-wheelbase */

int main(void){
    col_t col;
    if(!col_load("/mnt/c/tools/rccars_vita/assets/beach_1.col",&col)){printf("no col\n");return 1;}
    printf("grid %ux%u cell=%.2f min=(%.2f,%.2f)\n",col.nx,col.nz,col.cell,col.minx,col.minz);

    typedef struct { float x,z,y,spread,ny; } cand_t;
    cand_t best[8];
    int nb=0;
    for(int i=0;i<8;i++){best[i].spread=1e30f;}

    float x0=col.minx, z0=col.minz;
    float x1=col.minx+col.nx*col.cell, z1=col.minz+col.nz*col.cell;
    for(float x=x0;x<x1;x+=0.5f) for(float z=z0;z<z1;z+=0.5f){
        float gy,n0,n1,n2;
        if(!col_ground_at(&col,x,z,1e30f,&gy,&n0,&n1,&n2)) continue;
        if(gy < 0.5f) continue;                 /* skip sea level / water plane */
        if(n1 < 0.995f) continue;               /* within ~5.7 deg of level */
        float lo=1e30f,hi=-1e30f; int ok=1;
        const float wx[4]={-HT,HT,-HT,HT}, wz[4]={HB,HB,-HB,-HB};
        for(int k=0;k<4;k++){
            float wy,m0,m1,m2;
            if(!col_ground_at(&col,x+wx[k],z+wz[k],1e30f,&wy,&m0,&m1,&m2)){ok=0;break;}
            if(wy<lo)lo=wy; if(wy>hi)hi=wy;
        }
        if(!ok) continue;
        float spread=hi-lo;
        /* keep the flattest few, spread out from each other */
        int dup=0;
        for(int i=0;i<nb;i++){
            if(fabsf(best[i].x-x)<8.f && fabsf(best[i].z-z)<8.f){
                dup=1;
                if(spread<best[i].spread){best[i].x=x;best[i].z=z;best[i].y=gy;best[i].spread=spread;best[i].ny=n1;}
                break;
            }
        }
        if(dup) continue;
        if(nb<8){best[nb].x=x;best[nb].z=z;best[nb].y=gy;best[nb].spread=spread;best[nb].ny=n1;nb++;}
        else{
            int worst=0; for(int i=1;i<nb;i++) if(best[i].spread>best[worst].spread) worst=i;
            if(spread<best[worst].spread){best[worst].x=x;best[worst].z=z;best[worst].y=gy;best[worst].spread=spread;best[worst].ny=n1;}
        }
    }
    /* sort by spread */
    for(int i=0;i<nb;i++)for(int j=i+1;j<nb;j++)
        if(best[j].spread<best[i].spread){ cand_t t=best[i];best[i]=best[j];best[j]=t;}
    printf("flattest candidates (wheel-span height spread, suspension travel is 0.098):\n");
    for(int i=0;i<nb;i++)
        printf("  %d: (%8.2f, %8.3f, %8.2f)  spread=%.4f m  ny=%.4f\n",
               i,best[i].x,best[i].y,best[i].z,best[i].spread,best[i].ny);
    return 0;
}
