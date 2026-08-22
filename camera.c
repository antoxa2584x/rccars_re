/* ===== FUN_004f9da0 @ 004f9da0  size=946  callers: FUN_004f7760  ===== */

/* WARNING: Removing unreachable block (ram,0x004fa011) */
/* WARNING: Removing unreachable block (ram,0x004f9ecb) */
/* WARNING: Removing unreachable block (ram,0x004f9e07) */
/* WARNING: Removing unreachable block (ram,0x004f9f53) */
/* WARNING: Removing unreachable block (ram,0x004fa0cf) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004f9da0(void)

{
  int local_8;
  float local_4;
  
  cfgGetInt(3,s_CAMERA_00571d1c,s_defDistXZ_005724ec,&local_8);
  local_4 = (float)local_8;
  if (1.0 <= local_4) {
    if (local_4 <= 100.0) {
      DAT_014c4898 = (local_4 - 1.0) * 0.01010101 + 0.5;
    }
    else {
      DAT_014c4898 = 1.5;
    }
  }
  else {
    DAT_014c4898 = 0.5;
  }
  _DAT_014c48a8 = 0x49f42400;
  DAT_014c489c = 0;
  _DAT_014c48a0 = 0x49f42400;
  _DAT_014c48a4 = 0x49f42400;
  _DAT_014c48ac = 0x49f42400;
  _DAT_014c48b0 = 0;
  cfgGetInt(3,s_CAMERA_00571d1c,s_defDistY_005724e0,&local_8);
  local_4 = (float)local_8;
  if (1.0 <= local_4) {
    if (local_4 <= 100.0) {
      DAT_014c48b4 = (local_4 - 1.0) * 0.009090909 + 0.1;
    }
    else {
      DAT_014c48b4 = 1.0;
    }
  }
  else {
    DAT_014c48b4 = 0.1;
  }
  cfgGetInt(3,s_CAMERA_00571d1c,s_accDistY_005724d4,&local_8);
  local_4 = (float)local_8;
  if (1.0 <= local_4) {
    if (local_4 <= 100.0) {
      _DAT_014c48c4 = (local_4 - 1.0) * 0.019191919 + 0.1;
    }
    else {
      _DAT_014c48c4 = 2.0;
    }
  }
  else {
    _DAT_014c48c4 = 0.1;
  }
  _DAT_014c48bc = DAT_014c48b4 * 0.75;
  DAT_014c48b8 = 0;
  _DAT_014c48c8 = 0x49f42400;
  _DAT_014c48cc = 0;
  _DAT_014c48c0 = _DAT_014c48bc;
  cfgGetInt(3,s_CAMERA_00571d1c,s_accAlpha_005724c8,&local_8);
  local_4 = (float)local_8;
  if (1.0 <= local_4) {
    if (local_4 <= 100.0) {
      _DAT_014c48e0 = (local_4 - 1.0) * 0.01010101;
    }
    else {
      _DAT_014c48e0 = 1.0;
    }
  }
  else {
    _DAT_014c48e0 = 0.0;
  }
  DAT_014c48d0 = 0;
  DAT_014c48d4 = 0;
  _DAT_014c48d8 = 0x42b40000;
  _DAT_014c48dc = 0x42b40000;
  _DAT_014c48e4 = 0x49f42400;
  _DAT_014c48e8 = 0x3f99999a;
  cfgGetInt(3,s_Camera_0056bff8,s_VisTurn_005724c0,&local_8);
  local_4 = (float)local_8;
  if (1.0 <= local_4) {
    if (local_4 <= 100.0) {
      _DAT_014c48ec = (local_4 - 1.0) * 0.6060606 - 30.0;
    }
    else {
      _DAT_014c48ec = 30.0;
    }
  }
  else {
    _DAT_014c48ec = -30.0;
  }
  FUN_00404950(3,s_CAMERA_00571d1c,s_CDT_AngleUp_005724b4,&DAT_014c48f0);
  FUN_00404950(3,s_CAMERA_00571d1c,s_CDT_AngleUpSpeed_005724a0,&DAT_014c48f4);
  FUN_00404950(3,s_CAMERA_00571d1c,s_CDT_AngleUp_DIR_ADD_0057248c,(undefined4 *)&DAT_014c48f8);
  FUN_00404950(3,s_CAMERA_00571d1c,s_CDT_AngleUp_SPEED_DIR_ADD_00572470,&DAT_014c48fc);
  return;
}


/* ===== FUN_00500a80 @ 00500a80  size=724  callers: FUN_005007a0 FUN_005006a0  ===== */

/* WARNING: Removing unreachable block (ram,0x00500bc1) */

void __cdecl
FUN_00500a80(int param_1,undefined4 param_2,float param_3,float *param_4,undefined4 *param_5,
            undefined4 *param_6,undefined4 *param_7)

{
  float fVar1;
  float fVar2;
  float *pfVar3;
  int iVar4;
  float *pfVar5;
  undefined4 *puVar6;
  float *pfVar7;
  undefined4 *puVar8;
  float10 fVar9;
  float fVar10;
  float fVar11;
  
  pfVar3 = param_4;
  pfVar5 = (float *)&DAT_014c4898;
  pfVar7 = param_4;
  for (iVar4 = 7; iVar4 != 0; iVar4 = iVar4 + -1) {
    *pfVar7 = *pfVar5;
    pfVar5 = pfVar5 + 1;
    pfVar7 = pfVar7 + 1;
  }
  puVar6 = &DAT_014c48b4;
  puVar8 = param_5;
  for (iVar4 = 7; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar8 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar8 = puVar8 + 1;
  }
  puVar6 = &DAT_014c48d0;
  puVar8 = param_6;
  for (iVar4 = 7; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar8 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar8 = puVar8 + 1;
  }
  puVar6 = &DAT_00564810;
  puVar8 = param_7;
  for (iVar4 = 7; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar8 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar8 = puVar8 + 1;
  }
  iVar4 = *(int *)(param_1 + 0xe8);
  if ((*(int *)(iVar4 + 0x10) == 0) ||
     ((*(int *)(iVar4 + 0xe828) == 0 && (3.0 <= *(float *)(iVar4 + 0xe830))))) {
    fVar1 = SQRT(*(float *)(iVar4 + 0x58e0) * *(float *)(iVar4 + 0x58e0) +
                 *(float *)(iVar4 + 0x58e4) * *(float *)(iVar4 + 0x58e4) +
                 *(float *)(iVar4 + 0x58e8) * *(float *)(iVar4 + 0x58e8));
    param_4 = (float *)(*param_4 * 0.5);
    if (4.0 <= fVar1) {
      if (fVar1 <= 8.0) {
        param_4 = (float *)((float)param_4 * (fVar1 - 4.0) * 0.25);
      }
    }
    else {
      param_4 = (float *)0x0;
    }
    fVar9 = moveTowards(*(float *)(iVar4 + 0x6e0),(float)param_4,1.0,param_3);
    *(float *)(*(int *)(param_1 + 0xe8) + 0x6e0) = (float)fVar9;
    iVar4 = *(int *)(param_1 + 0xe8);
    fVar1 = *(float *)(iVar4 + 0x6e0);
    fVar2 = *pfVar3;
    if (((*(byte *)(iVar4 + 0x5754) & 1) == 0) || ((*(byte *)(iVar4 + 0x576c) & 1) == 0)) {
      fVar11 = 0.3;
      fVar10 = 0.0;
    }
    else {
      fVar10 = *pfVar3 * 0.2;
      fVar11 = 0.5;
    }
    fVar9 = moveTowards(*(float *)(iVar4 + 0x6e4),fVar10,fVar11,param_3);
    *(float *)(*(int *)(param_1 + 0xe8) + 0x6e4) = (float)fVar9;
    *pfVar3 = fVar1 + fVar2 + *(float *)(*(int *)(param_1 + 0xe8) + 0x6e4);
    iVar4 = *(int *)(param_1 + 0xe8);
    if (180.0 <= SQRT(*(float *)(iVar4 + 0x58ec) * *(float *)(iVar4 + 0x58ec) +
                      *(float *)(iVar4 + 0x58f0) * *(float *)(iVar4 + 0x58f0) +
                      *(float *)(iVar4 + 0x58f4) * *(float *)(iVar4 + 0x58f4)) * 57.295776) {
      fVar1 = 30.0;
    }
    else {
      fVar1 = (float)param_6[4] * 200.0;
    }
    param_6[4] = fVar1;
    param_6[5] = fVar1;
    param_6[6] = 0x40000000;
    param_6[2] = 0x41a00000;
    param_6[3] = 0x41a00000;
    if (DAT_0147d254 != 0) {
      *pfVar3 = 1.04432;
      pfVar3[1] = 0.0;
      pfVar3[2] = 0.0;
      pfVar3[3] = 0.0;
      pfVar3[6] = 0.0;
      pfVar3[5] = 2e+06;
      pfVar3[4] = 2e+06;
      *param_5 = 0x3ed17460;
      param_5[1] = 0;
      param_5[2] = 0;
      param_5[3] = 0;
      param_5[6] = 0;
      param_5[5] = 0x49f42400;
      param_5[4] = 0x49f42400;
      *param_6 = 0;
      param_6[1] = 0;
      param_6[2] = 0;
      param_6[3] = 0;
      param_6[6] = 0;
      param_6[5] = 0x49f42400;
      param_6[4] = 0x49f42400;
      puVar6 = &DAT_00564810;
      for (iVar4 = 7; iVar4 != 0; iVar4 = iVar4 + -1) {
        *param_7 = *puVar6;
        puVar6 = puVar6 + 1;
        param_7 = param_7 + 1;
      }
    }
  }
  else {
    if (*(int *)(iVar4 + 0xe82c) != 0) {
      *param_4 = 2.0;
      *param_5 = 0x3f800000;
      return;
    }
    if (*(float *)(iVar4 + 0xe830) < 3.0) {
      for (iVar4 = 7; iVar4 != 0; iVar4 = iVar4 + -1) {
        *param_5 = 0;
        param_5 = param_5 + 1;
      }
      for (iVar4 = 7; iVar4 != 0; iVar4 = iVar4 + -1) {
        *param_6 = 0;
        param_6 = param_6 + 1;
      }
      for (iVar4 = 7; iVar4 != 0; iVar4 = iVar4 + -1) {
        *param_4 = 0.0;
        param_4 = param_4 + 1;
      }
      return;
    }
  }
  return;
}


/* ===== FUN_005006a0 @ 005006a0  size=188  callers: FUN_004f27a0  ===== */

void __cdecl FUN_005006a0(int param_1)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
  float *pfVar4;
  undefined4 local_b0 [7];
  undefined4 local_94 [7];
  float local_78 [7];
  undefined4 local_5c [7];
  float local_40 [16];
  
  iVar1 = *(int *)(param_1 + 0xe8);
  pfVar3 = (float *)(param_1 + 0xf8);
  pfVar4 = local_40;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *pfVar4 = *pfVar3;
    pfVar3 = pfVar3 + 1;
    pfVar4 = pfVar4 + 1;
  }
  FUN_00471b20((undefined4 *)(iVar1 + 0x6f4));
  FUN_00500840(param_1,local_40);
  FUN_00500a80(param_1,local_40,2e+06,local_78,local_b0,local_5c,local_94);
  FUN_00471c00((undefined4 *)(*(int *)(param_1 + 0xe8) + 0x6f4),local_78,local_b0,local_5c,local_94);
  FUN_00471ca0((float *)(*(int *)(param_1 + 0xe8) + 0x6f4),local_40,2e+06);
  return;
}


/* ===== FUN_005007a0 @ 005007a0  size=147  callers: FUN_00508b50  ===== */

void __cdecl FUN_005007a0(int param_1,float param_2)

{
  undefined4 local_b0 [7];
  undefined4 local_94 [7];
  float local_78 [7];
  undefined4 local_5c [7];
  float local_40 [16];
  
  FUN_00500840(param_1,local_40);
  FUN_00500a80(param_1,local_40,param_2,local_78,local_b0,local_5c,local_94);
  FUN_00471c00((undefined4 *)(*(int *)(param_1 + 0xe8) + 0x6f4),local_78,local_b0,local_5c,local_94);
  FUN_00471ca0((float *)(*(int *)(param_1 + 0xe8) + 0x6f4),local_40,param_2);
  return;
}


/* ===== FUN_00500e60 @ 00500e60  size=608  callers:  ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_00500e60(int param_1,float *param_2,float *param_3,float *param_4,undefined4 param_5,float *param_6,
            float param_7,float *param_8,float *param_9,undefined4 *param_10)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  float10 fVar5;
  float local_c;
  float local_8;
  float local_4;
  
  *param_8 = *param_3;
  param_8[1] = param_3[1];
  param_8[2] = param_3[2];
  *param_9 = *param_6;
  param_9[1] = param_6[1];
  param_9[2] = param_6[2];
  if (param_2 != (float *)0x0) {
    local_c = *param_2;
    local_8 = param_2[1];
    fVar1 = local_c - *param_3;
    local_4 = param_2[2];
    fVar3 = local_8 - param_3[1];
    fVar2 = local_4 - param_3[2];
    iVar4 = FUN_00500d60(param_1,param_3,(float *)0x0);
    if (iVar4 == 0) {
      param_3 = DAT_014c48f0;
    }
    else {
      param_3 = (float *)0x0;
    }
    fVar5 = moveTowards(*(float *)(*(int *)(param_1 + 0xe8) + 0x6ec),(float)param_3,DAT_014c48f4,param_7);
    *(float *)(*(int *)(param_1 + 0xe8) + 0x6ec) = (float)fVar5;
    FUN_005010c0(&local_c,SQRT(fVar1 * fVar1 + fVar3 * fVar3 + fVar2 * fVar2),param_4,param_6,
                 -*(float *)(*(int *)(param_1 + 0xe8) + 0x6ec),param_8);
    *param_9 = local_c - *param_8;
    param_9[1] = local_8 - param_8[1];
    fVar1 = local_4 - param_8[2];
    param_9[2] = fVar1;
    fVar1 = SQRT(*param_9 * *param_9 + param_9[1] * param_9[1] + fVar1 * fVar1);
    if ((1e-06 <= fVar1) && (1e-06 <= ABS(fVar1 - 1.0))) {
      fVar1 = 1.0 / fVar1;
      *param_9 = fVar1 * *param_9;
      param_9[1] = fVar1 * param_9[1];
      param_9[2] = fVar1 * param_9[2];
    }
    fVar1 = *(float *)(*(int *)(param_1 + 0xe8) + 0x6ec);
    if (0.0 <= fVar1) {
      param_3 = _DAT_014c48f8;
      if (fVar1 <= (float)DAT_014c48f0) {
        if (1e-06 <= ABS((float)DAT_014c48f0)) {
          param_3 = (float *)(((float)_DAT_014c48f8 * fVar1) / (float)DAT_014c48f0);
        }
        else {
          param_3 = (float *)0x0;
        }
      }
    }
    else {
      param_3 = (float *)0x0;
    }
    fVar5 = moveTowards(*(float *)(*(int *)(param_1 + 0xe8) + 0x6f0),(float)param_3,DAT_014c48fc,param_7);
    *(float *)(*(int *)(param_1 + 0xe8) + 0x6f0) = (float)fVar5;
    *param_10 = *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0x6f0);
  }
  return;
}


