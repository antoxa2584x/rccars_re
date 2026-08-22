/* ===== FUN_0049d820 @ 0049d820  size=92  callers: carAccumForces FUN_0050d670  ===== */

void __cdecl FUN_0049d820(float *param_1,float *param_2,float param_3,float param_4,float *param_5)

{
  float10 fVar1;
  
  fVar1 = FUN_0049d7e0(*param_1,*param_2,param_3,param_4);
  *param_5 = (float)fVar1;
  fVar1 = FUN_0049d7e0(param_1[1],param_2[1],param_3,param_4);
  param_5[1] = (float)fVar1;
  fVar1 = FUN_0049d7e0(param_1[2],param_2[2],param_3,param_4);
  param_5[2] = (float)fVar1;
  return;
}


/* ===== FUN_0049d7e0 @ 0049d7e0  size=59  callers: FUN_00503880 FUN_004f3800 FUN_004f3800 FUN_004722c0 FUN_004722c0 FUN_00500a80 FUN_00500a80 FUN_004fe1f0 FUN_004fddd0 FUN_004eb670 FUN_00474240 FUN_004ef280 FUN_004ef280 FUN_004f43c0 FUN_004f6b20 FUN_004f2d60 FUN_004fb340 FUN_004fb340 FUN_004ecff0 FUN_0049d820 FUN_0049d820 FUN_0049d820 FUN_00500e60 FUN_00500e60  ===== */

float10 __cdecl FUN_0049d7e0(float param_1,float param_2,float param_3,float param_4)

{
  float10 fVar1;
  
  fVar1 = (float10)param_3 * (float10)param_4;
  if (ABS((float10)param_1 - (float10)param_2) < fVar1) {
    return (float10)param_2;
  }
  if (param_1 < param_2) {
    return fVar1 + (float10)param_1;
  }
  return (float10)param_1 - fVar1;
}


