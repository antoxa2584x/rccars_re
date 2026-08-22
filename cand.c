/* ===== FUN_00500a80 @ 00500a80  size=724 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

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
     ((*(int *)(iVar4 + 0xe828) == 0 && (_DAT_005543f4 <= *(float *)(iVar4 + 0xe830))))) {
    fVar2 = SQRT(*(float *)(iVar4 + 0x58e0) * *(float *)(iVar4 + 0x58e0) +
                 *(float *)(iVar4 + 0x58e4) * *(float *)(iVar4 + 0x58e4) +
                 *(float *)(iVar4 + 0x58e8) * *(float *)(iVar4 + 0x58e8));
    fVar1 = *param_4;
    if (_DAT_005543f0 <= fVar2) {
      param_4 = (float *)(fVar1 * _DAT_00554384);
      if ((fVar2 <= _DAT_0055472c) &&
         (param_4 = _DAT_005543a0, (double)_DAT_00554380 <= ABS(_DAT_00554980))) {
        param_4 = (float *)(fVar1 * _DAT_00554384 * (fVar2 - _DAT_005543f0) * _DAT_00554460);
      }
    }
    else {
      param_4 = (float *)0x0;
    }
    fVar9 = FUN_0049d7e0(*(float *)(iVar4 + 0x6e0),(float)param_4,1.0,param_3);
    *(float *)(*(int *)(param_1 + 0xe8) + 0x6e0) = (float)fVar9;
    iVar4 = *(int *)(param_1 + 0xe8);
    fVar1 = *(float *)(iVar4 + 0x6e0);
    fVar2 = *pfVar3;
    if (((*(byte *)(iVar4 + 0x5754) & 1) == 0) || ((*(byte *)(iVar4 + 0x576c) & 1) == 0)) {
      fVar11 = 0.3;
      fVar10 = 0.0;
    }
    else {
      fVar10 = *pfVar3 * _DAT_005544f0;
      fVar11 = 0.5;
    }
    fVar9 = FUN_0049d7e0(*(float *)(iVar4 + 0x6e4),fVar10,fVar11,param_3);
    *(float *)(*(int *)(param_1 + 0xe8) + 0x6e4) = (float)fVar9;
    *pfVar3 = fVar1 + fVar2 + *(float *)(*(int *)(param_1 + 0xe8) + 0x6e4);
    iVar4 = *(int *)(param_1 + 0xe8);
    fVar1 = _DAT_005547f0;
    if (SQRT(*(float *)(iVar4 + 0x58ec) * *(float *)(iVar4 + 0x58ec) +
             *(float *)(iVar4 + 0x58f0) * *(float *)(iVar4 + 0x58f0) +
             *(float *)(iVar4 + 0x58f4) * *(float *)(iVar4 + 0x58f4)) * _DAT_00554394 <
        _DAT_0055439c) {
      fVar1 = (float)param_6[4] * _DAT_005547f8;
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
    if (*(float *)(iVar4 + 0xe830) < _DAT_005543f4) {
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



