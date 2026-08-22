/* ===== FUN_004fb9e0 @ 004fb9e0  size=618  callers: FUN_004fb340 FUN_004fbc50  ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __cdecl FUN_004fb9e0(int param_1,int param_2,float param_3,int *param_4)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  float local_80;
  int local_7c;
  float local_78;
  int local_74;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48 [3];
  int local_3c [3];
  float local_30;
  float local_2c;
  float local_28;
  
  local_74 = param_2 * 0x68;
  iVar4 = local_74 + *(int *)(param_1 + 0xe8);
  if (*(float *)(iVar4 + 0x5934) - 1e-06 < param_3) {
    return 0;
  }
  mat4GetRow1(param_1 + 0xf8,&local_70);
  fVar1 = *(float *)(iVar4 + 0x595c);
  local_80 = 0.0;
  local_7c = 0;
  do {
    FUN_004ef650(param_1,param_2,local_48,&local_78,(float *)0x0,(float *)0x0);
    if (param_4 == (int *)0x0) {
      iVar3 = FUN_004fb8b0(param_1,param_2,0,0,(float *)0x0,1,local_3c);
      if (iVar3 == 0) break;
    }
    else {
      piVar5 = local_3c;
      for (iVar3 = 0xf; iVar3 != 0; iVar3 = iVar3 + -1) {
        *piVar5 = *param_4;
        param_4 = param_4 + 1;
        piVar5 = piVar5 + 1;
      }
      param_4 = (int *)0x0;
    }
    if (*(float *)(iVar4 + 0x595c) < *(float *)(iVar4 + 0x5964) + 0.0001) goto LAB_004fbbf5;
    fVar2 = local_78 * -3.0;
    local_58 = local_70 * fVar2 + local_30;
    local_54 = local_6c * fVar2 + local_2c;
    local_50 = local_68 * fVar2 + local_28;
    iVar3 = FUN_00408f50(&local_58,&local_70,local_48,local_78,&local_64,&local_4c);
    if (iVar3 != 1) {
      *(float *)(iVar4 + 0x595c) = fVar1;
      return 0;
    }
    local_80 = SQRT((local_30 - local_64) * (local_30 - local_64) +
                    (local_2c - local_60) * (local_2c - local_60) +
                    (local_28 - local_5c) * (local_28 - local_5c)) + local_80;
    if (param_3 < local_80) {
      *(float *)(iVar4 + 0x595c) = fVar1;
      return 0;
    }
    *(float *)(iVar4 + 0x595c) = fVar1 - local_80;
    fVar2 = (fVar1 - local_80) - *(float *)(local_74 + 0x5934 + *(int *)(param_1 + 0xe8)) * 0.01;
    *(float *)(iVar4 + 0x595c) = fVar2;
    if (fVar2 < *(float *)(iVar4 + 0x5964)) {
      *(undefined4 *)(iVar4 + 0x595c) = *(undefined4 *)(iVar4 + 0x5964);
    }
    local_7c = local_7c + 1;
  } while (local_7c < 5);
  if (1 < local_7c) {
    _DAT_014c4f24 = _DAT_014c4f24 + 1;
  }
  if (local_7c != 5) {
    return 1;
  }
LAB_004fbbf5:
  *(float *)(iVar4 + 0x595c) = fVar1;
  return 0;
}


/* ===== FUN_004fbc50 @ 004fbc50  size=280  callers: FUN_004fb340  ===== */

void __cdecl FUN_004fbc50(int param_1,int param_2,float param_3)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  float local_10;
  float local_c [3];
  
  iVar5 = param_2 * 0x68;
  iVar4 = iVar5 + *(int *)(param_1 + 0xe8);
  if (*(float *)(iVar5 + 0x5934 + *(int *)(param_1 + 0xe8)) < param_3) {
    param_3 = *(float *)(iVar4 + 0x5934) * 0.99;
  }
  FUN_004ef650(param_1,param_2,local_c,&local_10,(float *)0x0,(float *)0x0);
  local_10 = local_10 * 0.99;
  fVar2 = *(float *)(iVar5 + 0x595c + *(int *)(param_1 + 0xe8));
  pfVar1 = (float *)(iVar5 + 0x595c + *(int *)(param_1 + 0xe8));
  fVar3 = local_10;
  if (param_3 < local_10) {
    fVar3 = param_3;
  }
  *pfVar1 = fVar3 + *pfVar1;
  if (*(float *)(iVar4 + 0x5968) < *(float *)(iVar4 + 0x595c)) {
    *(undefined4 *)(iVar4 + 0x595c) = *(undefined4 *)(iVar4 + 0x5968);
  }
  iVar4 = FUN_004fb9e0(param_1,param_2,local_10,(int *)0x0);
  if (iVar4 != 0) {
    if (*(float *)(iVar5 + 0x595c + *(int *)(param_1 + 0xe8)) < fVar2) {
      *(float *)(iVar5 + 0x595c + *(int *)(param_1 + 0xe8)) = fVar2;
    }
    return;
  }
  *(float *)(iVar5 + 0x595c + *(int *)(param_1 + 0xe8)) = fVar2;
  return;
}


/* ===== FUN_004fbd70 @ 004fbd70  size=239  callers: FUN_004fb340  ===== */

bool __cdecl FUN_004fbd70(undefined4 param_1,float *param_2,float param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  uint *puVar6;
  uint local_90 [5];
  float local_7c;
  float local_74;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
  int *local_44;
  int local_3c [15];
  
  fVar1 = param_3 * 1.5;
  fVar2 = DAT_0055e9a0 * fVar1 + *param_2;
  fVar3 = DAT_0055e9a4 * fVar1 + param_2[1];
  fVar4 = DAT_0055e9a8 * fVar1 + param_2[2];
  iVar5 = FUN_00455890(DAT_0148f908,param_2,&DAT_0055e9a0,fVar1,(int *)0x0,(int *)0x0,0,4,(int *)0x0);
  if (iVar5 != 0) {
    return true;
  }
  puVar6 = local_90;
  for (iVar5 = 0x15; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  local_44 = DAT_0148f908;
  local_90[0] = 4;
  local_7c = fVar1;
  local_74 = fVar2;
  local_70 = fVar3;
  local_6c = fVar4;
  local_68 = fVar2;
  local_64 = fVar3;
  local_60 = fVar4;
  iVar5 = FUN_00454970(local_90,local_3c);
  return iVar5 != 0;
}


/* ===== FUN_004557e0 @ 004557e0  size=171  callers: collSpheresMoved FUN_0050ebb0 FUN_00456260 FUN_004fb340  ===== */

undefined4 __cdecl
FUN_004557e0(int *param_1,float *param_2,float *param_3,int *param_4,int *param_5,int param_6,uint param_7,
            int *param_8)

{
  float fVar1;
  float fVar2;
  undefined4 uVar3;
  float local_c;
  float local_8;
  float local_4;
  
  local_c = *param_3 - *param_2;
  local_8 = param_3[1] - param_2[1];
  local_4 = param_3[2] - param_2[2];
  fVar1 = SQRT(local_c * local_c + local_8 * local_8 + local_4 * local_4);
  if (ABS(fVar1) < 0.001) {
    return 0;
  }
  fVar2 = 1.0 / fVar1;
  local_c = local_c * fVar2;
  local_8 = local_8 * fVar2;
  local_4 = local_4 * fVar2;
  uVar3 = FUN_00455890(param_1,param_2,&local_c,fVar1,param_4,param_5,param_6,param_7,param_8);
  return uVar3;
}


