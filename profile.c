#include "col.h"
#include <stdio.h>
#include <math.h>
int main(void){
    col_t col; col_load("/mnt/c/tools/rccars_vita/assets/beach_1.col",&col);
    printf("terrain along the driven path (x=-1.64):\n");
    for(float z=40.0f; z<=47.0f; z+=0.5f){
        float gy,n0,n1,n2;
        if(col_ground_at(&col,-1.64f,z,1e30f,&gy,&n0,&n1,&n2)){
            /* height spread under the four wheel positions here */
            float lo=1e30f,hi=-1e30f; int ok=1;
            const float wx[4]={-0.141f,0.141f,-0.141f,0.141f};
            const float wz[4]={0.149f,0.149f,-0.149f,-0.149f};
            for(int k=0;k<4;k++){
                float wy,m0,m1,m2;
                if(!col_ground_at(&col,-1.64f+wx[k],z+wz[k],1e30f,&wy,&m0,&m1,&m2)){ok=0;break;}
                if(wy<lo)lo=wy; if(wy>hi)hi=wy;
            }
            printf("  z=%5.1f  y=%7.3f  slope=%5.1f deg  wheel-span spread=%s\n",
                   z,gy,acosf(fminf(1.f,n1))*57.2958f,
                   ok?(hi-lo>0.098f?"OVER TRAVEL":"ok"):"no ground");
            if(ok) printf("           spread=%.4f m (travel is 0.098)\n",hi-lo);
        } else printf("  z=%5.1f  NO GROUND\n",z);
    }
    return 0;
}
