/* ===== FUN_004f7450 @ 004f7450  size=26  refs=2 ===== */

undefined4 * FUN_004f7450(int param_1)

{
  undefined4 *puVar1;
  
  if ((param_1 < 0) || (puVar1 = &DAT_014c4b90 + param_1 * 2, 2 < param_1)) {
    puVar1 = &DAT_014c4b90;
  }
  return puVar1;
}



/* ===== FUN_004f7760 @ 004f7760  size=214  refs=1 ===== */

void FUN_004f7760(int param_1)

{
  if (DAT_014c4f1c != 0) {
    FUN_00404950(3,s_MAX_IMPULSE_00571edc,s_maxImpAng_00571ee8,&DAT_00571d28);
    FUN_00404950(3,s_MAX_IMPULSE_00571edc,s_maxImpLinear_00571ecc,&DAT_00571d2c);
    FUN_00404950(3,s_MAX_IMPULSE_00571edc,s_coeffPeople_00571ec0,&DAT_00571d30);
    FUN_004f7840();
    FUN_004f79a0(param_1 != 0);
    FUN_004f7ea0(param_1 != 0);
    FUN_004f7fa0();
    FUN_004f8400();
    FUN_004f91b0();
    FUN_004f9080();
    FUN_004f74f0();
    FUN_004f9da0();
    FUN_004fa9c0();
    FUN_004facb0();
    FUN_004fb050();
    FUN_004fa5f0();
    FUN_004fa2e0();
    FUN_004fa760();
    FUN_004fa160();
    FUN_004f98e0();
    FUN_004f9aa0();
    FUN_00404950(3,&PTR_DAT_00571ea8,s_momentFloorBackABS_00571eac,&DAT_014c4b84);
  }
  return;
}



/* ===== FUN_004f7fa0 @ 004f7fa0  size=1114  refs=1 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004f7fa0(void)

{
  float fVar1;
  int iVar2;
  float *pfVar3;
  int local_3c;
  float local_38;
  undefined1 local_34 [52];
  
  iVar2 = 0;
  pfVar3 = (float *)&DAT_014c4aec;
  do {
    iVar2 = iVar2 + 1;
    FUN_0054603b(local_34,s_Tires_d_005720f8,iVar2);
    FUN_00404770(3,local_34,s_CoeffFrontTires_005720e8,&local_3c);
    local_38 = (float)local_3c;
    fVar1 = _DAT_00554424;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar1 = _DAT_00554390, local_38 <= _DAT_00554490))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554a00))) {
      fVar1 = (local_38 - _DAT_00554390) * _DAT_005549f8 + _DAT_00554424;
    }
    pfVar3[-1] = fVar1;
    FUN_00404770(3,local_34,s_CoeffRearTires_005720d8,&local_3c);
    local_38 = (float)local_3c;
    fVar1 = _DAT_00554424;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar1 = _DAT_00554390, local_38 <= _DAT_00554490))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554a00))) {
      fVar1 = (local_38 - _DAT_00554390) * _DAT_005549f8 + _DAT_00554424;
    }
    *pfVar3 = fVar1;
    pfVar3[1] = fVar1;
    FUN_00404770(3,local_34,s_angleDriftOn_005720c8,&local_3c);
    local_38 = (float)local_3c;
    fVar1 = _DAT_00554390;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar1 = _DAT_005543e4, local_38 <= _DAT_005543e4))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_005549f0))) {
      fVar1 = (local_38 - _DAT_00554390) + _DAT_00554390;
    }
    pfVar3[2] = fVar1;
    FUN_00404770(3,local_34,s_speedDriftOn_005720b8,&local_3c);
    local_38 = (float)local_3c;
    fVar1 = _DAT_00554390;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar1 = _DAT_005549a4, local_38 <= _DAT_005549a4))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_005549e8))) {
      fVar1 = (local_38 - _DAT_00554390) * _DAT_005549e0 + _DAT_00554390;
    }
    pfVar3[3] = fVar1;
    FUN_00404770(3,local_34,s_coeffDriftRear_005720a8,&local_3c);
    local_38 = (float)local_3c;
    fVar1 = _DAT_00554390;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar1 = _DAT_00554424, local_38 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = _DAT_00554390 - (local_38 - _DAT_00554390) * _DAT_005549dc;
    }
    pfVar3[4] = fVar1;
    FUN_00404770(3,local_34,s_coeffDeepSand_00572098,&local_3c);
    local_38 = (float)local_3c;
    fVar1 = _DAT_005543a0;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar1 = _DAT_00554390, local_38 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = (local_38 - _DAT_00554390) * _DAT_00554910;
    }
    pfVar3[5] = fVar1;
    FUN_00404770(3,local_34,s_coeffWater_0057208c,&local_3c);
    local_38 = (float)local_3c;
    fVar1 = _DAT_005543a0;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar1 = _DAT_00554390, local_38 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = (local_38 - _DAT_00554390) * _DAT_00554910;
    }
    pfVar3[6] = fVar1;
    FUN_00404950(3,local_34,s_SpeedAngMaxREL_0057207c,pfVar3 + 0xb);
    pfVar3 = pfVar3 + 0xd;
  } while ((int)pfVar3 < 0x14c4b88);
  DAT_014c4f20 = FUN_00487580(s_UPGRADES_ini_00571ff4);
  if (((DAT_014c4f20 != 0) &&
      (iVar2 = FUN_0043cb90(DAT_014c4f20,s_TIRES_00572074,s_OverkillUpgrades_00571fe0,4,
                            &DAT_014c4b08), iVar2 != -1)) &&
     (iVar2 = FUN_0043cb90(DAT_014c4f20,s_TIRES_00572074,s_BuggyUpgrades_00571fac,4,&DAT_014c4b3c),
     iVar2 != -1)) {
    FUN_0043cb90(DAT_014c4f20,s_TIRES_00572074,s_HummerUpgrades_00571f84,4,&DAT_014c4b70);
  }
  return;
}



/* ===== FUN_004f9080 @ 004f9080  size=303  refs=4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004f9080(void)

{
  float fVar1;
  float *pfVar2;
  int iVar3;
  int local_38;
  undefined1 local_34 [52];
  
  iVar3 = 0;
  pfVar2 = (float *)&DAT_014c4b94;
  do {
    iVar3 = iVar3 + 1;
    FUN_0054603b(local_34,s_RESISTANCE_d_0057228c,iVar3);
    FUN_00404770(3,local_34,s_coeffFrictionBearings_00572274,&local_38);
    fVar1 = _DAT_005543a0;
    if (((_DAT_00554390 <= (float)local_38) &&
        (fVar1 = _DAT_00554490, (float)local_38 <= _DAT_00554474)) &&
       (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = ((float)local_38 - _DAT_00554390) * _DAT_00554a18;
    }
    pfVar2[-1] = fVar1;
    FUN_00404770(3,local_34,s_coeffAirResistance_00572260,&local_38);
    fVar1 = _DAT_005543a0;
    if (((_DAT_00554390 <= (float)local_38) &&
        (fVar1 = _DAT_00554424, (float)local_38 <= _DAT_00554474)) &&
       (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = ((float)local_38 - _DAT_00554390) * _DAT_00554a14;
    }
    *pfVar2 = fVar1;
    pfVar2 = pfVar2 + 2;
  } while ((int)pfVar2 < 0x14c4bac);
  return;
}



/* ===== FUN_004f9aa0 @ 004f9aa0  size=756  refs=3 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004f9aa0(void)

{
  uint local_c;
  uint local_8;
  undefined4 uStack_4;
  
  FUN_00404770(3,s_CarRender_00572454,s_FrameSkipNmb_00572460,&local_c);
  _DAT_014c4600 = local_c;
  FUN_00404770(3,s_CarRender_00572454,s_TextureSizePow2_00572444,&local_c);
  _DAT_014c4a44 = 1 << ((byte)local_c & 0x1f);
  FUN_00404770(3,s_CarRender_00572454,s_ShadowDensity_00572434,&local_c);
  _DAT_014c45f8 = local_c;
  FUN_00404770(3,s_CarRender_00572454,s_FilterSpeed_00572428,&local_c);
  _DAT_014c4b88 = local_c;
  FUN_00404770(3,s_CarRender_00572454,s_ShadowSize1_0057241c,&local_c);
  local_8 = local_c;
  uStack_4 = 0;
  _DAT_014c45ec = (float)local_c * _DAT_005544cc;
  FUN_00404770(3,s_CarRender_00572454,s_ShadowSize2_00572410,&local_c);
  uStack_4 = 0;
  local_8 = local_c;
  _DAT_014c45f0 = (float)local_c * _DAT_005544cc;
  FUN_00404770(3,s_CarRender_00572454,s_ShadowSize3_00572404,&local_c);
  uStack_4 = 0;
  local_8 = local_c;
  _DAT_014c45f4 = (float)local_c * _DAT_005544cc;
  FUN_00404770(3,s_CarRender_00572454,s_ShadowShift1_005723f4,&local_c);
  local_8 = local_c;
  uStack_4 = 0;
  _DAT_014c4858 = (float)local_c * _DAT_005544cc - _DAT_00554384;
  FUN_00404770(3,s_CarRender_00572454,s_ShadowShift2_005723e4,&local_c);
  uStack_4 = 0;
  local_8 = local_c;
  _DAT_014c485c = (float)local_c * _DAT_005544cc - _DAT_00554384;
  FUN_00404770(3,s_CarRender_00572454,s_ShadowShift3_005723d4,&local_c);
  uStack_4 = 0;
  local_8 = local_c;
  _DAT_014c4860 = (float)local_c * _DAT_005544cc - _DAT_00554384;
  FUN_00404770(3,s_CarLight_005723bc,s_AmbientI_005723c8,&local_c);
  DAT_014c46c0 = local_c;
  FUN_00404770(3,s_CarLight_005723bc,s_DirectI_005723b4,&local_c);
  DAT_014c4894 = local_c;
  FUN_00404770(3,s_CarLight_005723bc,s_AngleAzimuth_005723a4,&local_c);
  uStack_4 = 0;
  local_8 = local_c;
  DAT_014c45c8 = (float)local_c;
  FUN_00404770(3,s_CarLight_005723bc,s_AngleElevation_00572394,&local_c);
  uStack_4 = 0;
  local_8 = local_c;
  DAT_014c4a40 = (float)local_c;
  FUN_00404770(3,s_CarEnvMap_0057237c,s_EnvMapGRE1_00572388,&local_c);
  DAT_014c4ba9 = (undefined1)local_c;
  FUN_00404770(3,s_CarEnvMap_0057237c,s_EnvMapGRE2_00572370,&local_c);
  DAT_014c4ba8 = (undefined1)local_c;
  FUN_00404770(3,s_CarEnvMap_0057237c,s_EnvMapBody_00572364,&local_c);
  DAT_014c45cc = (undefined1)local_c;
  FUN_00404770(3,s_CarEnvMap_0057237c,s_EnvMapUpgrades_00572354,&local_c);
  DAT_014c46c4 = (undefined1)local_c;
  return;
}



/* ===== FUN_004fa2e0 @ 004fa2e0  size=781  refs=1 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004fa2e0(void)

{
  uint local_c;
  float local_8;
  undefined4 uStack_4;
  
  FUN_00404770(3,s_BOTTLE_00572538,s_radius_00572540,&local_c);
  uStack_4 = 0;
  local_8 = (float)local_c;
  DAT_014c4bb0 = _DAT_005544a4;
  if (((_DAT_00554390 <= (float)local_c) && (DAT_014c4bb0 = _DAT_00554424, local_8 <= _DAT_00554474)
      ) && (DAT_014c4bb0 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
    DAT_014c4bb0 = (local_8 - _DAT_00554390) * _DAT_00554a10 + _DAT_005544a4;
  }
  _DAT_014c4bd8 = DAT_0070c658;
  _DAT_014c4be0 = DAT_0070c660;
  DAT_014c4bdc = 0.0;
  DAT_014c4bb4 = DAT_014c4bb0;
  _DAT_014c4bb8 = DAT_014c4bb0;
  FUN_00404770(3,s_BOTTLE_00572538,s_shift0_00572530,&local_c);
  uStack_4 = 0;
  _DAT_014c4bd8 = DAT_0070c658;
  _DAT_014c4be0 = DAT_0070c660;
  local_8 = (float)local_c;
  DAT_014c4bdc = _DAT_00554a40;
  if (((_DAT_00554390 <= (float)local_c) && (DAT_014c4bdc = _DAT_00554568, local_8 <= _DAT_00554474)
      ) && (DAT_014c4bdc = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
    DAT_014c4bdc = (local_8 - _DAT_00554390) * _DAT_00554a3c - _DAT_005544f0;
  }
  FUN_00404770(3,s_BOTTLE_00572538,s_shift1_00572528,&local_c);
  uStack_4 = 0;
  _DAT_014c4be4 = DAT_0070c658;
  _DAT_014c4bec = DAT_0070c660;
  local_8 = (float)local_c;
  DAT_014c4be8 = _DAT_00554a40;
  if (((_DAT_00554390 <= (float)local_c) && (DAT_014c4be8 = _DAT_00554568, local_8 <= _DAT_00554474)
      ) && (DAT_014c4be8 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
    DAT_014c4be8 = (local_8 - _DAT_00554390) * _DAT_00554a3c - _DAT_005544f0;
  }
  FUN_00404770(3,s_BOTTLE_00572538,s_shift2_0056b508,&local_c);
  _DAT_014c4bf0 = DAT_0070c658;
  _DAT_014c4bf8 = DAT_0070c660;
  if ((float)local_c < _DAT_00554390) {
    _DAT_014c4bf4 = -0.2;
    return;
  }
  if (_DAT_00554474 < (float)local_c) {
    _DAT_014c4bf4 = 0.4;
    return;
  }
  if ((double)_DAT_00554380 <= ABS(_DAT_00554918)) {
    _DAT_014c4bf4 = ((float)local_c - _DAT_00554390) * _DAT_00554a3c - _DAT_005544f0;
    return;
  }
  _DAT_014c4bf4 = _DAT_005543a0;
  return;
}



/* ===== FUN_00507130 @ 00507130  size=430  refs=2 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00507130(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  uint *puVar4;
  undefined4 uVar5;
  int iVar6;
  undefined4 *puVar7;
  
  iVar1 = *(int *)(param_1 + 0xe8);
  uVar5 = *(undefined4 *)(iVar1 + 0x5cc8);
  uVar2 = *(undefined4 *)(iVar1 + 0x5cd4);
  puVar7 = (undefined4 *)(iVar1 + 0x5ca8);
  for (iVar6 = 0x20; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  *(undefined4 *)(iVar1 + 0x5cc8) = uVar5;
  *(undefined4 *)(iVar1 + 0x5cd4) = uVar2;
  puVar4 = (uint *)FUN_004513e0(*(undefined4 *)(param_1 + 0x10),s_ENVIR_CAR_BODY_body_00570864);
  if (puVar4 == (uint *)0x0) {
    puVar4 = (uint *)FUN_004513e0(*(undefined4 *)(param_1 + 0x10),s_ENVIR_CAR_BODY_00570854);
  }
  *(uint **)(iVar1 + 0x5d08) = puVar4;
  if (puVar4 != (uint *)0x0) {
    puVar4[2] = puVar4[2] | 1;
    *puVar4 = *puVar4 | 0x40000000;
    puVar4[5] = puVar4[5] | 0x4000;
    FUN_00457190(puVar4,0xffffffff);
  }
  FUN_005080b0(&DAT_014edd60,DAT_014c45cc,DAT_014c4ba9,DAT_014c4ba8,DAT_014c46c4);
  FUN_00507d80(*(undefined4 *)(param_1 + 0x10),&DAT_014edd60);
  if (*(int *)(param_1 + 0xe8) != -0xe718) {
    iVar6 = 0;
    do {
      iVar3 = *(int *)(*(int *)(param_1 + 0xe8) + 0xe718 + iVar6 * 4);
      if (iVar3 == 0) {
        puVar4 = (uint *)0x0;
      }
      else {
        puVar4 = (uint *)FUN_004513e0(*(undefined4 *)(iVar3 + 0x10),s_ENVIR_CAR_BODY_body_00570864);
        if (puVar4 == (uint *)0x0) {
          puVar4 = (uint *)FUN_004513e0(*(undefined4 *)(iVar3 + 0x10),s_ENVIR_CAR_BODY_00570854);
        }
      }
      *(uint **)(iVar1 + 0x5d0c + iVar6 * 4) = puVar4;
      uVar5 = FUN_004513e0(*(undefined4 *)(iVar3 + 0x10),s_cyl534_1___pivot_null_00573524);
      *(undefined4 *)(iVar1 + 0x5d1c + iVar6 * 4) = uVar5;
      if ((puVar4 != (uint *)0x0) && (iVar6 == 0)) {
        puVar4[2] = puVar4[2] | 1;
        *puVar4 = *puVar4 | 0x40000000;
        puVar4[5] = puVar4[5] | 0x4000;
        FUN_00457190(puVar4,0xffffffff);
      }
      FUN_00507d80(*(undefined4 *)(iVar3 + 0x10),&DAT_014edd60);
      iVar6 = iVar6 + 1;
    } while (iVar6 < 3);
  }
  _DAT_014edd50 = FUN_0046e180(s_shadows2_00573518);
  _DAT_014edd54 = FUN_0046e180(s_shadows3_0057350c);
  _DAT_014edd58 = FUN_0046e180(s_Bshadows1_005734ff + 1);
  return 1;
}



/* ===== FUN_00507ba0 @ 00507ba0  size=472  refs=2 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00507ba0(int param_1,int param_2,int param_3,int param_4)

{
  int iVar1;
  undefined4 uVar2;
  int local_20;
  undefined4 local_1c;
  float local_18 [3];
  float local_c [3];
  
  iVar1 = FUN_00404770(0,s_Video_00561940,s_VIDEO_EnvMap_00568bbc,&local_20);
  if (((iVar1 != 0) && (local_20 != 0)) && (*(int *)(param_2 + 0xe8) != -0x5ca8)) {
    if (param_4 < 1) {
      local_1c = *(undefined4 *)(*(int *)(param_2 + 0xe8) + 0xe824);
      if ((param_4 == 0) && (param_1 != 0)) {
        FUN_0040cbb0(param_1,local_18);
        FUN_0040cbb0(param_2 + 0xf8,local_c);
      }
      uVar2 = __ftol();
      uVar2 = __ftol(uVar2);
      uVar2 = __ftol(uVar2);
      uVar2 = __ftol(uVar2);
      FUN_005080b0(&DAT_014edd60,uVar2);
      FUN_00507d80(*(undefined4 *)(param_3 + 0x10),&DAT_014edd60);
      return 1;
    }
    return 1;
  }
  return 0;
}



/* ===== FUN_005091b0 @ 005091b0  size=262  refs=1 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_005091b0(int param_1,float *param_2)

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
  iVar4 = FUN_005092c0(param_1);
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



/* ===== FUN_0050e020 @ 0050e020  size=139  refs=2 ===== */

void FUN_0050e020(int param_1,int *param_2,int param_3)

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



