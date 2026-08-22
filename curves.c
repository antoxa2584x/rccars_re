/* ===== FUN_00488030 @ 00488030  size=70  callers: FUN_004aa9f0 FUN_004aa9f0 FUN_004aa9f0 FUN_004aa9f0 FUN_004aa9f0 FUN_004aa9f0 FUN_00532cf0 FUN_004f74f0 FUN_004f74f0 FUN_004f74f0 physLoadEngine physLoadEngine physLoadEngine physLoadEngine physLoadEngine physLoadEngine physLoadSteering FUN_005323a0 FUN_005289f0 FUN_00529a20 FUN_0052ea60 FUN_0052ea60 FUN_005308f0  ===== */

undefined4 * __cdecl FUN_00488030(char *param_1)

{
  int iVar1;
  int iVar2;
  char *_Str1;
  
  iVar2 = 0;
  if (0 < DAT_01486520) {
    _Str1 = &DAT_01486530;
    do {
      iVar1 = __strcmpi(_Str1,param_1);
      if (iVar1 == 0) {
        return &DAT_01486528 + iVar2 * 0x12;
      }
      iVar2 = iVar2 + 1;
      _Str1 = _Str1 + 0x48;
    } while (iVar2 < DAT_01486520);
  }
  return (undefined4 *)0x0;
}


/* ===== FUN_0040f710 @ 0040f710  size=285  callers: FUN_0040f120  ===== */

float10 __cdecl FUN_0040f710(int *param_1,float param_2,undefined4 *param_3)

{
  int iVar1;
  float10 fVar2;
  
  if ((param_1 == (int *)0x0) || (*param_1 != 0x111)) goto LAB_0040f81a;
  iVar1 = FUN_0040f1e0((int)param_1,param_2);
  if (iVar1 == -3) {
    if (((param_1[1] == 1) || (param_1[1] == 0)) || (iVar1 = param_1[3], iVar1 == 1)) {
      *param_3 = 1;
      return (float10)*(float *)(param_1[5] + 8);
    }
LAB_0040f7a2:
    fVar2 = FUN_0040c6d0(param_2,*(float *)(param_1[5] + 4),
                         *(float *)((iVar1 + -1) * param_1[4] + 4 + param_1[5]));
    iVar1 = FUN_0040f1e0((int)param_1,(float)fVar2);
    if (iVar1 != -3) {
      if (iVar1 == -2) {
        iVar1 = param_1[3] + -2;
      }
      goto LAB_0040f7e1;
    }
    iVar1 = 0;
  }
  else {
    if (iVar1 == -2) {
      if (((param_1[1] == 1) || (param_1[1] == 0)) || (iVar1 = param_1[3], iVar1 == 1)) {
        *param_3 = 1;
        return (float10)*(float *)((param_1[3] + -1) * param_1[4] + 8 + param_1[5]);
      }
      goto LAB_0040f7a2;
    }
    if (iVar1 == -1) {
      *param_3 = 0;
      return (float10)0.0;
    }
LAB_0040f7e1:
    if (iVar1 < 0) goto LAB_0040f81a;
  }
  if (iVar1 < param_1[3] + -1) {
    *param_3 = 1;
    return (float10)*(float *)(param_1[4] * iVar1 + 8 + param_1[5]);
  }
LAB_0040f81a:
  *param_3 = 0;
  return (float10)0.0;
}


/* ===== FUN_0040f830 @ 0040f830  size=583  callers: FUN_0040f120  ===== */

float10 __cdecl FUN_0040f830(int *param_1,float param_2,undefined4 *param_3)

{
  int iVar1;
  int *piVar2;
  float10 fVar3;
  float10 fVar4;
  float10 fVar5;
  float10 fVar6;
  float10 fVar7;
  
  if ((param_1 == (int *)0x0) || (*param_1 != 0x311)) goto LAB_0040fa64;
  iVar1 = FUN_0040f1e0((int)param_1,param_2);
  if (iVar1 == -3) {
    if (param_1[1] == 1) {
      *param_3 = 1;
      iVar1 = param_1[5];
      if (param_1[3] == 1) {
        return (float10)*(float *)(iVar1 + 8);
      }
LAB_0040f8a0:
      return ((float10)param_2 - (float10)*(float *)(iVar1 + 4)) * (float10)*(float *)(iVar1 + 0xc) +
             (float10)*(float *)(iVar1 + 8);
    }
    if ((param_1[1] == 0) || (iVar1 = param_1[3], iVar1 == 1)) {
      *param_3 = 1;
      return (float10)*(float *)(param_1[5] + 8);
    }
LAB_0040f90b:
    fVar3 = FUN_0040c6d0(param_2,*(float *)(param_1[5] + 4),
                         *(float *)((iVar1 + -1) * param_1[4] + 4 + param_1[5]));
    param_2 = (float)fVar3;
    iVar1 = FUN_0040f1e0((int)param_1,param_2);
    if (iVar1 != -3) {
      if (iVar1 == -2) {
        iVar1 = param_1[3] + -2;
      }
      goto LAB_0040f94a;
    }
    iVar1 = 0;
  }
  else {
    if (iVar1 == -2) {
      if (param_1[1] == 1) {
        iVar1 = (param_1[3] + -1) * param_1[4] + param_1[5];
        if (param_1[3] == 1) {
          return (float10)*(float *)(iVar1 + 8);
        }
        goto LAB_0040f8a0;
      }
      if ((param_1[1] == 0) || (iVar1 = param_1[3], iVar1 == 1)) {
        *param_3 = 1;
        return (float10)*(float *)((param_1[3] + -1) * param_1[4] + 8 + param_1[5]);
      }
      goto LAB_0040f90b;
    }
    if (iVar1 == -1) {
      *param_3 = 0;
      return (float10)0.0;
    }
LAB_0040f94a:
    if (iVar1 < 0) goto LAB_0040fa64;
  }
  if (iVar1 < param_1[3] + -1) {
    piVar2 = (int *)(param_1[4] * iVar1 + param_1[5]);
    iVar1 = (iVar1 + 1) * param_1[4] + param_1[5];
    *param_3 = 1;
    if (*piVar2 != 1) {
      if (ABS((float)piVar2[1] - *(float *)(iVar1 + 4)) < 1e-06) {
        return ((float10)*(float *)(iVar1 + 8) + (float10)(float)piVar2[2]) * (float10)0.5;
      }
      fVar3 = (float10)*(float *)(iVar1 + 4) - (float10)(float)piVar2[1];
      fVar4 = ((float10)param_2 - (float10)(float)piVar2[1]) / fVar3;
      fVar5 = fVar4 * fVar4;
      fVar6 = fVar5 * fVar4;
      fVar7 = (fVar6 + fVar6) - fVar5 * (float10)3.0;
      return (fVar6 - fVar5) * fVar3 * (float10)*(float *)(iVar1 + 0xc) +
             ((fVar7 + (float10)1.0) * (float10)(float)piVar2[2] - fVar7 * (float10)*(float *)(iVar1 + 8)) +
             fVar3 * (float10)(float)piVar2[4] * ((fVar6 - (fVar5 + fVar5)) + fVar4);
    }
    fVar3 = (float10)*(float *)(iVar1 + 4) - (float10)(float)piVar2[1];
    if (ABS(fVar3) < (float10)1e-06) {
      return (float10)0.0;
    }
    return (((float10)param_2 - (float10)(float)piVar2[1]) *
           ((float10)*(float *)(iVar1 + 8) - (float10)(float)piVar2[2])) / fVar3 + (float10)(float)piVar2[2];
  }
LAB_0040fa64:
  *param_3 = 0;
  return (float10)0.0;
}


