/* ===== FUN_00471ca0 @ 00471ca0  size=1365  callers: FUN_005007a0 FUN_005006a0  ===== */

undefined4 __cdecl FUN_00471ca0(float *param_1,float *param_2,float param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *pfVar4;
  float *pfVar5;
  float *pfVar6;
  int iVar7;
  float *pfVar8;
  int iVar9;
  float10 fVar10;
  float local_120 [5];
  float local_10c;
  float local_108;
  float local_104 [6];
  float local_ec;
  float local_e8;
  float local_e4;
  float local_e0;
  float local_dc;
  float local_d8;
  float local_d4;
  float local_d0;
  float local_cc;
  float local_c8;
  float local_c4;
  float local_c0 [16];
  float local_80 [4];
  float local_70 [8];
  float afStack_50 [4];
  float local_40 [12];
  float afStack_10 [4];
  
  if (2e+06 <= param_3) {
    pfVar6 = param_2;
    pfVar5 = param_1 + 0x2b;
    for (iVar7 = 0x10; iVar7 != 0; iVar7 = iVar7 + -1) {
      *pfVar5 = *pfVar6;
      pfVar6 = pfVar6 + 1;
      pfVar5 = pfVar5 + 1;
    }
  }
  FUN_0040b1f0((int)param_2,local_80);
  pfVar6 = param_1 + 0x1c;
  iVar7 = 0;
  do {
    pfVar4 = local_70 + iVar7;
    pfVar5 = param_1 + 0x1d;
    iVar9 = 2;
    local_104[iVar7 + 1] = local_80[iVar7] * *pfVar6;
    do {
      fVar1 = *pfVar4;
      fVar2 = *pfVar5;
      pfVar4 = pfVar4 + 4;
      pfVar5 = pfVar5 + 1;
      iVar9 = iVar9 + -1;
      local_104[iVar7 + 1] = fVar1 * fVar2 + local_104[iVar7 + 1];
    } while (iVar9 != 0);
    pfVar5 = afStack_50 + iVar7;
    iVar9 = iVar7 + 1;
    iVar7 = iVar7 + 1;
    local_104[iVar7] = *pfVar5 + local_104[iVar9];
  } while (iVar7 < 3);
  local_120[4] = local_104[2];
  local_e8 = SQRT(local_104[3] * local_104[3] + local_104[1] * local_104[1]);
  if (local_e8 <= 1e-06) {
    local_120[3] = 0.0;
  }
  else {
    fVar10 = FUN_00410150((float *)&DAT_0055e9e0,local_104 + 1);
    local_120[3] = (float)fVar10;
  }
  FUN_004722c0(local_120[4],param_3,param_1 + 7,&local_dc);
  FUN_004722c0(local_e8,param_3,param_1,&local_e4);
  FUN_004722c0(local_120[3],param_3,param_1 + 0xe,&local_e0);
  local_104[1] = 0.0;
  local_104[3] = -(local_e4 + local_e8);
  local_104[2] = local_dc + local_120[4];
  mat4RotY(local_40,local_e0 + local_120[3]);
  iVar7 = 0;
  do {
    local_120[iVar7] = 0.0;
    pfVar4 = local_104;
    fVar1 = local_120[iVar7];
    pfVar5 = local_40 + iVar7;
    iVar9 = 3;
    do {
      pfVar4 = pfVar4 + 1;
      fVar2 = *pfVar5;
      pfVar5 = pfVar5 + 4;
      iVar9 = iVar9 + -1;
      fVar1 = fVar2 * *pfVar4 + fVar1;
    } while (iVar9 != 0);
    local_120[iVar7] = fVar1;
    pfVar4 = afStack_10 + iVar7;
    pfVar5 = local_120 + iVar7;
    iVar7 = iVar7 + 1;
    *(float *)(&stack0xfffffedc + iVar7 * 4) = *pfVar4 + *pfVar5;
  } while (iVar7 < 3);
  local_104[1] = local_120[0];
  local_104[2] = local_120[1];
  local_104[3] = local_120[2];
  pfVar5 = pfVar6;
  iVar7 = 0;
  do {
    pfVar8 = local_104 + 2;
    pfVar4 = (float *)(((int)param_2 - (int)pfVar6) + 0x10 + (int)pfVar5);
    iVar9 = 2;
    *pfVar5 = *(float *)(((int)param_2 - (int)pfVar6) + (int)pfVar5) * local_120[0];
    do {
      fVar1 = *pfVar8;
      fVar2 = *pfVar4;
      pfVar4 = pfVar4 + 4;
      pfVar8 = pfVar8 + 1;
      iVar9 = iVar9 + -1;
      *pfVar5 = fVar1 * fVar2 + *pfVar5;
    } while (iVar9 != 0);
    iVar9 = iVar7 + 1;
    *pfVar5 = param_2[iVar7 + 0xc] + *pfVar5;
    pfVar5 = pfVar5 + 1;
    iVar7 = iVar9;
  } while (iVar9 < 3);
  mat4GetRow3((int)param_2,&local_cc);
  local_10c = local_cc - *pfVar6;
  local_108 = local_c8 - param_1[0x1d];
  local_104[0] = local_c4 - param_1[0x1e];
  local_120[3] = SQRT(local_10c * local_10c + local_108 * local_108 + local_104[0] * local_104[0]);
  if (local_120[3] < 1e-06) {
    return 0;
  }
  if (1e-06 <= ABS(local_120[3] - 1.0)) {
    fVar1 = 1.0 / local_120[3];
    local_10c = fVar1 * local_10c;
    local_108 = local_108 * fVar1;
    local_104[0] = local_104[0] * fVar1;
  }
  pfVar6 = param_1 + 0x25;
  local_d8 = *pfVar6;
  local_d4 = param_1[0x26];
  local_d0 = param_1[0x27];
  fVar10 = vec3AngleDeg(&local_10c,pfVar6);
  if ((float10)1e-06 < fVar10) {
    fVar10 = FUN_00410150(pfVar6,&local_10c);
    mat4RotY(local_c0,(float)fVar10);
    iVar7 = 0;
    do {
      *(undefined4 *)((int)local_120 + iVar7) = 0;
      fVar1 = *(float *)((int)local_120 + iVar7);
      pfVar5 = (float *)((int)local_c0 + iVar7);
      iVar9 = 3;
      pfVar4 = pfVar6;
      do {
        fVar2 = *pfVar5;
        fVar3 = *pfVar4;
        pfVar5 = pfVar5 + 4;
        pfVar4 = pfVar4 + 1;
        iVar9 = iVar9 + -1;
        fVar1 = fVar2 * fVar3 + fVar1;
      } while (iVar9 != 0);
      *(float *)((int)local_120 + iVar7) = fVar1;
      iVar7 = iVar7 + 4;
    } while (iVar7 < 0xc);
    *pfVar6 = local_120[0];
    param_1[0x26] = local_120[1];
    param_1[0x27] = local_120[2];
    fVar10 = vec3AngleDeg(&local_10c,pfVar6);
    local_120[4] = (float)fVar10;
    if ((float10)1e-06 < fVar10) {
      FUN_004722c0(local_120[4],param_3,param_1 + 0x15,local_120 + 4);
      FUN_0040b4c0(pfVar6,&local_10c,ABS(local_120[4]),local_120);
      *pfVar6 = local_120[0];
      param_1[0x26] = local_120[1];
      param_1[0x27] = local_120[2];
    }
  }
  mat4GetRow1((int)param_2,local_104 + 4);
  local_104[4] = DAT_0055e9a0;
  local_104[5] = DAT_0055e9a4;
  local_ec = DAT_0055e9a8;
  fVar10 = vec3AngleDeg(local_104 + 4,pfVar6);
  if ((float10)0.0001 <= fVar10) {
    pfVar5 = param_1 + 0x1f;
    *pfVar5 = local_ec * param_1[0x26] - local_104[5] * param_1[0x27];
    param_1[0x20] = local_104[4] * param_1[0x27] - local_ec * *pfVar6;
    param_1[0x21] = local_104[5] * *pfVar6 - local_104[4] * param_1[0x26];
    iVar7 = FUN_0040cd00(pfVar5);
    if (iVar7 != 0) {
      FUN_004507a0(pfVar5,pfVar6,param_1 + 0x22);
      FUN_0040cd00(param_1 + 0x22);
    }
  }
  else {
    FUN_00407620(&local_d8,pfVar6,local_c0);
    iVar7 = 0;
    do {
      *(undefined4 *)((int)local_120 + iVar7) = 0;
      fVar1 = *(float *)((int)local_120 + iVar7);
      pfVar6 = (float *)((int)local_c0 + iVar7);
      iVar9 = 3;
      pfVar5 = param_1 + 0x22;
      do {
        fVar2 = *pfVar6;
        fVar3 = *pfVar5;
        pfVar6 = pfVar6 + 4;
        pfVar5 = pfVar5 + 1;
        iVar9 = iVar9 + -1;
        fVar1 = fVar2 * fVar3 + fVar1;
      } while (iVar9 != 0);
      *(float *)((int)local_120 + iVar7) = fVar1;
      iVar7 = iVar7 + 4;
    } while (iVar7 < 0xc);
    param_1[0x22] = local_120[0];
    param_1[0x23] = local_120[1];
    param_1[0x24] = local_120[2];
    iVar7 = 0;
    do {
      *(undefined4 *)((int)local_120 + iVar7) = 0;
      fVar1 = *(float *)((int)local_120 + iVar7);
      pfVar6 = (float *)((int)local_c0 + iVar7);
      iVar9 = 3;
      pfVar5 = param_1 + 0x1f;
      do {
        fVar2 = *pfVar5;
        fVar3 = *pfVar6;
        pfVar6 = pfVar6 + 4;
        pfVar5 = pfVar5 + 1;
        iVar9 = iVar9 + -1;
        fVar1 = fVar2 * fVar3 + fVar1;
      } while (iVar9 != 0);
      *(float *)((int)local_120 + iVar7) = fVar1;
      iVar7 = iVar7 + 4;
    } while (iVar7 < 0xc);
    param_1[0x1f] = local_120[0];
    param_1[0x20] = local_120[1];
    param_1[0x21] = local_120[2];
  }
  pfVar6 = param_1 + 0x2b;
  for (iVar7 = 0x10; iVar7 != 0; iVar7 = iVar7 + -1) {
    *pfVar6 = *param_2;
    param_2 = param_2 + 1;
    pfVar6 = pfVar6 + 1;
  }
  return 1;
}


/* ===== FUN_00471c00 @ 00471c00  size=146  callers: FUN_005007a0 FUN_005006a0  ===== */

void __cdecl
FUN_00471c00(undefined4 *param_1,undefined4 *param_2,undefined4 *param_3,undefined4 *param_4,
            undefined4 *param_5)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  
  puVar2 = &DAT_00564810;
  puVar3 = param_1;
  for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar2 = &DAT_00564810;
  puVar3 = param_1 + 7;
  for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar2 = &DAT_00564810;
  puVar3 = param_1 + 0xe;
  for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar2 = &DAT_00564810;
  puVar3 = param_1 + 0x15;
  for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  if (param_2 != (undefined4 *)0x0) {
    puVar2 = param_1;
    for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = *param_2;
      param_2 = param_2 + 1;
      puVar2 = puVar2 + 1;
    }
  }
  if (param_3 != (undefined4 *)0x0) {
    puVar2 = param_1 + 7;
    for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = *param_3;
      param_3 = param_3 + 1;
      puVar2 = puVar2 + 1;
    }
  }
  if (param_4 != (undefined4 *)0x0) {
    puVar2 = param_1 + 0xe;
    for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = *param_4;
      param_4 = param_4 + 1;
      puVar2 = puVar2 + 1;
    }
  }
  if (param_5 != (undefined4 *)0x0) {
    puVar2 = param_1 + 0x15;
    for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = *param_5;
      param_5 = param_5 + 1;
      puVar2 = puVar2 + 1;
    }
  }
  return;
}


/* ===== FUN_00471b20 @ 00471b20  size=219  callers: FUN_005006a0  ===== */

undefined4 __cdecl FUN_00471b20(undefined4 *param_1)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  
  puVar2 = &DAT_00564810;
  puVar3 = param_1;
  for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar2 = &DAT_00564810;
  puVar3 = param_1 + 7;
  for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar2 = &DAT_00564810;
  puVar3 = param_1 + 0xe;
  for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar2 = &DAT_00564810;
  puVar3 = param_1 + 0x15;
  for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  param_1[0x1c] = DAT_0070c658;
  param_1[0x1d] = DAT_0070c65c;
  param_1[0x1e] = DAT_0070c660;
  param_1[0x1f] = DAT_0055e990;
  param_1[0x20] = DAT_0055e994;
  param_1[0x21] = DAT_0055e998;
  param_1[0x22] = DAT_0055e9a0;
  param_1[0x23] = DAT_0055e9a4;
  param_1[0x24] = DAT_0055e9a8;
  param_1[0x25] = DAT_0055e9b0;
  param_1[0x26] = DAT_0055e9b4;
  param_1[0x27] = DAT_0055e9b8;
  puVar2 = param_1 + 0x2b;
  for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  return 1;
}


/* ===== FUN_00500840 @ 00500840  size=563  callers: FUN_005007a0 FUN_005006a0  ===== */

void __cdecl FUN_00500840(int param_1,undefined4 *param_2)

{
  float fVar1;
  float fVar2;
  float *pfVar3;
  float *pfVar4;
  float *pfVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  float10 fVar9;
  float local_24 [4];
  undefined4 local_14;
  float local_10;
  float local_c;
  float local_8 [2];
  
  iVar6 = *(int *)(param_1 + 0xe8);
  local_24[3] = *(float *)(iVar6 + 0x5884);
  local_14 = *(undefined4 *)(iVar6 + 0x5888);
  local_10 = *(float *)(iVar6 + 0x588c);
  mat4GetRow2(param_1 + 0xf8,local_24);
  fVar9 = vec3AngleDeg(local_24,&DAT_0055e9a0);
  if (fVar9 < (float10)30.0) {
    pfVar3 = local_24;
    iVar8 = (param_1 + 0xf8) - (int)local_24;
    iVar6 = 3;
    local_c = DAT_0055e9b0 * 0.5 - DAT_0055e9a0 * 0.5;
    local_8[0] = DAT_0055e9b4 * 0.5 - DAT_0055e9a4 * 0.5;
    local_8[1] = DAT_0055e9b8 * 0.5 - DAT_0055e9a8 * 0.5;
    do {
      pfVar5 = local_8;
      pfVar4 = (float *)(iVar8 + 0x10 + (int)pfVar3);
      iVar7 = 2;
      *pfVar3 = *(float *)(iVar8 + (int)pfVar3) * local_c;
      do {
        fVar1 = *pfVar5;
        fVar2 = *pfVar4;
        pfVar4 = pfVar4 + 4;
        pfVar5 = pfVar5 + 1;
        iVar7 = iVar7 + -1;
        *pfVar3 = fVar1 * fVar2 + *pfVar3;
      } while (iVar7 != 0);
      pfVar3 = pfVar3 + 1;
      iVar6 = iVar6 + -1;
    } while (iVar6 != 0);
  }
  local_24[1] = 0.0;
  fVar1 = SQRT(local_24[0] * local_24[0] + local_24[2] * local_24[2]);
  if (fVar1 < 1e-06) {
    iVar6 = *(int *)(param_1 + 0xe8);
    local_24[0] = local_24[3] - *(float *)(iVar6 + 0x764);
    local_24[2] = local_10 - *(float *)(iVar6 + 0x76c);
    local_24[1] = 0.0;
    fVar1 = SQRT(local_24[0] * local_24[0] + local_24[2] * local_24[2]);
    if (fVar1 < 1e-06) {
      local_24[0] = *(float *)(iVar6 + 0x788);
      local_24[1] = *(float *)(iVar6 + 0x78c);
      local_24[2] = *(float *)(iVar6 + 0x790);
      goto LAB_00500a4f;
    }
    if (ABS(fVar1 - 1.0) < 1e-06) goto LAB_00500a4f;
    fVar1 = 1.0 / fVar1;
    local_24[0] = fVar1 * local_24[0];
  }
  else {
    if (ABS(fVar1 - 1.0) < 1e-06) goto LAB_00500a4f;
    fVar1 = 1.0 / fVar1;
    local_24[0] = local_24[0] * fVar1;
  }
  local_24[2] = local_24[2] * fVar1;
  local_24[1] = 0.0;
LAB_00500a4f:
  FUN_00408460(local_24 + 3,(float *)0x0,&DAT_0055e9a0,local_24,param_2);
  return;
}


