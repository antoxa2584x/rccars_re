/* ===== FUN_004722c0 @ 004722c0  size=416  callers: FUN_00471ca0 FUN_00471ca0 FUN_00471ca0 FUN_00471ca0  ===== */

void __cdecl FUN_004722c0(float param_1,float param_2,float *param_3,float *param_4)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float *pfVar4;
  float *pfVar5;
  float10 fVar6;
  float10 fVar7;
  
  pfVar5 = param_4;
  pfVar4 = param_3;
  if ((param_4 != (float *)0x0) && (*param_4 = 0.0, param_3[1] + 1e-06 <= ABS(param_1 - *param_3))) {
    param_4 = (float *)(*param_3 - param_3[2]);
    if (((float)param_4 <= param_1) && (param_4 = (float *)(param_3[3] + *param_3), param_1 <= (float)param_4)
       ) {
      param_4 = (float *)param_1;
    }
    fVar6 = moveTowards(param_1,(float)param_4,param_3[5],param_2);
    if ((fVar6 < (float10)param_3[3] + (float10)*param_3 + (float10)1e-06) &&
       (((float10)*param_3 - (float10)param_3[2]) - (float10)1e-06 < fVar6)) {
      pfVar1 = param_3 + 6;
      fVar2 = param_3[4];
      fVar7 = (float10)fVar2;
      if (1e-06 <= ABS(*pfVar1)) {
        fVar3 = *param_3;
        if (fVar3 <= param_1) {
          param_3 = (float *)param_3[3];
        }
        else {
          param_3 = (float *)param_3[2];
        }
        fVar7 = FUN_0049d740(fVar3,(float)param_3,*pfVar1,(float)fVar6);
        fVar7 = fVar7 * (float10)fVar2;
      }
      fVar7 = fVar7 * (float10)param_2 - (float10)(float)ABS(fVar6 - (float10)param_1);
      if ((float10)0.0 <= fVar7) {
        if (fVar7 <= (float10)2e+06) {
          param_2 = (float)fVar7;
        }
        else {
          param_2 = 2e+06;
        }
      }
      else {
        param_2 = 0.0;
      }
      fVar6 = moveTowards((float)fVar6,*pfVar4,param_2,1.0);
    }
    *pfVar5 = (float)(fVar6 - (float10)param_1);
  }
  return;
}


/* ===== FUN_0040b1f0 @ 0040b1f0  size=444  callers: FUN_00448f20 FUN_0044bbe0 FUN_00408040 FUN_00503190 FUN_00471ca0 FUN_0040b4c0 FUN_00409760 FUN_00409760 FUN_00409ac0 FUN_00409ac0 FUN_00503cb0 FUN_0050d0d0 FUN_004f2270 FUN_004739a0 FUN_00473e70 FUN_004763c0 FUN_00476420 FUN_0044d6b0 FUN_00508e70 FUN_0044e2f0 FUN_005111f0 FUN_00472cc0 FUN_004ff2d0 FUN_00504470 FUN_00506360 FUN_00436230 FUN_00450f10 FUN_004508a0  ===== */

undefined4 __cdecl FUN_0040b1f0(int param_1,float *param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  bool bVar6;
  float10 fVar7;
  float10 fVar8;
  short local_20;
  int local_1c;
  float *local_18;
  int local_10;
  float *local_c;
  int local_8;
  int local_4;
  
  fVar7 = FUN_0040b3d0(param_1);
  if ((float10)1e-12 <= ABS(fVar7)) {
    local_1c = 0;
    local_c = param_2;
    local_4 = 4;
    iVar2 = param_1;
    iVar3 = param_1;
    iVar4 = param_1;
    iVar5 = param_1;
    do {
      local_10 = 0;
      local_18 = local_c;
      local_8 = 4;
      do {
        switch(local_1c) {
        case 0:
          iVar3 = 1;
          local_20 = 2;
          param_2 = (float *)0x3;
          break;
        case 1:
          iVar3 = 0;
          local_20 = 2;
          param_2 = (float *)0x3;
          break;
        case 2:
          iVar3 = 0;
          local_20 = 1;
          param_2 = (float *)0x3;
          break;
        case 3:
          iVar3 = 0;
          local_20 = 1;
          param_2 = (float *)0x2;
        }
        switch(local_10) {
        case 0:
          iVar4 = 1;
          iVar5 = 2;
          iVar2 = 3;
          break;
        case 1:
          iVar4 = 0;
          iVar5 = 2;
          iVar2 = 3;
          break;
        case 2:
          iVar4 = 0;
          iVar5 = 1;
          iVar2 = 3;
          break;
        case 3:
          iVar4 = 0;
          iVar5 = 1;
          iVar2 = 2;
        }
        uVar1 = local_1c + local_10 & 0x80000001;
        bVar6 = uVar1 == 0;
        if ((int)uVar1 < 0) {
          bVar6 = (uVar1 - 1 | 0xfffffffe) == 0xffffffff;
        }
        if (bVar6) {
          fVar8 = FUN_0040b3f0(param_1,(short)iVar3,local_20,(short)param_2,(short)iVar4,(short)iVar5,
                               (short)iVar2);
          *local_18 = (float)(fVar8 / (float10)(float)fVar7);
        }
        else {
          fVar8 = FUN_0040b3f0(param_1,(short)iVar3,local_20,(short)param_2,(short)iVar4,(short)iVar5,
                               (short)iVar2);
          *local_18 = (float)-(fVar8 / (float10)(float)fVar7);
        }
        local_10 = local_10 + 1;
        local_18 = local_18 + 4;
        local_8 = local_8 + -1;
      } while (local_8 != 0);
      local_1c = local_1c + 1;
      local_c = local_c + 1;
      local_4 = local_4 + -1;
    } while (local_4 != 0);
    return 1;
  }
  return 0;
}


/* ===== FUN_00410150 @ 00410150  size=167  callers: FUN_00471ca0 FUN_00471ca0 FUN_004fdb50 FUN_004fddd0 FUN_00511b50 FUN_00409150 FUN_00520760 FUN_00520a10 FUN_00516f50 FUN_0050ec10 FUN_00518050 FUN_0040d310 FUN_004105a0 FUN_004e8f00 FUN_004eb360 FUN_00510a40 FUN_00510f40 FUN_0045aa50 FUN_005072f0 FUN_005072f0 carVisualUpdate  ===== */

float10 __cdecl FUN_00410150(float *param_1,float *param_2)

{
  float10 fVar1;
  float local_18 [3];
  float local_c [3];
  
  local_c[1] = 0.0;
  local_18[1] = 0.0;
  local_c[0] = *param_1;
  local_c[2] = param_1[2];
  local_18[0] = *param_2;
  local_18[2] = param_2[2];
  FUN_00410080(local_c,local_18);
  fVar1 = (float10)FUN_00547be0();
  fVar1 = fVar1 * (float10)57.295776;
  if ((float10)0.0 <= fVar1) {
    if ((float10)180.0 < fVar1) {
      fVar1 = (float10)180.0;
    }
  }
  else {
    fVar1 = (float10)0.0;
  }
  if (local_c[2] * local_18[0] - local_18[2] * local_c[0] < 0.0) {
    fVar1 = -fVar1;
  }
  return fVar1;
}


