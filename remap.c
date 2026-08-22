/* ===== FUN_0040ba70 @ 0040ba70  size=59 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_0040ba70(float param_1,float param_2,float param_3,float param_4,float param_5)

{
  if (ABS((float10)param_4 - (float10)param_3) < (float10)_DAT_00554380) {
    return (float10)_DAT_005543a0;
  }
  return (((float10)param_5 - (float10)param_3) * ((float10)param_2 - (float10)param_1)) /
         ((float10)param_4 - (float10)param_3) + (float10)param_1;
}



