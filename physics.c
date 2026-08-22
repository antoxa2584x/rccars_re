/* ===== FUN_004f79a0 @ 004f79a0  size=1265  callers=1 =====
 *   speedBaseMax
 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004f79a0(int param_1)

{
  float fVar1;
  int iVar2;
  float *pfVar3;
  uint local_5c;
  float local_58;
  uint local_54;
  undefined4 uStack_50;
  uint local_4c;
  undefined4 uStack_48;
  uint local_44;
  undefined4 uStack_40;
  uint local_3c;
  undefined4 uStack_38;
  undefined1 local_34 [52];
  
  iVar2 = 0;
  pfVar3 = (float *)&DAT_014c46d8;
  do {
    iVar2 = iVar2 + 1;
    FUN_0054603b(local_34,s_ENGINE_d_00572044,iVar2);
    FUN_00404770(3,local_34,s_boostSpeedUp_00572034,&local_5c);
    uStack_50 = 0;
    local_54 = local_5c;
    local_58 = (float)local_5c;
    fVar1 = _DAT_00554424;
    if (((_DAT_00554390 <= (float)local_5c) && (fVar1 = _DAT_00554824, local_58 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = (local_58 - _DAT_00554390) * _DAT_005549d4 + _DAT_00554424;
    }
    pfVar3[-2] = fVar1;
    FUN_00404770(3,local_34,s_boostSpeedDown_00572024,&local_5c);
    uStack_48 = 0;
    local_4c = local_5c;
    local_58 = (float)local_5c;
    fVar1 = _DAT_00554390;
    if (((_DAT_00554390 <= (float)local_5c) && (fVar1 = _DAT_00554490, local_58 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = (local_58 - _DAT_00554390) * _DAT_005549d0 + _DAT_00554390;
    }
    pfVar3[-1] = fVar1;
    FUN_00404770(3,local_34,s_speedBaseMax_00572014,&local_5c);
    uStack_40 = 0;
    local_44 = local_5c;
    local_58 = (float)local_5c;
    fVar1 = _DAT_00554390;
    if (((_DAT_00554390 <= (float)local_5c) && (fVar1 = _DAT_00554474, local_58 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = (local_58 - _DAT_00554390) + _DAT_00554390;
    }
    *pfVar3 = fVar1 + _DAT_00554424;
    FUN_00404770(3,local_34,s_speedBoostMax_00572004,&local_5c);
    uStack_38 = 0;
    local_3c = local_5c;
    local_58 = (float)local_5c;
    fVar1 = _DAT_00554390;
    if (((_DAT_00554390 <= (float)local_5c) && (fVar1 = _DAT_00554474, local_58 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = (local_58 - _DAT_00554390) + _DAT_00554390;
    }
    fVar1 = fVar1 + _DAT_00554424;
    pfVar3[1] = fVar1;
    if (ABS(*pfVar3) <= _DAT_00554380) {
      pfVar3[2] = 1.0;
    }
    else {
      pfVar3[2] = fVar1 / *pfVar3;
    }
    pfVar3 = pfVar3 + 0x13;
  } while ((int)pfVar3 < 0x14c47bc);
  DAT_014c4f20 = FUN_00487580(s_UPGRADES_ini_00571ff4);
  if (((DAT_014c4f20 == 0) ||
      (((iVar2 = FUN_0043cb90(DAT_014c4f20,s_RESONATORS_00571fd4,s_OverkillUpgrades_00571fe0,4,
                              &DAT_014c46fc), iVar2 != -1 &&
        (iVar2 = FUN_0043cb90(DAT_014c4f20,s_RESONATORS_00571fd4,s_OverkillUpgrades_SPL_OY_00571fbc,
                              4,&DAT_014c470c), iVar2 != -1)) &&
       ((((iVar2 = FUN_0043cb90(DAT_014c4f20,s_RESONATORS_00571fd4,s_BuggyUpgrades_00571fac,4,
                                &DAT_014c4748), iVar2 != -1 &&
          ((iVar2 = FUN_0043cb90(DAT_014c4f20,s_RESONATORS_00571fd4,s_BuggyUpgrades_SPL_OY_00571f94,
                                 4,&DAT_014c4758), iVar2 != -1 &&
           (iVar2 = FUN_0043cb90(DAT_014c4f20,s_RESONATORS_00571fd4,s_HummerUpgrades_00571f84,4,
                                 &DAT_014c4794), iVar2 != -1)))) &&
         (iVar2 = FUN_0043cb90(DAT_014c4f20,s_RESONATORS_00571fd4,s_HummerUpgrades_SPL_OY_00571f6c,4
                               ,&DAT_014c47a4), iVar2 != -1)) &&
        (((iVar2 = FUN_0043cb90(DAT_014c4f20,s_BOOSTERS_00571f60,s_OverkillUpgrades_00571fe0,4,
                                &DAT_014c46ec), iVar2 != -1 &&
          (iVar2 = FUN_0043cb90(DAT_014c4f20,s_BOOSTERS_00571f60,s_BuggyUpgrades_00571fac,4,
                                &DAT_014c4738), iVar2 != -1)) &&
         (iVar2 = FUN_0043cb90(DAT_014c4f20,s_BOOSTERS_00571f60,s_HummerUpgrades_00571f84,4,
                               &DAT_014c4784), iVar2 != -1)))))))) && (param_1 != 0)) {
    FUN_0054603b(local_34,s_OVERKILL_00571f54);
    _DAT_014c46e8 = FUN_00488030(local_34);
    FUN_0054603b(local_34,s_BUGGY_00571f4c);
    _DAT_014c4734 = FUN_00488030(local_34);
    FUN_0054603b(local_34,s_HUMMER_00571f44);
    _DAT_014c4780 = FUN_00488030(local_34);
    FUN_0054603b(local_34,s_OVERKILL_RESTRICT_00571f30);
    _DAT_014c46e4 = FUN_00488030(local_34);
    FUN_0054603b(local_34,s_BUGGY_RESTRICT_00571f20);
    _DAT_014c4730 = FUN_00488030(local_34);
    FUN_0054603b(local_34,s_HUMMER_RESTRICT_00571f10);
    _DAT_014c477c = FUN_00488030(local_34);
  }
  return;
}



/* ===== FUN_004f7ea0 @ 004f7ea0  size=246  callers=1 =====
 *   angleSteer
 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004f7ea0(int param_1)

{
  float fVar1;
  undefined4 uVar2;
  float *pfVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  uint local_40;
  uint local_3c;
  undefined4 uStack_38;
  undefined1 local_34 [52];
  
  iVar6 = 0;
  iVar4 = 0;
  pfVar3 = (float *)&DAT_014c486c;
  do {
    iVar4 = iVar4 + 1;
    FUN_0054603b(local_34,s_STEERING_d_00572068,iVar4);
    FUN_00404770(3,local_34,s_angleSteer_0057205c,&local_40);
    uStack_38 = 0;
    local_3c = local_40;
    fVar1 = _DAT_005543a0;
    if (((_DAT_00554390 <= (float)local_40) &&
        (fVar1 = _DAT_00554474, (float)local_40 <= _DAT_00554474)) &&
       (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = ((float)local_40 - _DAT_00554390) * _DAT_005549d8;
    }
    *pfVar3 = fVar1;
    pfVar3 = pfVar3 + 2;
  } while ((int)pfVar3 < 0x14c4884);
  if (param_1 != 0) {
    puVar5 = &DAT_014c4868;
    do {
      iVar6 = iVar6 + 1;
      FUN_0054603b(local_34,s_steering_d_00572050,iVar6);
      uVar2 = FUN_00488030(local_34);
      *puVar5 = uVar2;
      puVar5 = puVar5 + 2;
    } while ((int)puVar5 < 0x14c4880);
  }
  return;
}



/* ===== FUN_004f7fa0 @ 004f7fa0  size=1114  callers=1 =====
 *   coeffDriftRear
 *   CoeffFrontTires
 */

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



/* ===== FUN_004f8400 @ 004f8400  size=3074  callers=1 =====
 *   CenterMassOY
 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004f8400(void)

{
  int iVar1;
  float fVar2;
  int iVar3;
  float10 fVar4;
  int local_3c;
  float local_38;
  undefined1 local_34 [52];
  
  iVar3 = 0;
  do {
    iVar1 = iVar3 + 1;
    FUN_0054603b(local_34,s_CDT_d_00572258,iVar1);
    if (iVar3 == 2) {
      _DAT_014c4e68 = 6;
    }
    else {
      (&DAT_014c4d00)[iVar3 * 0x2d] = 4;
    }
    FUN_00404770(3,local_34,s_CdtRadWheel_0057224c,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_005544a4;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_00554424, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a10 + _DAT_005544a4;
    }
    (&DAT_014c4d04)[iVar3 * 0x2d] = fVar2;
    (&DAT_014c4d08)[iVar3 * 0x2d] = fVar2;
    FUN_00404770(3,local_34,s_coeffRadBackWheels_00572238,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_00554390;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543c8, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554910 + _DAT_00554390;
    }
    fVar2 = fVar2 * (float)(&DAT_014c4d04)[iVar3 * 0x2d];
    (&DAT_014c4d0c)[iVar3 * 0x2d] = fVar2;
    (&DAT_014c4d10)[iVar3 * 0x2d] = fVar2;
    (&DAT_014c4d14)[iVar3 * 0x2d] = fVar2;
    (&DAT_014c4d18)[iVar3 * 0x2d] = fVar2;
    FUN_00404770(3,local_34,s_CdtRadUp_0057222c,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_00554424;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a0c + _DAT_00554424;
    }
    (&DAT_014c4d2c)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaX_00572220,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_0055449c;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a08 - _DAT_005543f4;
    }
    (&DAT_014c4d48)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaY_00572214,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_0055449c;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a08 - _DAT_005543f4;
    }
    (&DAT_014c4d4c)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaZ_00572208,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_0055449c;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a08 - _DAT_005543f4;
    }
    (&DAT_014c4d50)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaZ_CENTRAL_005721f4,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_0055449c;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a08 - _DAT_005543f4;
    }
    (&DAT_014c4d80)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtRadUp2_005721e8,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_00554424;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a0c + _DAT_00554424;
    }
    (&DAT_014c4d30)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaX2_005721dc,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_0055449c;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a08 - _DAT_005543f4;
    }
    (&DAT_014c4d54)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaY2_005721d0,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_0055449c;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a08 - _DAT_005543f4;
    }
    (&DAT_014c4d58)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaZ2_005721c4,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_0055449c;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a08 - _DAT_005543f4;
    }
    (&DAT_014c4d5c)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaZ_CENTRAL2_005721b0,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_0055449c;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a08 - _DAT_005543f4;
    }
    (&DAT_014c4d8c)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtRadUp3_005721a4,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_00554424;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a0c + _DAT_00554424;
    }
    (&DAT_014c4d34)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaX3_00572198,&local_3c);
    local_38 = (float)local_3c;
    fVar2 = _DAT_0055449c;
    if (((_DAT_00554390 <= (float)local_3c) && (fVar2 = _DAT_005543f4, local_38 <= _DAT_00554474))
       && (fVar2 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar2 = (local_38 - _DAT_00554390) * _DAT_00554a08 - _DAT_005543f4;
    }
    (&DAT_014c4d60)[iVar3 * 0x2d] = fVar2 * _DAT_00554424;
    FUN_00404770(3,local_34,s_CdtDeltaY3_0057218c,&local_3c);
    local_38 = (float)local_3c;
    if (_DAT_00554390 <= (float)local_3c) {
      if (local_38 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0xc0400000,0x40400000,0x3f800000,0x42c80000,local_38);
      }
      else {
        fVar4 = (float10)_DAT_005543f4;
      }
    }
    else {
      fVar4 = (float10)_DAT_0055449c;
    }
    (&DAT_014c4d64)[iVar3 * 0x2d] = (float)(fVar4 * (float10)_DAT_00554424);
    FUN_00404770(3,local_34,s_CdtDeltaZ3_00572180,&local_3c);
    local_38 = (float)local_3c;
    if (_DAT_00554390 <= (float)local_3c) {
      if (local_38 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0xc0400000,0x40400000,0x3f800000,0x42c80000,local_38);
      }
      else {
        fVar4 = (float10)_DAT_005543f4;
      }
    }
    else {
      fVar4 = (float10)_DAT_0055449c;
    }
    (&DAT_014c4d68)[iVar3 * 0x2d] = (float)(fVar4 * (float10)_DAT_00554424);
    FUN_00404770(3,local_34,s_CdtDeltaZ_CENTRAL3_0057216c,&local_3c);
    local_38 = (float)local_3c;
    if (_DAT_00554390 <= (float)local_3c) {
      if (local_38 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0xc0400000,0x40400000,0x3f800000,0x42c80000,local_38);
      }
      else {
        fVar4 = (float10)_DAT_005543f4;
      }
    }
    else {
      fVar4 = (float10)_DAT_0055449c;
    }
    (&DAT_014c4d98)[iVar3 * 0x2d] = (float)(fVar4 * (float10)_DAT_00554424);
    FUN_00404770(3,local_34,s_CdtRadUp4_00572160,&local_3c);
    local_38 = (float)local_3c;
    if (_DAT_00554390 <= (float)local_3c) {
      if (local_38 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0x3dcccccd,0x40400000,0x3f800000,0x42c80000,local_38);
      }
      else {
        fVar4 = (float10)_DAT_005543f4;
      }
    }
    else {
      fVar4 = (float10)_DAT_00554424;
    }
    (&DAT_014c4d38)[iVar3 * 0x2d] = (float)(fVar4 * (float10)_DAT_00554424);
    FUN_00404770(3,local_34,s_CdtDeltaX4_00572154,&local_3c);
    local_38 = (float)local_3c;
    if (_DAT_00554390 <= (float)local_3c) {
      if (local_38 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0xc0400000,0x40400000,0x3f800000,0x42c80000,local_38);
      }
      else {
        fVar4 = (float10)_DAT_005543f4;
      }
    }
    else {
      fVar4 = (float10)_DAT_0055449c;
    }
    (&DAT_014c4d6c)[iVar3 * 0x2d] = (float)(fVar4 * (float10)_DAT_00554424);
    FUN_00404770(3,local_34,s_CdtDeltaY4_00572148,&local_3c);
    local_38 = (float)local_3c;
    if (_DAT_00554390 <= (float)local_3c) {
      if (local_38 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0xc0400000,0x40400000,0x3f800000,0x42c80000,local_38);
      }
      else {
        fVar4 = (float10)_DAT_005543f4;
      }
    }
    else {
      fVar4 = (float10)_DAT_0055449c;
    }
    (&DAT_014c4d70)[iVar3 * 0x2d] = (float)(fVar4 * (float10)_DAT_00554424);
    FUN_00404770(3,local_34,s_CdtDeltaZ4_0057213c,&local_3c);
    local_38 = (float)local_3c;
    if (_DAT_00554390 <= (float)local_3c) {
      if (local_38 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0xc0400000,0x40400000,0x3f800000,0x42c80000,local_38);
      }
      else {
        fVar4 = (float10)_DAT_005543f4;
      }
    }
    else {
      fVar4 = (float10)_DAT_0055449c;
    }
    (&DAT_014c4d74)[iVar3 * 0x2d] = (float)(fVar4 * (float10)_DAT_00554424);
    FUN_00404770(3,local_34,s_CenterMassOY_0057212c,&local_3c);
    fVar4 = (float10)FUN_004f9010(0,0x3e4ccccd,0x3f800000,0x42c80000,(float)local_3c);
    (&DAT_014c4da8)[iVar3 * 0x2d] = (float)fVar4;
    FUN_00404950(3,local_34,s_shiftRoofBaseLefter_00572118,&DAT_014c4dac + iVar3 * 0xb4);
    FUN_00404950(3,local_34,s_shiftRoofBaseRighter_00572100,&DAT_014c4db0 + iVar3 * 0xb4);
    iVar3 = iVar1;
  } while (iVar1 < 3);
  return;
}



/* ===== FUN_004f9080 @ 004f9080  size=303  callers=1 =====
 *   coeffAirResistance
 */

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



/* ===== FUN_004f91b0 @ 004f91b0  size=1840  callers=1 =====
 *   coeffMomentOZ
 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004f91b0(void)

{
  float fVar1;
  int iVar2;
  float *pfVar3;
  float10 fVar4;
  uint local_a4;
  float local_a0;
  uint local_9c;
  undefined4 uStack_98;
  uint local_94;
  undefined4 uStack_90;
  uint local_8c;
  undefined4 uStack_88;
  uint local_84;
  undefined4 uStack_80;
  uint local_7c;
  undefined4 uStack_78;
  uint local_74;
  undefined4 uStack_70;
  uint local_6c;
  undefined4 uStack_68;
  uint local_64;
  undefined4 uStack_60;
  uint local_5c;
  undefined4 uStack_58;
  uint local_54;
  undefined4 uStack_50;
  uint local_4c;
  undefined4 uStack_48;
  uint local_44;
  undefined4 uStack_40;
  uint local_3c;
  undefined4 uStack_38;
  undefined1 local_34 [52];
  
  iVar2 = 0;
  pfVar3 = (float *)&DAT_014c4c60;
  do {
    iVar2 = iVar2 + 1;
    FUN_0054603b(local_34,s_Springs_d_00572320,iVar2);
    FUN_00404770(3,local_34,s_length_00572318,&local_a4);
    uStack_68 = 0;
    local_6c = local_a4;
    local_a0 = (float)local_a4;
    fVar1 = _DAT_005544a4;
    if (((_DAT_00554390 <= (float)local_a4) && (fVar1 = _DAT_00554384, local_a0 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = (local_a0 - _DAT_00554390) * _DAT_00554a24 + _DAT_005544a4;
    }
    *pfVar3 = fVar1;
    FUN_00404770(3,local_34,s_lenMin_00572310,&local_a4);
    uStack_88 = 0;
    pfVar3[1] = *pfVar3;
    local_8c = local_a4;
    local_a0 = (float)local_a4;
    fVar1 = _DAT_00554424;
    if (((_DAT_00554390 <= (float)local_a4) && (fVar1 = _DAT_00554768, local_a0 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = (local_a0 - _DAT_00554390) * _DAT_00554a20 + _DAT_00554424;
    }
    pfVar3[1] = *pfVar3 * fVar1;
    FUN_00404770(3,local_34,s_lenMax_00572308,&local_a4);
    uStack_58 = 0;
    pfVar3[2] = *pfVar3;
    local_5c = local_a4;
    local_a0 = (float)local_a4;
    fVar1 = _DAT_00554518;
    if (((_DAT_00554390 <= (float)local_a4) && (fVar1 = _DAT_005543c8, local_a0 <= _DAT_00554474))
       && (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = (local_a0 - _DAT_00554390) * _DAT_00554a1c + _DAT_00554518;
    }
    pfVar3[2] = *pfVar3 * fVar1;
    FUN_00404770(3,local_34,&DAT_00572300,&local_a4);
    local_a0 = *pfVar3 - pfVar3[1];
    uStack_78 = 0;
    local_7c = local_a4;
    pfVar3[3] = local_a0;
    fVar1 = _DAT_00554424;
    if (((_DAT_00554390 <= (float)local_a4) &&
        (fVar1 = _DAT_00554768, (float)local_a4 <= _DAT_00554474)) &&
       (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = ((float)local_a4 - _DAT_00554390) * _DAT_00554a20 + _DAT_00554424;
    }
    pfVar3[3] = local_a0 * fVar1;
    FUN_00404770(3,local_34,s_kSpeed_005722f8,&local_a4);
    uStack_38 = 0;
    local_3c = local_a4;
    fVar1 = _DAT_00554390;
    if (((_DAT_00554390 <= (float)local_a4) &&
        (fVar1 = _DAT_00554490, (float)local_a4 <= _DAT_00554474)) &&
       (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
      fVar1 = ((float)local_a4 - _DAT_00554390) * _DAT_005549d0 + _DAT_00554390;
    }
    pfVar3[5] = fVar1;
    FUN_00404770(3,local_34,&DAT_00564848,&local_a4);
    uStack_98 = 0;
    local_9c = local_a4;
    if (_DAT_00554390 <= (float)local_a4) {
      if ((float)local_a4 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0,0x3f800000,0x3f800000,0x42c80000,(float)local_a4);
      }
      else {
        fVar4 = (float10)_DAT_00554390;
      }
    }
    else {
      fVar4 = (float10)_DAT_005543a0;
    }
    pfVar3[6] = (float)fVar4;
    FUN_00404770(3,local_34,s_touchPoint_005722ec,&local_a4);
    uStack_48 = 0;
    local_4c = local_a4;
    if (_DAT_00554390 <= (float)local_a4) {
      if ((float)local_a4 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0,0x3f800000,0x3f800000,0x42c80000,(float)local_a4);
      }
      else {
        fVar4 = (float10)_DAT_00554390;
      }
    }
    else {
      fVar4 = (float10)_DAT_005543a0;
    }
    pfVar3[7] = (float)fVar4;
    FUN_00404770(3,local_34,&DAT_005722e8,&local_a4);
    uStack_90 = 0;
    local_94 = local_a4;
    if (_DAT_00554390 <= (float)local_a4) {
      if ((float)local_a4 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0,0x3f800000,0x3f800000,0x42c80000,(float)local_a4);
      }
      else {
        fVar4 = (float10)_DAT_00554390;
      }
    }
    else {
      fVar4 = (float10)_DAT_005543a0;
    }
    pfVar3[8] = (float)fVar4;
    FUN_00404770(3,local_34,s_coeffMomentOX_005722d8,&local_a4);
    uStack_80 = 0;
    local_84 = local_a4;
    if (_DAT_00554390 <= (float)local_a4) {
      if ((float)local_a4 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0x3f000000,0x40000000,0x3f800000,0x42c80000,(float)local_a4);
      }
      else {
        fVar4 = (float10)_DAT_005543c8;
      }
    }
    else {
      fVar4 = (float10)_DAT_00554384;
    }
    pfVar3[-2] = (float)fVar4;
    FUN_00404770(3,local_34,s_coeffMomentOZ_005722c8,&local_a4);
    uStack_70 = 0;
    local_74 = local_a4;
    if (_DAT_00554390 <= (float)local_a4) {
      if ((float)local_a4 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0x3f800000,0x40400000,0x3f800000,0x42c80000,(float)local_a4);
      }
      else {
        fVar4 = (float10)_DAT_005543f4;
      }
    }
    else {
      fVar4 = (float10)_DAT_00554390;
    }
    pfVar3[-1] = (float)fVar4;
    FUN_00404770(3,local_34,s_lenAxe_005722c0,&local_a4);
    uStack_60 = 0;
    local_64 = local_a4;
    if (_DAT_00554390 <= (float)local_a4) {
      if ((float)local_a4 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0x3d4ccccd,0x3e4ccccd,0x3f800000,0x42c80000,(float)local_a4);
      }
      else {
        fVar4 = (float10)_DAT_005544f0;
      }
    }
    else {
      fVar4 = (float10)_DAT_00554514;
    }
    pfVar3[9] = (float)fVar4;
    FUN_00404770(3,local_34,s_angleProcUpMax_005722b0,&local_a4);
    uStack_50 = 0;
    local_54 = local_a4;
    if (_DAT_00554390 <= (float)local_a4) {
      if ((float)local_a4 <= _DAT_00554474) {
        fVar4 = (float10)FUN_0040ba70(0,0x42b40000,0x3f800000,0x42c80000,(float)local_a4);
      }
      else {
        fVar4 = (float10)_DAT_0055448c;
      }
    }
    else {
      fVar4 = (float10)_DAT_005543a0;
    }
    pfVar3[10] = (float)fVar4;
    FUN_00404770(3,local_34,s_angleProcDownMax_0057229c,&local_a4);
    uStack_40 = 0;
    local_44 = local_a4;
    fVar4 = (float10)FUN_004f9010(0,0x42b40000,0x3f800000,0x42c80000,(float)local_a4);
    pfVar3[0xb] = (float)fVar4;
    pfVar3 = pfVar3 + 0xe;
  } while ((int)pfVar3 < 0x14c4d08);
  return;
}



/* ===== FUN_004fcce0 @ 004fcce0  size=691  callers=0 =====
 *   Car can't create physics subsystem\n
 */

undefined4 FUN_004fcce0(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  undefined1 local_34 [52];
  
  iVar3 = FUN_00402ab0(1,0x123a4);
  iVar2 = param_2;
  iVar6 = param_1;
  if (iVar3 == 0) {
    FUN_00401220(s_Car_can_t_alocate_memory_00572720);
    return 0;
  }
  *(int *)(param_1 + 0xe8) = iVar3;
  *(int *)(iVar3 + 0x14) = param_1;
  *(undefined4 *)(iVar3 + 0x5d3c) = *(undefined4 *)(param_2 + 0xe0);
  iVar3 = FUN_00448f40(param_1,s_Geometry_0057270c,s_typeCar_00572718,&param_1);
  if (iVar3 == -1) {
    *(undefined4 *)(*(int *)(iVar6 + 0xe8) + 0x54) = 0;
  }
  else {
    *(int *)(*(int *)(iVar6 + 0xe8) + 0x54) = param_1;
  }
  iVar3 = 1;
  *(undefined4 *)(*(int *)(iVar6 + 0xe8) + 0xe714) = 3;
  do {
    FUN_0054603b(local_34,s_Car_dLOD_d_00572700,*(int *)(*(int *)(iVar6 + 0xe8) + 0x54) + 1,iVar3);
    iVar4 = FUN_0048e630(0,DAT_0148f908,local_34);
    if (iVar4 == 0) {
      FUN_00401220(s_Car_can_t_find_proto_for_LOD_00572628);
      return 0;
    }
    uVar5 = FUN_00452300(DAT_0148f908,iVar4,0);
    *(undefined4 *)(*(int *)(iVar6 + 0xe8) + 0xe714 + iVar3 * 4) = uVar5;
    if (*(int *)(*(int *)(iVar6 + 0xe8) + 0xe714 + iVar3 * 4) == 0) {
      FUN_00401220(s_Car_can_t_create_instance_of_LOD_00572604);
      return 0;
    }
    if (iVar3 == 1) {
      FUN_004507e0(*(undefined4 *)(*(int *)(*(int *)(iVar6 + 0xe8) + 0xe718) + 0x10),0);
      FUN_00448d50(*(undefined4 *)(*(int *)(iVar6 + 0xe8) + 0xe718),1);
      FUN_00448d50(*(undefined4 *)(*(int *)(iVar6 + 0xe8) + 0xe718),5);
      FUN_00450b80(*(undefined4 *)(*(int *)(*(int *)(iVar6 + 0xe8) + 0xe718) + 0x10),0);
      iVar4 = FUN_004513e0(*(undefined4 *)(*(int *)(*(int *)(iVar6 + 0xe8) + 0xe718) + 0x10),
                           s_ENVIR_CAR_BODY_body_00570864);
      if (iVar4 == 0) {
        iVar4 = FUN_004513e0(*(undefined4 *)(*(int *)(*(int *)(iVar6 + 0xe8) + 0xe718) + 0x10),
                             s_ENVIR_CAR_BODY_00570854);
        if (iVar4 != 0) goto LAB_004fceb1;
        iVar4 = *(int *)(*(int *)(iVar6 + 0xe8) + 0xe718);
        FUN_00401270(5,s_CAR___s____s___can_t_find_car_ob_005726b4,iVar4 + 0x14,
                     *(int *)(iVar4 + 0xe4) + 0xc);
      }
      else {
LAB_004fceb1:
        iVar1 = *(int *)(*(int *)(iVar6 + 0xe8) + 0xe718);
        FUN_00401270(5,s_object_with_environment_map_to_b_0057266c,iVar1 + 0x14,
                     *(int *)(iVar1 + 0xe4) + 0xc);
        if (iVar4 != 0) {
          *(uint *)(iVar4 + 0x304) = (uint)DAT_014c45cc << 0x18 | 0xffffff;
        }
      }
      FUN_004516b0(*(undefined4 *)(*(int *)(*(int *)(iVar6 + 0xe8) + 0xe718) + 0x10),0x40000);
      FUN_00457960(*(undefined4 *)(*(int *)(*(int *)(iVar6 + 0xe8) + 0xe718) + 0x10),0xffffffff);
    }
    iVar3 = iVar3 + 1;
    if (3 < iVar3) {
      iVar6 = FUN_004f16a0(iVar6,iVar2);
      if (iVar6 == 0) {
        FUN_00401220(s_Car_can_t_create_physics_subsyst_00572648);
        return 0;
      }
      return 1;
    }
  } while( true );
}



/* ===== FUN_00504470 @ 00504470  size=776  callers=1 =====
 *   SpringLength
 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00504470(int param_1,int param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int *piVar4;
  undefined4 *puVar5;
  float *pfVar6;
  int iVar7;
  int iVar8;
  float *pfVar9;
  float *pfVar10;
  int iVar11;
  undefined4 *puVar12;
  undefined4 *puVar13;
  undefined1 auStack_36c [716];
  undefined4 uStack_a0;
  int iStack_9c;
  float *pfStack_98;
  char *pcStack_94;
  float *pfStack_90;
  int local_78;
  undefined1 *local_74;
  uint local_70;
  uint local_6c;
  undefined4 uStack_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54 [4];
  float local_44;
  undefined1 local_40 [48];
  float afStack_10 [4];
  
  if (param_2 != 2) {
    iVar11 = 4;
    piVar4 = (int *)(param_1 + 0x48);
    puVar5 = (undefined4 *)(param_1 + 0x60);
    do {
      iVar8 = *piVar4;
      piVar4 = piVar4 + 1;
      iVar11 = iVar11 + -1;
      puVar12 = (undefined4 *)(iVar8 + 0xac);
      puVar13 = puVar5;
      for (iVar7 = 0x10; iVar7 != 0; iVar7 = iVar7 + -1) {
        *puVar13 = *puVar12;
        puVar12 = puVar12 + 1;
        puVar13 = puVar13 + 1;
      }
      puVar5 = puVar5 + 0x10;
    } while (iVar11 != 0);
    puVar5 = (undefined4 *)(*(int *)(param_1 + 0x2ec) + 0xac);
    puVar12 = (undefined4 *)(param_1 + 0x2a8);
    for (iVar11 = 0x10; iVar11 != 0; iVar11 = iVar11 + -1) {
      *puVar12 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar12 = puVar12 + 1;
    }
    puVar5 = (undefined4 *)(*(int *)(param_1 + 0x2e8) + 0xac);
    puVar12 = (undefined4 *)(param_1 + 0x268);
    for (iVar11 = 0x10; iVar11 != 0; iVar11 = iVar11 + -1) {
      *puVar12 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar12 = puVar12 + 1;
    }
    puVar5 = (undefined4 *)(*(int *)(param_1 + 0x1e0) + 0xac);
    puVar12 = (undefined4 *)(param_1 + 0x1e8);
    for (iVar11 = 0x10; iVar11 != 0; iVar11 = iVar11 + -1) {
      *puVar12 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar12 = puVar12 + 1;
    }
    puVar5 = (undefined4 *)(*(int *)(param_1 + 0x1e4) + 0xac);
    puVar12 = (undefined4 *)(param_1 + 0x228);
    for (iVar11 = 0x10; iVar11 != 0; iVar11 = iVar11 + -1) {
      *puVar12 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar12 = puVar12 + 1;
    }
    if (param_2 < 1) {
      iVar11 = 0;
      if (0 < *(int *)(param_1 + 0x2f0)) {
        puVar5 = (undefined4 *)(param_1 + 0x2f8);
        do {
          puVar12 = (undefined4 *)(puVar5[0x10] + 0xac);
          puVar13 = puVar5;
          for (iVar8 = 0x10; iVar8 != 0; iVar8 = iVar8 + -1) {
            *puVar13 = *puVar12;
            puVar12 = puVar12 + 1;
            puVar13 = puVar13 + 1;
          }
          iVar11 = iVar11 + 1;
          puVar5 = puVar5 + 0x16;
        } while (iVar11 < *(int *)(param_1 + 0x2f0));
      }
      local_78 = 0;
      if (0 < *(int *)(param_1 + 0x2f0)) {
        piVar4 = (int *)(param_1 + 0x348);
        local_74 = auStack_36c + -param_1;
        do {
          if (local_78 < 4) {
            *piVar4 = *(int *)(param_1 + 0x2e8);
          }
          else {
            *piVar4 = *(int *)(param_1 + 0x2ec);
          }
          iVar11 = piVar4[-4];
          pfStack_90 = local_54 + 2;
          pfStack_98 = (float *)0x5045b1;
          pcStack_94 = (char *)(iVar11 + 0x6c);
          FUN_0040cbb0();
          pfStack_98 = &local_64;
          uStack_a0 = 0x5045bc;
          iStack_9c = iVar11 + 0x6c;
          FUN_0040cbf0();
          fVar1 = SQRT(local_64 * local_64 + local_60 * local_60 + local_5c * local_5c);
          if ((_DAT_00554380 <= fVar1) && (_DAT_00554380 <= ABS(fVar1 - _DAT_00554390))) {
            fVar1 = _DAT_00554390 / fVar1;
            local_64 = local_64 * fVar1;
            local_60 = local_60 * fVar1;
            local_5c = local_5c * fVar1;
          }
          pfStack_90 = (float *)&local_70;
          pcStack_94 = s_SpringLength_005731a8;
          pfStack_98 = (float *)s_WheelsVisual_00573198;
          iStack_9c = 3;
          uStack_a0 = 0x504647;
          FUN_00404770();
          iVar11 = 0;
          local_6c = local_70;
          uStack_68 = 0;
          fVar1 = _DAT_005544a4;
          if (((_DAT_00554390 <= (float)local_70) &&
              (fVar1 = _DAT_00554424, (float)local_70 <= _DAT_00554474)) &&
             (fVar1 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554918))) {
            fVar1 = ((float)local_70 - _DAT_00554390) * _DAT_00554a10 + _DAT_005544a4;
          }
          piVar4[-0x15] = (int)fVar1;
          pfStack_90 = (float *)local_40;
          pcStack_94 = (char *)(*piVar4 + 0x6c);
          local_58 = fVar1 * local_64 + local_54[2];
          local_54[0] = local_60 * fVar1 + local_54[3];
          local_54[1] = local_5c * fVar1 + local_44;
          pfStack_98 = (float *)0x504701;
          FUN_0040b1f0();
          pfVar6 = (float *)(piVar4 + -3);
          do {
            iVar8 = 2;
            pfVar9 = (float *)(local_74 + (int)pfVar6);
            pfVar10 = local_54;
            *pfVar6 = *(float *)(local_74 + -0x10 + (int)pfVar6) * local_58;
            fVar1 = *pfVar6;
            do {
              fVar2 = *pfVar9;
              fVar3 = *pfVar10;
              pfVar9 = pfVar9 + 4;
              pfVar10 = pfVar10 + 1;
              iVar8 = iVar8 + -1;
              fVar1 = fVar2 * fVar3 + fVar1;
            } while (iVar8 != 0);
            *pfVar6 = fVar1;
            pfVar9 = afStack_10 + iVar11;
            iVar11 = iVar11 + 1;
            *pfVar6 = *pfVar9 + *pfVar6;
            pfVar6 = pfVar6 + 1;
          } while (iVar11 < 3);
          local_78 = local_78 + 1;
          local_74 = local_74 + -0x58;
          piVar4 = piVar4 + 0x16;
        } while (local_78 < *(int *)(param_1 + 0x2f0));
      }
    }
  }
  return 1;
}



/* ===== FUN_0050d0d0 @ 0050d0d0  size=764  callers=1 =====
 *   shiftCenterMassOY
 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_0050d0d0(int param_1)

{
  int iVar1;
  int iVar2;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  iVar1 = param_1;
  **(undefined4 **)(param_1 + 0xe8) = 1;
  iVar2 = *(int *)(param_1 + 0xe8);
  switch(*(undefined4 *)(iVar2 + 0x34)) {
  case 1:
    *(undefined4 *)(iVar2 + 0xf0) = 0x3f000000;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xf8) = 0x3f800000;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xfc) = 0x3f333333;
    break;
  case 2:
  case 8:
    *(undefined4 *)(iVar2 + 0xf0) = 0x3dcccccd;
    goto LAB_0050d153;
  case 3:
    *(undefined4 *)(iVar2 + 0xe8) = 0x3cf5c28f;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xf0) = 0x3f800000;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xf8) = 0x3f800000;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xfc) = 0x3e99999a;
    break;
  case 4:
  case 6:
  case 7:
    *(undefined4 *)(iVar2 + 0xe8) = 0x3ca3d70a;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xf0) = 0x3ecccccd;
LAB_0050d153:
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xf8) = 0x3f800000;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xfc) = 0x3e99999a;
    break;
  case 5:
    *(undefined4 *)(iVar2 + 0xe8) = 0x3dcccccd;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xf0) = 0x3e99999a;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xf8) = 0x3f800000;
    *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0xfc) = 0x3e99999a;
  }
  iVar2 = FUN_00448fa0(param_1,&DAT_00573c28,s_shiftCenterMassOY_00573c54,&param_1);
  if (iVar2 == 1) {
    *(int *)(*(int *)(iVar1 + 0xe8) + 0xe8) = param_1;
  }
  iVar2 = FUN_00448fa0(iVar1,s_Params_005735c8,&DAT_00572c8c,&param_1);
  if (iVar2 == 1) {
    *(int *)(*(int *)(iVar1 + 0xe8) + 0xf0) = param_1;
  }
  iVar2 = FUN_00448fa0(iVar1,s_Params_005735c8,s_coeffHookNorm_00573c44,&param_1);
  if (iVar2 == 1) {
    *(int *)(*(int *)(iVar1 + 0xe8) + 0xfc) = param_1;
  }
  iVar2 = FUN_00448fa0(iVar1,s_Params_005735c8,s_coeffFrict_00573c38,&param_1);
  if (iVar2 == 1) {
    *(int *)(*(int *)(iVar1 + 0xe8) + 0xf8) = param_1;
  }
  FUN_00448d50(iVar1,4);
  FUN_00448d50(iVar1,0x40);
  FUN_00450050(*(undefined4 *)(iVar1 + 0x10),&local_18,iVar1 + 0x138);
  *(float *)(*(int *)(iVar1 + 0xe8) + 0xe0) =
       SQRT((local_c - local_18) * (local_c - local_18) +
            (local_8 - local_14) * (local_8 - local_14) +
            (local_4 - local_10) * (local_4 - local_10));
  *(float *)(*(int *)(iVar1 + 0xe8) + 0xe0) =
       *(float *)(*(int *)(iVar1 + 0xe8) + 0xe0) * _DAT_00554384;
  *(float *)(*(int *)(iVar1 + 0xe8) + 0xe0) =
       *(float *)(*(int *)(iVar1 + 0xe8) + 0xe0) / SQRT((float)_DAT_00554450);
  *(float *)(*(int *)(iVar1 + 0xe8) + 0xe0) =
       *(float *)(*(int *)(iVar1 + 0xe8) + 0xe0) * _DAT_00554768;
  FUN_0050cd10(iVar1,&local_18);
  FUN_005072e0(iVar1,&local_18);
  *(float *)(*(int *)(iVar1 + 0xe8) + 0xf4) =
       _DAT_00554390 / *(float *)(*(int *)(iVar1 + 0xe8) + 0xf0);
  FUN_0040b1f0(*(int *)(iVar1 + 0xe8) + 0x100,*(int *)(iVar1 + 0xe8) + 0x140);
  FUN_00474f70(*(int *)(iVar1 + 0xe8) + 0xf0,iVar1 + 0xf8,*(int *)(iVar1 + 0xe8) + 0xe4);
  FUN_0050cea0(iVar1);
  *(undefined4 *)(*(int *)(iVar1 + 0xe8) + 0x2c) = 1;
  return 1;
}



