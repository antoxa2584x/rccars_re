/* ===== physStep @ 005074d0  size=770 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* per-frame vehicle update; called indirectly */

undefined4 __cdecl physStep(uint param_1,int *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  uint uVar4;
  int iVar5;
  float *pfVar6;
  float *pfVar7;
  int iVar8;
  float *pfVar9;
  undefined4 *puVar10;
  int *piVar11;
  float *pfVar12;
  undefined4 *puVar13;
  float10 fVar14;
  int local_100;
  float local_f8;
  float local_f4;
  float local_f0;
  int local_ec;
  float *local_e8;
  float local_e4 [9];
  float local_c0 [16];
  undefined4 local_80 [16];
  float local_40 [16];
  
  iVar8 = param_2[0x3a];
  uVar4 = *(uint *)(iVar8 + 0x58 + *(int *)(param_1 + 0x23c) * 4);
  iVar5 = *(int *)(iVar8 + 0x54);
  if ((((iVar5 == 0) || (iVar5 == 1)) || (iVar5 == 2)) &&
     (((int)uVar4 < DAT_005728b0 && (*(int *)(iVar8 + 0x50) != 4)))) {
    FUN_004feff0((int)param_2,param_1);
  }
  FUN_00531ab0((int)param_2,param_1);
  FUN_0040cbb0((int)(param_2 + 0x3e),&local_f8);
  iVar8 = param_2[0x3a];
  iVar5 = cfgGetInt(0,s_Video_00561940,s_VIDEO_EnvMap_00568bbc,&local_ec);
  if ((iVar5 == 0) || (local_ec == 0)) goto LAB_005076e7;
  local_e4[0] = local_f8 - *(float *)(iVar8 + 0x5cb4);
  local_e8 = (float *)(iVar8 + 0x5cb4);
  local_e4[1] = local_f4 - *(float *)(iVar8 + 0x5cb8);
  local_e4[2] = local_f0 - *(float *)(iVar8 + 0x5cbc);
  FUN_00448d50((int)param_2,0x40);
  pfVar12 = local_e4 + 3;
  pfVar9 = (float *)(param_2 + 0x4e);
  local_100 = 3;
  do {
    *pfVar12 = 0.0;
    pfVar7 = local_e4;
    fVar1 = *pfVar12;
    iVar5 = 3;
    pfVar6 = pfVar9;
    do {
      fVar2 = *pfVar6;
      fVar3 = *pfVar7;
      pfVar6 = pfVar6 + 4;
      pfVar7 = pfVar7 + 1;
      iVar5 = iVar5 + -1;
      fVar1 = fVar2 * fVar3 + fVar1;
    } while (iVar5 != 0);
    *pfVar12 = fVar1;
    pfVar12 = pfVar12 + 1;
    pfVar9 = pfVar9 + 1;
    local_100 = local_100 + -1;
  } while (local_100 != 0);
  fVar1 = *(float *)(iVar8 + 0x5ca8) - local_e4[5] * _DAT_0055453c;
  *(float *)(iVar8 + 0x5ca8) = fVar1;
  FUN_0040cc10(local_80,fVar1);
  FUN_0040cbf0((int)(param_2 + 0x3e),local_e4 + 6);
  fVar14 = FUN_00410150((float *)&DAT_0055e990,local_e4 + 6);
  *(float *)(iVar8 + 0x5cac) = (float)fVar14;
  FUN_0040cc60(local_40,(float)fVar14);
  FUN_0040c7f0((int)local_40,(int)local_80,local_c0);
  *local_e8 = local_f8;
  local_e8[1] = local_f4;
  local_e8[2] = local_f0;
  if (DAT_00e2aa5c == 0) {
    if (&stack0x00000000 != (undefined1 *)0xc0) goto LAB_00507693;
    DAT_00e2aa5c = 1;
    puVar10 = &DAT_005619e8;
    puVar13 = &DAT_00e2b950;
    for (iVar8 = 0x10; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar13 = *puVar10;
      puVar10 = puVar10 + 1;
      puVar13 = puVar13 + 1;
    }
  }
  else {
    if (&stack0x00000000 == (undefined1 *)0xc0) goto LAB_005076e7;
LAB_00507693:
    DAT_00e2aa5c = 0;
    pfVar9 = local_c0;
    pfVar12 = (float *)&DAT_00e2b950;
    for (iVar8 = 0x10; iVar8 != 0; iVar8 = iVar8 + -1) {
      *pfVar12 = *pfVar9;
      pfVar9 = pfVar9 + 1;
      pfVar12 = pfVar12 + 1;
    }
  }
  iVar8 = (**(code **)(*(int *)*DAT_00e32328 + 0x94))();
  if (iVar8 != 0) {
    FUN_00460e60(iVar8);
  }
LAB_005076e7:
  FUN_00503f00(param_2,uVar4);
  piVar11 = param_2;
  if (-1 < (int)uVar4) {
    piVar11 = *(int **)(param_2[0x3a] + 0xe718 + uVar4 * 4);
    FUN_0044c8e0((int)piVar11,(float *)(param_2 + 0x3e),0);
  }
  physStepWheels(param_1,(int)param_2,(int)piVar11,uVar4);
  physStepBody(param_1,param_2,uVar4,1);
  FUN_00459cd0((uint)piVar11,(uint *)0x0);
  physStepBody(param_1,param_2,uVar4,0);
  if (DAT_00e2aa5c == 0) {
    DAT_00e2aa5c = 1;
    puVar10 = &DAT_005619e8;
    puVar13 = &DAT_00e2b950;
    for (iVar8 = 0x10; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar13 = *puVar10;
      puVar10 = puVar10 + 1;
      puVar13 = puVar13 + 1;
    }
    iVar8 = (**(code **)(*(int *)*DAT_00e32328 + 0x94))();
    if (iVar8 != 0) {
      FUN_00460e60(iVar8);
    }
  }
  physStepPost(param_1,(int)param_2);
  return 1;
}



/* ===== physStepBody @ 005077e0  size=952 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* child of physStep */

undefined4 __cdecl physStepBody(uint param_1,undefined4 *param_2,uint param_3,int param_4)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  int iVar8;
  float *pfVar9;
  float *pfVar10;
  uint *puVar11;
  float local_bc [4];
  float local_ac [12];
  uint local_7c [7];
  float local_60;
  float local_5c;
  float local_58;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20 [3];
  int local_14;
  undefined4 *local_10;
  uint local_c;
  uint *puStack_8;
  
  iVar8 = cfgGetInt(0,s_Video_00561940,s_VIDEO_Light_00568bcc,&local_14);
  if ((iVar8 == 0) || (local_14 == 0)) {
    param_4 = 0;
  }
  if ((int)param_3 < 1) {
    if (param_4 != 0) {
      if ((param_3 == 0) && (param_1 != 0)) {
        FUN_0040cbb0(param_1,&local_38);
        puVar5 = param_2;
        FUN_0040cbb0((int)(param_2 + 0x3e),&local_2c);
        fVar2 = SQRT((local_38 - local_2c) * (local_38 - local_2c) +
                     (local_34 - local_28) * (local_34 - local_28) +
                     (local_30 - local_24) * (local_30 - local_24));
        puVar6 = _DAT_00554390;
        if (((DAT_014c4888 <= fVar2) && (puVar6 = _DAT_005543a0, fVar2 <= DAT_014c488c)) &&
           (_DAT_00554380 <= ABS(DAT_014c488c - DAT_014c4888))) {
          puVar6 = (undefined4 *)
                   ((float)_DAT_00554390 - (fVar2 - DAT_014c4888) / (DAT_014c488c - DAT_014c4888));
        }
        puVar7 = (undefined4 *)((float)DAT_014c46c0 * *(float *)(puVar5[0x3a] + 0xe824));
        if ((float)_DAT_005543a0 <= (float)puVar6) {
          param_2 = puVar7;
          if (((float)puVar6 <= (float)_DAT_00554390) &&
             (param_2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_005543c0))) {
            param_2 = (undefined4 *)
                      ((float)puVar6 * ((float)puVar7 - _DAT_005544b0) + _DAT_005544b0);
          }
        }
        else {
          param_2 = (undefined4 *)0x437f0000;
        }
        local_c = DAT_014c4894;
        puVar5 = (undefined4 *)((float)DAT_014c4894 * *(float *)(puVar5[0x3a] + 0xe824));
        if ((float)_DAT_005543a0 <= (float)puVar6) {
          local_10 = puVar5;
          if (((float)puVar6 <= (float)_DAT_00554390) &&
             (local_10 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_005543c0))) {
            local_10 = (undefined4 *)((float)puVar6 * (float)puVar5);
          }
        }
        else {
          local_10 = (undefined4 *)0x0;
        }
      }
      else {
        piVar1 = param_2 + 0x3a;
        local_c = DAT_014c4894;
        param_2 = (undefined4 *)((float)DAT_014c46c0 * *(float *)(*piVar1 + 0xe824));
        local_10 = (undefined4 *)((float)DAT_014c4894 * *(float *)(*piVar1 + 0xe824));
      }
      puStack_8 = &param_3;
      param_3 = (uint)ROUND(ROUND((float)param_2));
      if (0xff < param_3) {
        param_3 = 0xff;
      }
      param_2 = &param_1;
      param_1 = (uint)ROUND(ROUND((float)local_10));
      if (0xff < param_1) {
        param_1 = 0xff;
      }
      DAT_014c4894 = local_c;
      FUN_00457150(((param_3 | 0xffffff00) << 8 | param_3 & 0xff) << 8 | param_3 & 0xff);
      puVar11 = local_7c;
      for (iVar8 = 0x11; iVar8 != 0; iVar8 = iVar8 + -1) {
        *puVar11 = 0;
        puVar11 = puVar11 + 1;
      }
      local_7c[0] = 2;
      local_7c[1] = ((param_1 | 0xffffff00) << 8 | param_1 & 0xff) << 8 | param_1 & 0xff;
      FUN_0040cc60(local_bc,DAT_014c45c8);
      iVar8 = 0;
      do {
        pfVar9 = (float *)&DAT_0055e9b4;
        pfVar10 = (float *)((int)local_ac + iVar8);
        *(float *)((int)local_20 + iVar8) = *(float *)((int)local_bc + iVar8) * DAT_0055e9b0;
        fVar2 = *(float *)((int)local_20 + iVar8);
        do {
          fVar3 = *pfVar10;
          fVar4 = *pfVar9;
          pfVar9 = pfVar9 + 1;
          pfVar10 = pfVar10 + 4;
          fVar2 = fVar3 * fVar4 + fVar2;
        } while ((int)pfVar9 < 0x55e9bc);
        *(float *)((int)local_20 + iVar8) = fVar2;
        iVar8 = iVar8 + 4;
      } while (iVar8 < 0xc);
      FUN_0040b4c0(local_20,(float *)&DAT_0055e9a0,DAT_014c4a40,&local_60);
      param_2 = (undefined4 *)SQRT(local_60 * local_60 + local_5c * local_5c + local_58 * local_58);
      if ((_DAT_00554380 <= (float)param_2) &&
         (_DAT_00554380 <= ABS((float)param_2 - (float)_DAT_00554390))) {
        fVar2 = (float)_DAT_00554390 / (float)param_2;
        local_60 = local_60 * fVar2;
        local_5c = local_5c * fVar2;
        local_58 = local_58 * fVar2;
      }
      FUN_00457110(0,local_7c);
    }
  }
  else {
    param_4 = 0;
  }
  iVar8 = param_4;
  FUN_00457130(0,param_4);
  FUN_00457130(0xffffffff,iVar8);
  return 1;
}



/* ===== physStepWheels @ 00507ba0  size=472 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* child of physStep */

undefined4 __cdecl physStepWheels(int param_1,int param_2,int param_3,int param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  longlong lVar4;
  int local_20;
  undefined4 local_1c;
  float local_18 [3];
  float local_c [3];
  
  iVar1 = cfgGetInt(0,s_Video_00561940,s_VIDEO_EnvMap_00568bbc,&local_20);
  if (((iVar1 != 0) && (local_20 != 0)) && (*(int *)(param_2 + 0xe8) != -0x5ca8)) {
    if (param_4 < 1) {
      local_1c = *(undefined4 *)(*(int *)(param_2 + 0xe8) + 0xe824);
      if ((param_4 == 0) && (param_1 != 0)) {
        FUN_0040cbb0(param_1,local_18);
        FUN_0040cbb0(param_2 + 0xf8,local_c);
      }
      lVar4 = __ftol();
      uVar2 = (uint)lVar4;
      lVar4 = __ftol();
      iVar1 = (int)lVar4;
      lVar4 = __ftol();
      iVar3 = (int)lVar4;
      lVar4 = __ftol();
      physHelperB(0x14edd60,(int)lVar4,iVar3,iVar1,uVar2);
      physHelperA(*(uint **)(param_3 + 0x10),0x14edd60);
      return 1;
    }
    return 1;
  }
  return 0;
}



/* ===== physStepPost @ 00507ef0  size=314 ===== */

/* child of physStep */

undefined4 __cdecl physStepPost(int param_1,int param_2)

{
  int iVar1;
  uint *puVar2;
  longlong lVar3;
  int iVar4;
  int iVar5;
  int local_78 [30];
  
  local_78[1] = 0;
  iVar1 = cfgGetInt(0,s_Video_00561940,s_Video_DirtCarEff_00573544,local_78);
  if ((iVar1 != 0) && (local_78[0] != 0)) {
    if (((DAT_00571d24 != 0) || (*(int *)(*(int *)(param_2 + 0xe8) + 0x50) == 0)) &&
       ((*(uint *)(param_2 + 4) & 0x8000000) != 0)) {
      iVar1 = *(int *)(param_2 + 0xe8);
      if ((DAT_00571d24 == 0) && (*(int *)(iVar1 + 0x5cc0) != 0)) {
        iVar5 = 0;
        *(undefined4 *)(iVar1 + 0x5cc0) = 0;
        iVar4 = 0;
        lVar3 = __ftol();
        FUN_0046c2a0(*(int *)(iVar1 + 0x5ccc),*(int *)(iVar1 + 0x5cd0),*(int *)(iVar1 + 0x5cc4),
                     (byte)lVar3,iVar4,iVar5);
      }
      iVar4 = *(int *)(*(int *)(param_2 + 0xe8) + 0x58 + *(int *)(param_1 + 0x23c) * 4);
      if ((*(int *)(iVar1 + 0x5ccc) != 0) && (*(int *)(iVar1 + 0x5cd0) != 0)) {
        if (iVar4 < 0) {
          puVar2 = *(uint **)(iVar1 + 0x5d08);
        }
        else {
          puVar2 = *(uint **)(iVar1 + 0x5d0c + iVar4 * 4);
        }
        if (puVar2 != (uint *)0x0) {
          *(undefined4 *)(**(int **)(puVar2[0x3c] + 8) + 0x9c) = *(undefined4 *)(iVar1 + 0x5cc4);
          puVar2[0xc1] = 0xffffffff;
          FUN_00456be0(local_78 + 2);
          FUN_00456e50(&LAB_00508030);
          FUN_004575b0(puVar2,local_78 + 1);
          FUN_00456cf0(local_78 + 2);
          return 1;
        }
      }
    }
  }
  return 0;
}



/* ===== physHelperA @ 00507d80  size=64 ===== */

/* shared by physStepWheels/physStep */

void __cdecl physHelperA(uint *param_1,int param_2)

{
  if (((*param_1 & 0x40000000) != 0) && ((param_1[2] & 7) != 0)) {
    param_1[0xc1] = *(uint *)(param_2 + (param_1[2] & 7) * 4);
    FUN_00457190(param_1,0x20);
  }
  FUN_0044f780((int)param_1,physHelperA,param_2);
  return;
}



/* ===== physHelperB @ 005080b0  size=101 ===== */

/* shared by physStepWheels/physStep */

undefined4 __cdecl physHelperB(int param_1,int param_2,int param_3,int param_4,uint param_5)

{
  uint uVar1;
  
  *(uint *)(param_1 + 4) = param_2 << 0x18 | 0xffffff;
  *(uint *)(param_1 + 0xc) = param_4 << 0x18 | 0xffffff;
  uVar1 = param_5 & 0xff;
  *(uint *)(param_1 + 8) = param_3 << 0x18 | 0xffffff;
  *(uint *)(param_1 + 0x10) = ((uVar1 << 8 | uVar1) << 8 | uVar1) << 8 | uVar1;
  return 1;
}



/* ===== physSuspension @ 005091b0  size=262 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* reaches physSuspensionInner */

undefined4 __cdecl physSuspension(int param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  float *pfVar3;
  int iVar4;
  undefined4 uVar5;
  float local_c;
  float local_8;
  float local_4;
  
  pfVar3 = param_2;
  *param_2 = DAT_0070c658;
  uVar5 = 0;
  param_2[1] = DAT_0070c65c;
  param_2[2] = DAT_0070c660;
  iVar4 = physSuspensionInner(param_1);
  if (iVar4 == 0) {
    return 0;
  }
  FUN_0040cbf0(param_1 + 0xf8,&local_c);
  iVar4 = *(int *)(param_1 + 0xe8);
  param_2 = (float *)0x0;
  fVar2 = _DAT_005543a0;
  if ((*(byte *)(iVar4 + 0x575c) & 1) == 0) {
    if ((*(byte *)(iVar4 + 0x5764) & 1) == 0) goto LAB_00509281;
    fVar1 = *(float *)(iVar4 + 0x5768);
    param_2 = (float *)0x3f800000;
  }
  else {
    fVar1 = *(float *)(iVar4 + 0x5760);
    param_2 = (float *)0xbf800000;
  }
  uVar5 = 1;
  if ((_DAT_005543a0 <= fVar1) && (fVar2 = fVar1, _DAT_00554390 < fVar1)) {
    fVar2 = _DAT_00554390;
  }
LAB_00509281:
  fVar2 = (fVar2 + _DAT_00554390) * _DAT_014c4b84 * (float)param_2;
  *pfVar3 = local_c * fVar2;
  pfVar3[1] = local_8 * fVar2;
  pfVar3[2] = local_4 * fVar2;
  return uVar5;
}



/* ===== physSuspensionInner @ 005092c0  size=415 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __cdecl physSuspensionInner(int param_1)

{
  int iVar1;
  float fVar2;
  float10 fVar3;
  float local_30 [3];
  float local_24 [2];
  float local_1c;
  float local_18 [3];
  float local_c [3];
  
  FUN_0040cbd0(param_1 + 0xf8,local_c);
  FUN_0040cbf0(param_1 + 0xf8,local_24);
  iVar1 = *(int *)(param_1 + 0xe8);
  if ((SQRT(*(float *)(iVar1 + 0x58e0) * *(float *)(iVar1 + 0x58e0) +
            *(float *)(iVar1 + 0x58e4) * *(float *)(iVar1 + 0x58e4) +
            *(float *)(iVar1 + 0x58e8) * *(float *)(iVar1 + 0x58e8)) <= _DAT_005544a0) &&
     (SQRT(*(float *)(iVar1 + 0x58e0) * *(float *)(iVar1 + 0x58e0) +
           *(float *)(iVar1 + 0x58e4) * *(float *)(iVar1 + 0x58e4) +
           *(float *)(iVar1 + 0x58e8) * *(float *)(iVar1 + 0x58e8)) <= _DAT_005547f8)) {
    local_30[2] = local_1c;
    local_30[0] = local_24[0];
    local_30[1] = 0.0;
    fVar2 = SQRT(local_24[0] * local_24[0] + local_1c * local_1c);
    if (_DAT_00554380 <= fVar2) {
      if (_DAT_00554380 <= ABS(fVar2 - _DAT_00554390)) {
        local_30[0] = local_24[0] * (_DAT_00554390 / fVar2);
        local_30[2] = local_1c * (_DAT_00554390 / fVar2);
      }
      local_30[1] = 0.0;
      fVar3 = FUN_00410130(local_24,local_30);
      if ((float10)_DAT_005547f0 < fVar3) {
        return 0;
      }
    }
    FUN_004159b0(local_24,(float *)&DAT_0055e9a0,local_18);
    fVar3 = FUN_00410130(local_c,local_18);
    if ((float10)_DAT_00554818 <= fVar3) {
      return 1;
    }
  }
  return 0;
}



/* ===== physWheelPoints @ 0050e020  size=139 ===== */

/* transforms contact points by orientation */

void __cdecl physWheelPoints(int param_1,int *param_2,int param_3)

{
  float fVar1;
  float *pfVar2;
  int iVar3;
  float *pfVar4;
  float *pfVar5;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  FUN_0040cbb0(param_1 + 0xf8,&local_c);
  FUN_0040cbd0(param_1 + 0xf8,&local_18);
  iVar3 = 0;
  *param_2 = 3;
  pfVar5 = (float *)&DAT_014c4bdc;
  pfVar4 = (float *)&DAT_014c4bb0;
  pfVar2 = (float *)(param_3 + 4);
  do {
    iVar3 = iVar3 + 1;
    pfVar2[2] = *pfVar4;
    pfVar4 = pfVar4 + 1;
    fVar1 = *pfVar5;
    pfVar5 = pfVar5 + 3;
    pfVar2[-1] = local_18 * fVar1 + local_c;
    *pfVar2 = local_14 * fVar1 + local_8;
    pfVar2[1] = local_10 * fVar1 + local_4;
    pfVar2 = pfVar2 + 4;
  } while (iVar3 < *param_2);
  return;
}



