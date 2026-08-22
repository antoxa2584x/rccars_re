/* ===== FUN_004f5400 @ 004f5400  size=326  callers: FUN_004f5b60 FUN_004f5b60 FUN_004f0270 FUN_004f0270  ===== */

void FUN_004f5400(int *param_1,float param_2,undefined4 *param_3,float *param_4,undefined4 *param_5,
                 undefined4 *param_6,undefined4 *param_7)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  float fStack_20;
  undefined4 uStack_1c;
  undefined4 uStack_18;
  undefined4 uStack_14;
  float fStack_10;
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  iVar1 = param_1[0x3a];
  puVar3 = param_3;
  for (iVar2 = 0x20; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  if ((param_6 == (undefined4 *)0x0) || (param_7 == (undefined4 *)0x0)) {
    FUN_004f0910(param_1,param_2,param_4,param_5);
  }
  else {
    iVar2 = param_1[0x3a];
    *(undefined4 *)(iVar2 + 0x58f8) = *param_6;
    *(undefined4 *)(iVar2 + 0x58fc) = param_6[1];
    *(undefined4 *)(iVar2 + 0x5900) = param_6[2];
    iVar2 = param_1[0x3a];
    *(undefined4 *)(iVar2 + 0x5904) = *param_7;
    *(undefined4 *)(iVar2 + 0x5908) = param_7[1];
    *(undefined4 *)(iVar2 + 0x590c) = param_7[2];
  }
  *param_3 = *(undefined4 *)(iVar1 + 0x58e0);
  param_3[1] = *(undefined4 *)(iVar1 + 0x58e4);
  param_3[2] = *(undefined4 *)(iVar1 + 0x58e8);
  fStack_10 = 0.0;
  uStack_c = *(undefined4 *)(iVar1 + 0x58ec);
  uStack_8 = *(undefined4 *)(iVar1 + 0x58f0);
  uStack_4 = *(undefined4 *)(iVar1 + 0x58f4);
  FUN_004079f0(&fStack_10,(float *)(iVar1 + 0x5890),&fStack_20);
  FUN_00407990(&fStack_20,0.5);
  param_3[3] = fStack_20;
  param_3[4] = uStack_1c;
  param_3[5] = uStack_18;
  param_3[6] = uStack_14;
  param_3[7] = *(undefined4 *)(iVar1 + 0x5910);
  param_3[8] = *(undefined4 *)(iVar1 + 0x5914);
  param_3[9] = *(undefined4 *)(iVar1 + 0x5918);
  param_3[10] = *(undefined4 *)(iVar1 + 0x591c);
  param_3[0xb] = *(undefined4 *)(iVar1 + 0x5920);
  param_3[0xc] = *(undefined4 *)(iVar1 + 0x5924);
  return;
}


/* ===== FUN_004f0910 @ 004f0910  size=712  callers: FUN_004f5400  ===== */

void __cdecl FUN_004f0910(int *param_1,float param_2,float *param_3,undefined4 *param_4)

{
  int iVar1;
  float fVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int local_4ec;
  uint local_4e8;
  int local_4e4;
  float local_4e0;
  float local_4dc;
  float local_4d8;
  float local_4d4;
  float local_4d0;
  float local_4cc;
  float local_4bc [3];
  float local_4b0 [150];
  float local_258 [150];
  
  iVar1 = param_1[0x3a];
  local_4e4 = 0;
  local_4e8 = (uint)(*(int *)(iVar1 + 0xa4) != 0);
  local_4ec = 0;
  FUN_004f0be0((int)param_1,param_2,local_4bc,param_3,(int)param_4);
  FUN_004f1000((int)param_1,param_2,&local_4d4,param_3,(int)param_4);
  iVar5 = param_1[0x3a];
  local_258[local_4ec * 3] = *(float *)(iVar5 + 0x5884);
  local_258[local_4ec * 3 + 1] = *(float *)(iVar5 + 0x5888);
  local_258[local_4ec * 3 + 2] = *(float *)(iVar5 + 0x588c);
  fVar2 = *(float *)(iVar5 + 0x57f4) * -10.0;
  local_4b0[local_4ec * 3] = DAT_0055e9a0 * fVar2;
  local_4b0[local_4ec * 3 + 1] = DAT_0055e9a4 * fVar2;
  local_4b0[local_4ec * 3 + 2] = DAT_0055e9a8 * fVar2;
  local_4ec = local_4ec + 1;
  if (local_4e8 == 0) {
    iVar5 = param_1[0x3a];
    local_258[local_4ec * 3] = *(float *)(iVar5 + 0x5884);
    local_258[local_4ec * 3 + 1] = *(float *)(iVar5 + 0x5888);
    local_258[local_4ec * 3 + 2] = *(float *)(iVar5 + 0x588c);
    FUN_004ed970((int)param_1,local_4b0 + local_4ec * 3);
    local_4ec = local_4ec + 1;
  }
  iVar5 = param_1[0x3a];
  iVar4 = local_4ec + 1;
  local_258[local_4ec * 3] = *(float *)(iVar5 + 0x5884);
  local_258[local_4ec * 3 + 1] = *(float *)(iVar5 + 0x5888);
  local_258[local_4ec * 3 + 2] = *(float *)(iVar5 + 0x588c);
  local_4b0[local_4ec * 3] = local_4d4;
  local_4b0[local_4ec * 3 + 1] = local_4d0;
  local_4b0[local_4ec * 3 + 2] = local_4cc;
  local_4ec = iVar4;
  FUN_004749d0(param_1[0x3a] + 0x57f4,local_4bc,local_4b0 + iVar4 * 3,local_258 + iVar4 * 3);
  uVar3 = local_4e8;
  local_4ec = local_4ec + 2;
  if (((local_4e8 == 0) &&
      (iVar5 = FUN_004edac0(param_1,param_2,&local_4ec,local_258,(int)local_4b0,param_3,param_4), iVar5 != 0))
     && (ABS(*(float *)(param_1[0x3a] + 0x5c6c)) < 0.2)) {
    local_4e4 = 1;
  }
  FUN_00475100(iVar1 + 0x57f4,local_258,local_4ec,(int)local_4b0);
  if (uVar3 != 0) {
    *(undefined4 *)(iVar1 + 0x5918) = 0;
    *(undefined4 *)(iVar1 + 0x5910) = 0;
    *(undefined4 *)(iVar1 + 0x5920) = 0;
  }
  if ((local_4e4 != 0) && (*(int *)(param_1[0x3a] + 4) == 0)) {
    FUN_0049d820((float *)(iVar1 + 0x5904),(float *)&DAT_0070c658,2e+06,param_2,&local_4e0);
    *(float *)(iVar1 + 0x591c) = (local_4e0 - *(float *)(iVar1 + 0x5904)) + *(float *)(iVar1 + 0x591c);
    *(float *)(iVar1 + 0x5920) = (local_4dc - *(float *)(iVar1 + 0x5908)) + *(float *)(iVar1 + 0x5920);
    *(float *)(iVar1 + 0x5924) = (local_4d8 - *(float *)(iVar1 + 0x590c)) + *(float *)(iVar1 + 0x5924);
  }
  return;
}


/* ===== FUN_004ed970 @ 004ed970  size=326  callers: FUN_004f0910 FUN_004eea50  ===== */

void __cdecl FUN_004ed970(int param_1,float *param_2)

{
  float fVar1;
  int iVar2;
  float fVar3;
  undefined4 *puVar4;
  float *pfVar5;
  float local_c;
  float local_8;
  float local_4;
  
  if (param_2 != (float *)0x0) {
    *param_2 = DAT_0070c658;
    param_2[1] = DAT_0070c65c;
    param_2[2] = DAT_0070c660;
  }
  iVar2 = *(int *)(param_1 + 0xe8);
  local_c = *(float *)(iVar2 + 0x58e0);
  local_8 = *(float *)(iVar2 + 0x58e4);
  local_4 = *(float *)(iVar2 + 0x58e8);
  fVar3 = SQRT(local_c * local_c + local_8 * local_8 + local_4 * local_4);
  if (0.0001 < fVar3) {
    if ((1e-06 <= fVar3) && (1e-06 <= ABS(fVar3 - 1.0))) {
      fVar1 = 1.0 / fVar3;
      local_c = fVar1 * local_c;
      local_8 = local_8 * fVar1;
      local_4 = local_4 * fVar1;
    }
    puVar4 = FUN_004f7450(*(int *)(iVar2 + 0x54));
    fVar1 = (float)puVar4[1];
    pfVar5 = (float *)FUN_004f7450(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
    if (param_2 != (float *)0x0) {
      fVar3 = -(fVar3 * fVar1 * fVar3 + *pfVar5);
      *param_2 = fVar3 * local_c;
      param_2[1] = local_8 * fVar3;
      param_2[2] = local_4 * fVar3;
    }
  }
  return;
}


/* ===== FUN_004f0be0 @ 004f0be0  size=393  callers: FUN_004f0910  ===== */

void __cdecl FUN_004f0be0(int param_1,float param_2,float *param_3,float *param_4,int param_5)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined4 *puVar5;
  float *pfVar6;
  int local_4dc;
  float local_4d8;
  float local_4d4;
  float local_4d0;
  float local_4cc;
  float local_4c8;
  float local_4c4;
  float local_4c0;
  undefined4 local_4bc [3];
  undefined1 local_4b0 [600];
  float local_258 [150];
  
  if (param_3 != (float *)0x0) {
    *param_3 = DAT_0070c658;
    local_4dc = 0;
    param_3[1] = DAT_0070c65c;
    param_3[2] = DAT_0070c660;
    iVar1 = *(int *)(param_1 + 0xe8);
    FUN_004f0d70(param_1,param_2,&local_4dc,(int)local_258,(int)local_4b0,0,param_4,param_5);
    FUN_00475100(iVar1 + 0x57f4,local_258,local_4dc,(int)local_4b0);
    *param_3 = *(float *)(iVar1 + 0x591c);
    param_3[1] = *(float *)(iVar1 + 0x5920);
    puVar5 = (undefined4 *)(param_1 + 0xf8);
    param_3[2] = *(float *)(iVar1 + 0x5924);
    FUN_00406640(puVar5,&local_4cc);
    FUN_0040cbd0((int)puVar5,local_4bc);
    FUN_0040cbf0((int)puVar5,&local_4d8);
    local_4c0 = local_4cc * *param_3 + local_4c8 * param_3[1] + local_4c4 * param_3[2];
    fVar2 = local_4d0 * param_3[2];
    fVar4 = local_4d4 * param_3[1];
    fVar3 = local_4d8 * *param_3;
    puVar5 = FUN_004f74a0(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
    fVar2 = (fVar3 + fVar4 + fVar2) * (float)puVar5[1];
    pfVar6 = (float *)FUN_004f74a0(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
    local_4c0 = local_4c0 * *pfVar6;
    *param_3 = local_4d8 * fVar2 + local_4c0 * local_4cc;
    param_3[1] = local_4d4 * fVar2 + local_4c8 * local_4c0;
    param_3[2] = local_4c4 * local_4c0 + local_4d0 * fVar2;
  }
  return;
}


/* ===== FUN_004f1000 @ 004f1000  size=205  callers: FUN_004f0910 FUN_004ee5e0  ===== */

void __cdecl FUN_004f1000(int param_1,float param_2,undefined4 *param_3,float *param_4,int param_5)

{
  int iVar1;
  int local_4c0;
  undefined4 local_4bc [3];
  undefined1 local_4b0 [600];
  float local_258 [150];
  
  if (param_3 != (undefined4 *)0x0) {
    *param_3 = DAT_0070c658;
    local_4c0 = 0;
    param_3[1] = DAT_0070c65c;
    param_3[2] = DAT_0070c660;
    iVar1 = *(int *)(param_1 + 0xe8);
    FUN_0040cbd0(param_1 + 0xf8,local_4bc);
    FUN_004f0d70(param_1,param_2,&local_4c0,(int)local_258,(int)local_4b0,1,param_4,param_5);
    FUN_00475100(iVar1 + 0x57f4,local_258,local_4c0,(int)local_4b0);
    *param_3 = *(undefined4 *)(iVar1 + 0x5910);
    param_3[1] = *(undefined4 *)(iVar1 + 0x5914);
    param_3[2] = *(undefined4 *)(iVar1 + 0x5918);
  }
  return;
}


/* ===== FUN_004f0d70 @ 004f0d70  size=515  callers: FUN_004f0be0 FUN_004f1000  ===== */

void __cdecl
FUN_004f0d70(int param_1,float param_2,int *param_3,int param_4,int param_5,int param_6,float *param_7,
            int param_8)

{
  float *pfVar1;
  float fVar2;
  float *pfVar3;
  float *pfVar4;
  float *pfVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  float10 fVar9;
  int local_20;
  int local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  if ((*(int *)(*(int *)(param_1 + 0xe8) + 0x48) != 0) && (0 < (int)param_7)) {
    FUN_0040cbd0(param_1 + 0xf8,&local_18);
    iVar6 = *(int *)(param_1 + 0xe8);
    local_20 = 0;
    if (0 < *(int *)(iVar6 + 0x5928)) {
      param_7 = (float *)(param_8 + 0x78);
      local_1c = 0;
      do {
        if ((param_7[-0xb] != 0.0) &&
           (iVar7 = iVar6 + 0x5948 + local_1c,
           *(float *)(iVar6 + 0x5974 + local_1c) + *(float *)(iVar6 + 0x595c + local_1c) <=
           *(float *)(iVar7 + 0x18))) {
          fVar9 = FUN_004f0f80(iVar7,param_2);
          iVar6 = 0;
          pfVar3 = (float *)(param_4 + *param_3 * 0xc);
          iVar7 = (param_1 + 0xf8) - (int)pfVar3;
          pfVar1 = (float *)(*(int *)(param_1 + 0xe8) + 0x5948 + local_1c);
          do {
            pfVar4 = (float *)(iVar7 + 0x10 + (int)pfVar3);
            iVar8 = 2;
            *pfVar3 = *(float *)(iVar7 + (int)pfVar3) * *pfVar1;
            pfVar5 = pfVar1;
            do {
              pfVar5 = pfVar5 + 1;
              fVar2 = *pfVar4;
              pfVar4 = pfVar4 + 4;
              iVar8 = iVar8 + -1;
              *pfVar3 = *pfVar5 * fVar2 + *pfVar3;
            } while (iVar8 != 0);
            iVar6 = iVar6 + 1;
            *pfVar3 = *(float *)(param_1 + 0x124 + iVar6 * 4) + *pfVar3;
            pfVar3 = pfVar3 + 1;
          } while (iVar6 < 3);
          if (param_6 == 0) {
            pfVar3 = (float *)(param_5 + *param_3 * 0xc);
            *pfVar3 = (float)((float10)local_18 * fVar9);
            pfVar3[1] = (float)((float10)local_14 * fVar9);
            pfVar3[2] = (float)((float10)local_10 * fVar9);
          }
          else {
            pfVar3 = (float *)(param_5 + *param_3 * 0xc);
            *pfVar3 = (float)((float10)local_18 * fVar9);
            pfVar3[1] = (float)((float10)local_14 * fVar9);
            pfVar3[2] = (float)((float10)local_10 * fVar9);
            pfVar3 = (float *)(param_5 + *param_3 * 0xc);
            local_4 = param_7[-2] * *pfVar3 +
                      pfVar3[2] * *param_7 + *(float *)(param_5 + 4 + *param_3 * 0xc) * param_7[-1];
            local_c = local_4 * param_7[-2];
            local_8 = local_4 * param_7[-1];
            local_4 = local_4 * *param_7;
            *pfVar3 = local_c;
            pfVar3[1] = local_8;
            pfVar3[2] = local_4;
          }
          *param_3 = *param_3 + 1;
        }
        iVar6 = *(int *)(param_1 + 0xe8);
        local_20 = local_20 + 1;
        local_1c = local_1c + 0x68;
        param_7 = param_7 + 0x26;
      } while (local_20 < *(int *)(iVar6 + 0x5928));
    }
  }
  return;
}


/* ===== FUN_004f0f80 @ 004f0f80  size=122  callers: FUN_004f0d70  ===== */

float10 __cdecl FUN_004f0f80(int param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float10 fVar3;
  
  if (param_2 <= 0.001) {
    fVar3 = (float10)0.0;
  }
  else {
    fVar3 = ((float10)*(float *)(param_1 + 0x28) * (float10)*(float *)(param_1 + 0x10)) / (float10)param_2;
  }
  fVar1 = -(((*(float *)(param_1 + 0x2c) + *(float *)(param_1 + 0x14)) - *(float *)(param_1 + 0x18)) *
           *(float *)(param_1 + 0xc));
  fVar2 = ABS(fVar1) * 2e+06;
  if (ABS(fVar3) < (float10)fVar2) {
    return -fVar3 + (float10)fVar1;
  }
  if ((float10)0.0 < fVar3) {
    return (float10)fVar1 - (float10)fVar2;
  }
  return (float10)fVar2 + (float10)fVar1;
}


/* ===== FUN_004edac0 @ 004edac0  size=1648  callers: FUN_004f0910  ===== */

undefined4 __cdecl
FUN_004edac0(int *param_1,float param_2,int *param_3,float *param_4,int param_5,float *param_6,
            undefined4 *param_7)

{
  undefined4 *puVar1;
  float fVar2;
  bool bVar3;
  float *pfVar4;
  float *pfVar5;
  int *piVar6;
  float *pfVar7;
  int iVar8;
  float **ppfVar9;
  float *pfVar10;
  undefined4 *puVar11;
  float *pfVar12;
  float10 fVar13;
  int iVar14;
  float *local_1a8;
  float *local_1a4;
  float local_1a0;
  float local_19c;
  float *local_198;
  int local_194;
  float *local_190;
  float *local_18c;
  float local_188 [2];
  float *local_180;
  int local_17c;
  float local_178;
  float local_174;
  float local_170;
  float local_16c;
  float local_168;
  float local_164;
  float local_160;
  float local_15c;
  float local_158;
  float local_154;
  float local_150;
  float local_14c;
  float local_144;
  float local_140;
  float local_12c [9];
  int local_108 [14];
  float local_d0 [52];
  
  piVar6 = param_7 + 0x13;
  iVar8 = 4;
  iVar14 = param_1[0x3a];
  pfVar4 = (float *)0x0;
  do {
    if (*piVar6 != 0) {
      pfVar4 = (float *)((int)pfVar4 + 1);
    }
    piVar6 = piVar6 + 0x26;
    iVar8 = iVar8 + -1;
  } while (iVar8 != 0);
  if ((int)param_6 < (int)pfVar4) {
    pfVar4 = param_6;
  }
  local_17c = iVar14 + 0x57f4;
  FUN_004ee280(param_1,param_2,(int *)&local_198,(float)local_108,param_3,(int)param_4,param_5,pfVar4,param_7)
  ;
  if (((-1 < (int)local_198) && ((int)local_198 < 3)) && (local_198 != (float *)0x0)) {
    FUN_00474e00(iVar14 + 0x57f4);
    local_194 = 0;
    if (0 < (int)local_198) {
      local_190 = &local_16c;
      local_1a8 = local_188;
      pfVar4 = local_d0;
      do {
        pfVar5 = pfVar4 + -2;
        FUN_00476c50(local_17c,pfVar4 + -0xb,pfVar5,*param_3,param_4,param_5,&local_178);
        local_18c = (float *)0x0;
        *local_1a8 = -(local_178 * *pfVar5 + local_174 * pfVar4[-1] + local_170 * *pfVar4);
        if (0 < (int)local_198) {
          pfVar7 = local_d0;
          pfVar10 = local_190;
          do {
            *pfVar10 = (pfVar7[-2] * *pfVar5 + pfVar4[-1] * pfVar7[-1] + *pfVar4 * *pfVar7) *
                       *(float *)(local_17c + 4);
            local_1a4 = (float *)(pfVar7[-7] * *pfVar7 - pfVar7[-6] * pfVar7[-1]);
            local_1a0 = pfVar7[-6] * pfVar7[-2] - pfVar7[-8] * *pfVar7;
            local_19c = pfVar7[-1] * pfVar7[-8] - pfVar7[-7] * pfVar7[-2];
            FUN_00474910((float *)(local_17c + 0xac),(float *)&local_1a4,&local_15c);
            local_18c = (float *)((int)local_18c + 1);
            local_1a4 = (float *)(local_158 * pfVar4[-6] - local_154 * pfVar4[-7]);
            pfVar7 = pfVar7 + 0x21;
            local_1a0 = local_154 * pfVar4[-8] - local_15c * pfVar4[-6];
            local_19c = local_15c * pfVar4[-7] - local_158 * pfVar4[-8];
            *pfVar10 = (float)local_1a4 * *pfVar5 + local_1a0 * pfVar4[-1] + local_19c * *pfVar4 + *pfVar10;
            pfVar10 = pfVar10 + 1;
          } while ((int)local_18c < (int)local_198);
        }
        local_194 = local_194 + 1;
        local_1a8 = local_1a8 + 1;
        pfVar4 = pfVar4 + 0x21;
        local_190 = local_190 + 2;
      } while (local_194 < (int)local_198);
    }
    local_194 = 0;
    if (0 < (int)local_198) {
      pfVar4 = local_188;
      piVar6 = local_108;
      do {
        if ((*piVar6 == 2) || (*piVar6 == 3)) {
          bVar3 = true;
          iVar14 = 1;
        }
        else {
          bVar3 = false;
          iVar14 = 0;
        }
        fVar13 = FUN_004ee180((int)param_1,iVar14);
        fVar2 = (float)fVar13;
        iVar8 = FUN_004ee130((int)param_1);
        iVar14 = local_17c;
        if (iVar8 == 0) {
          if (bVar3) {
            local_1a4 = (float *)(fVar2 * 10.0);
            local_180 = (float *)*pfVar4;
            pfVar5 = (float *)(fVar2 * -10.0);
            if ((fVar2 * -10.0 <= (float)local_180) &&
               (pfVar5 = local_180, (float)local_1a4 < (float)local_180)) {
              pfVar5 = local_1a4;
            }
          }
          else {
            local_1a4 = (float *)(fVar2 * 5.0);
            local_18c = (float *)*pfVar4;
            pfVar5 = (float *)(fVar2 * -5.0);
            if ((fVar2 * -5.0 <= (float)local_18c) &&
               (pfVar5 = local_18c, (float)local_1a4 < (float)local_18c)) {
              pfVar5 = local_1a4;
            }
          }
        }
        else {
          local_1a4 = (float *)(fVar2 * 10.0);
          local_190 = (float *)*pfVar4;
          pfVar5 = (float *)(fVar2 * -10.0);
          if ((fVar2 * -10.0 <= (float)local_190) && (pfVar5 = local_190, (float)local_1a4 < (float)local_190)
             ) {
            pfVar5 = local_1a4;
          }
        }
        *pfVar4 = (float)pfVar5;
        FUN_00474ed0(iVar14,(float *)(piVar6 + 3),&local_178);
        local_1a4 = (float *)(local_174 * (float)piVar6[0xd] +
                             local_170 * (float)piVar6[0xe] + local_178 * (float)piVar6[0xc]);
        if (0.0005 < ABS((float)local_1a4)) {
          local_1a4 = (float *)((float)local_1a4 / param_2);
          pfVar5 = (float *)(fVar2 * -10.0);
          if ((fVar2 * -10.0 <= (float)local_1a4) && (pfVar5 = local_1a4, fVar2 * 10.0 < (float)local_1a4)) {
            pfVar5 = (float *)(fVar2 * 10.0);
          }
          *pfVar4 = *pfVar4 - (float)pfVar5;
        }
        local_194 = local_194 + 1;
        piVar6 = piVar6 + 0x21;
        pfVar4 = pfVar4 + 1;
      } while (local_194 < (int)local_198);
    }
    if (local_198 == (float *)0x1) {
      if (ABS(local_16c) <= 1e-06) {
        local_1a4 = (float *)0x0;
      }
      else {
        local_1a4 = (float *)(local_188[0] / local_16c);
      }
    }
    else {
      pfVar4 = (float *)&DAT_0055e918;
      pfVar5 = local_12c;
      for (iVar14 = 9; iVar14 != 0; iVar14 = iVar14 + -1) {
        *pfVar5 = *pfVar4;
        pfVar4 = pfVar4 + 1;
        pfVar5 = pfVar5 + 1;
      }
      if (0 < (int)local_198) {
        pfVar4 = &local_16c;
        local_1a4 = local_198;
        pfVar5 = local_12c;
        do {
          pfVar10 = pfVar4;
          pfVar12 = pfVar5;
          for (pfVar7 = local_198; pfVar7 != (float *)0x0; pfVar7 = (float *)((int)pfVar7 + -1)) {
            *pfVar12 = *pfVar10;
            pfVar10 = pfVar10 + 1;
            pfVar12 = pfVar12 + 1;
          }
          pfVar4 = pfVar4 + 2;
          local_1a4 = (float *)((int)local_1a4 + -1);
          pfVar5 = pfVar5 + 3;
        } while (local_1a4 != (float *)0x0);
      }
      iVar14 = FUN_00408c20(local_12c,&local_150);
      if (iVar14 == 0) {
        if (ABS(local_16c) <= ABS(local_168)) {
          local_1a4 = (float *)0x0;
          local_1a0 = local_188[0] / local_168;
          local_164 = local_160 * (local_188[0] / local_168);
        }
        else {
          local_1a0 = 0.0;
          local_1a4 = (float *)(local_188[0] / local_16c);
          local_164 = local_164 * (local_188[0] / local_16c);
        }
        if (0.0001 < ABS(local_188[0] - local_164)) {
          local_1a0 = 0.0;
          local_1a4 = (float *)0x0;
        }
      }
      else {
        local_1a4 = (float *)(local_150 * local_188[0] + local_188[1] * local_14c);
        local_1a0 = local_144 * local_188[0] + local_140 * local_188[1];
      }
    }
    local_194 = 0;
    if (0 < (int)local_198) {
      ppfVar9 = &local_1a4;
      pfVar4 = local_d0;
      puVar11 = &DAT_014b0b98;
      do {
        pfVar7 = *ppfVar9;
        pfVar5 = param_4 + *param_3 * 3;
        *pfVar5 = pfVar4[-0xb];
        ppfVar9 = ppfVar9 + 1;
        pfVar5[1] = pfVar4[-10];
        pfVar5[2] = pfVar4[-9];
        pfVar5 = (float *)(param_5 + *param_3 * 0xc);
        *pfVar5 = (float)pfVar7 * pfVar4[-2];
        pfVar5[1] = (float)pfVar7 * pfVar4[-1];
        pfVar5[2] = (float)pfVar7 * *pfVar4;
        iVar14 = *param_3;
        *param_3 = iVar14 + 1;
        puVar1 = (undefined4 *)(param_5 + -0xc + (iVar14 + 1) * 0xc);
        *puVar11 = *puVar1;
        puVar11[1] = puVar1[1];
        puVar11[2] = puVar1[2];
        local_194 = local_194 + 1;
        pfVar4 = pfVar4 + 0x21;
        puVar11 = puVar11 + 3;
      } while (local_194 < (int)local_198);
    }
    return 1;
  }
  return 0;
}


/* ===== FUN_004ee280 @ 004ee280  size=860  callers: FUN_004edac0  ===== */

void __cdecl
FUN_004ee280(int *param_1,float param_2,int *param_3,float param_4,int *param_5,int param_6,int param_7,
            float *param_8,undefined4 *param_9)

{
  int iVar1;
  float fVar2;
  int iVar3;
  float *pfVar4;
  int iVar5;
  bool bVar6;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  
  fVar2 = param_4;
  *param_3 = 0;
  iVar1 = param_1[0x3a];
  if (0 < (int)param_8) {
    iVar3 = FUN_004ee5e0(param_1,param_2,param_8,(float)param_9,param_5,param_6,param_7);
    if ((int)param_8 < 3) {
      iVar5 = 0;
      do {
        if (param_9[0x13] != 0) {
          if (*param_3 == 2) break;
          if ((iVar5 == 0) || (iVar5 == 1)) {
            param_4 = *(float *)(param_1[0x3a] + 0x5c6c);
          }
          else {
            param_4 = 0.0;
          }
          FUN_004ed6a0((int)param_1,param_9,param_4,(int)fVar2 + *param_3 * 0x84);
          *(int *)((int)fVar2 + *param_3 * 0x84) = iVar5;
          *param_3 = *param_3 + 1;
        }
        iVar5 = iVar5 + 1;
        param_9 = param_9 + 0x26;
      } while (iVar5 < 4);
    }
    else {
      if (0.01 <= *(float *)(param_1[0x3a] + 0x5c6c)) {
        iVar5 = (param_9[0x85] != 0) + 2;
      }
      else {
        iVar5 = 3 - (uint)(param_9[0x5f] != 0);
      }
      FUN_004ed6a0((int)param_1,param_9 + iVar5 * 0x26,0.0,(int)param_4 + *param_3 * 0x84);
      *(int *)((int)param_4 + *param_3 * 0x84) = iVar5;
      iVar5 = *param_3;
      *param_3 = iVar5 + 1;
      if (0.01 <= *(float *)(param_1[0x3a] + 0x5c6c)) {
        bVar6 = param_9[0x13] == 0;
      }
      else {
        bVar6 = param_9[0x39] != 0;
      }
      FUN_004ed6a0((int)param_1,param_9 + (uint)bVar6 * 0x26,*(float *)(param_1[0x3a] + 0x5c6c),
                   (int)param_4 + (iVar5 + 1) * 0x84);
      *(uint *)((int)param_4 + *param_3 * 0x84) = (uint)bVar6;
      *param_3 = *param_3 + 1;
    }
    if ((iVar3 != 0) && (iVar3 = 0, 0 < *param_3)) {
      pfVar4 = (float *)((int)fVar2 + 0x2c);
      do {
        FUN_00474ed0(iVar1 + 0x57f4,pfVar4 + -8,&local_24);
        fVar2 = SQRT(local_24 * local_24 + local_20 * local_20 + local_1c * local_1c);
        if (1e-06 <= fVar2) {
          if (1e-06 <= ABS(fVar2 - 1.0)) {
            fVar2 = 1.0 / fVar2;
            local_24 = local_24 * fVar2;
            local_20 = local_20 * fVar2;
            local_1c = local_1c * fVar2;
          }
          fVar2 = local_20 * pfVar4[-1] + local_24 * pfVar4[-2] + local_1c * *pfVar4;
          local_c = fVar2 * pfVar4[-2];
          local_8 = fVar2 * pfVar4[-1];
          local_18 = local_24 - local_c;
          local_14 = local_20 - local_8;
          local_10 = local_1c - fVar2 * *pfVar4;
          fVar2 = SQRT(local_18 * local_18 + local_14 * local_14 + local_10 * local_10);
          if (1e-06 <= fVar2) {
            if (1e-06 <= ABS(fVar2 - 1.0)) {
              fVar2 = 1.0 / fVar2;
              local_18 = local_18 * fVar2;
              local_14 = local_14 * fVar2;
              local_10 = local_10 * fVar2;
            }
            pfVar4[1] = local_18;
            pfVar4[2] = local_14;
            pfVar4[3] = local_10;
          }
        }
        iVar3 = iVar3 + 1;
        pfVar4 = pfVar4 + 0x21;
      } while (iVar3 < *param_3);
    }
  }
  return;
}


/* ===== FUN_004ee5e0 @ 004ee5e0  size=1122  callers: FUN_004ee280  ===== */

int __cdecl
FUN_004ee5e0(int *param_1,float param_2,float *param_3,float param_4,int *param_5,int param_6,int param_7)

{
  undefined4 *puVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  float *pfVar8;
  float10 fVar9;
  float10 fVar10;
  float local_b8;
  float local_b4;
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  int local_98;
  float *local_94;
  float local_90 [2];
  float local_88;
  undefined1 local_84 [12];
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  float local_48;
  float local_44;
  float local_40;
  
  iVar6 = 0;
  iVar5 = param_1[0x3a];
  if (0 < *(int *)(iVar5 + 0x5928)) {
    iVar7 = 0;
    iVar4 = iVar5;
    do {
      puVar1 = (undefined4 *)(iVar7 + 0x5988 + iVar4);
      iVar6 = iVar6 + 1;
      iVar7 = iVar7 + 0x68;
      *puVar1 = DAT_0070c658;
      puVar1[1] = DAT_0070c65c;
      puVar1[2] = DAT_0070c660;
      iVar4 = param_1[0x3a];
    } while (iVar6 < *(int *)(iVar4 + 0x5928));
  }
  FUN_004ee180((int)param_1,0);
  fVar9 = FUN_004ee180((int)param_1,1);
  iVar6 = param_1[0x3a];
  if ((*(byte *)(iVar6 + 0x5784) & 1) == 0) {
    *(undefined4 *)(iVar6 + 0x57ec) = 0;
    iVar6 = param_1[0x3a];
    if (*(int *)(iVar6 + 0x57f0) < 0) {
      if ((*(byte *)(iVar6 + 0x576c) & 1) != 0) {
        if (SQRT(*(float *)(iVar5 + 0x58e0) * *(float *)(iVar5 + 0x58e0) +
                 *(float *)(iVar5 + 0x58e4) * *(float *)(iVar5 + 0x58e4) +
                 *(float *)(iVar5 + 0x58e8) * *(float *)(iVar5 + 0x58e8)) <= 2.0) {
          *(undefined4 *)(iVar6 + 0x57f0) = 0;
        }
        else {
          *(undefined4 *)(iVar6 + 0x57ec) = 1;
        }
      }
    }
    else if ((*(byte *)(iVar6 + 0x5774) & 1) != 0) {
      if (SQRT(*(float *)(iVar5 + 0x58e0) * *(float *)(iVar5 + 0x58e0) +
               *(float *)(iVar5 + 0x58e4) * *(float *)(iVar5 + 0x58e4) +
               *(float *)(iVar5 + 0x58e8) * *(float *)(iVar5 + 0x58e8)) <= 2.0) {
        *(undefined4 *)(iVar6 + 0x57f0) = 0xffffffff;
      }
      else {
        *(undefined4 *)(iVar6 + 0x57ec) = 1;
      }
    }
  }
  else {
    *(undefined4 *)(iVar6 + 0x57ec) = 1;
  }
  fVar10 = FUN_004eea50((float)param_1,&local_b8);
  local_a8 = (float)fVar9 * 20.0;
  fVar2 = (float)fVar9 * -20.0;
  if ((float10)fVar2 <= fVar10) {
    if ((float10)local_a8 < fVar10) {
      fVar10 = (float10)local_a8;
    }
  }
  else {
    fVar10 = (float10)fVar2;
  }
  fVar2 = (float)(fVar10 * (float10)*(float *)(iVar5 + 0x57f4));
  FUN_004eeea0(param_1,param_2,param_3,param_4,param_5,param_6,param_7);
  iVar5 = *(int *)(param_1[0x3a] + 0x57ec);
  if (iVar5 == 0) {
    if (ABS(fVar2) < 0.001) {
      return 0;
    }
    fVar2 = fVar2 * 0.5;
    pfVar8 = (float *)((int)param_4 + 0x1a0);
    local_98 = 0xd0;
    local_b8 = local_b8 * 0.5;
    do {
      iVar5 = local_98;
      if ((pfVar8[-9] != 0.0) &&
         (local_94 = pfVar8, fVar9 = FUN_00410130((float *)&DAT_0055e9a0,pfVar8), fVar9 <= (float10)46.0)) {
        FUN_004f1000((int)param_1,param_2,local_90,param_3,(int)param_4);
        FUN_004ed6a0((int)param_1,pfVar8 + -0x1c,0.0,(int)local_84);
        puVar1 = (undefined4 *)(param_6 + *param_5 * 0xc);
        *puVar1 = local_78;
        puVar1[1] = local_74;
        puVar1[2] = local_70;
        local_b4 = local_48 * fVar2;
        local_a4 = local_48;
        local_b0 = local_44 * fVar2;
        local_a0 = local_44;
        local_9c = local_40;
        local_ac = local_40 * fVar2;
        local_a8 = local_88 * local_40 + local_90[0] * local_48;
        fVar9 = FUN_00410130((float *)&DAT_0055e9a0,&local_48);
        if ((fVar9 < (float10)80.0) && (local_a8 < 0.0)) {
          if ((local_b8 <= 0.0) || (fVar3 = ABS(local_a8), local_b8 <= fVar3)) {
            local_b4 = local_a4 * local_b8 + local_b4;
            local_b0 = local_a0 * local_b8 + local_b0;
            local_ac = local_9c * local_b8 + local_ac;
          }
          else {
            local_b4 = local_a4 * fVar3 + local_b4;
            local_b0 = local_a0 * fVar3 + local_b0;
            local_ac = local_9c * fVar3 + local_ac;
          }
        }
        pfVar8 = (float *)(param_1[0x3a] + 0x5988 + iVar5);
        *pfVar8 = local_b4;
        pfVar8[1] = local_b0;
        pfVar8[2] = local_ac;
        pfVar8 = (float *)(param_7 + *param_5 * 0xc);
        *pfVar8 = local_b4;
        pfVar8[1] = local_b0;
        pfVar8[2] = local_ac;
        *param_5 = *param_5 + 1;
        pfVar8 = local_94;
        iVar5 = local_98;
      }
      local_98 = iVar5 + 0x68;
      pfVar8 = pfVar8 + 0x26;
    } while (local_98 < 0x1a0);
    iVar5 = *(int *)(param_1[0x3a] + 0x57ec);
  }
  return iVar5;
}


/* ===== FUN_004eea50 @ 004eea50  size=1091  callers: FUN_004ee5e0  ===== */

/* WARNING: Removing unreachable block (ram,0x004eee63) */

float10 __cdecl FUN_004eea50(float param_1,float *param_2)

{
  float fVar1;
  int iVar2;
  int iVar3;
  float *pfVar4;
  int iVar5;
  float fVar6;
  float fVar7;
  bool bVar8;
  undefined4 *puVar9;
  int iVar10;
  bool bVar11;
  float10 fVar12;
  float fVar13;
  undefined4 *puVar14;
  float local_18;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  fVar7 = param_1;
  bVar11 = false;
  iVar2 = *(int *)((int)param_1 + 0xe8);
  bVar8 = false;
  if (param_2 != (float *)0x0) {
    *param_2 = 0.0;
  }
  if (DAT_014c4910 == 0) {
    iVar3 = *(int *)((int)param_1 + 0xe8);
    bVar11 = (*(byte *)(iVar3 + 0x576c) & 1) != 0;
    if (bVar11) {
      param_1 = *(float *)(iVar3 + 0x5770);
    }
    if ((*(byte *)(iVar3 + 0x5774) & 1) != 0) {
      local_10 = *(float *)(iVar3 + 0x5778);
      bVar8 = true;
    }
  }
  else if (DAT_014c45d4 <= 1e-06) {
    if (DAT_014c45d4 < -1e-06) {
      bVar8 = true;
      local_10 = DAT_014c45d4;
    }
  }
  else {
    bVar11 = true;
    param_1 = DAT_014c45d4;
  }
  FUN_0040cbf0((int)fVar7 + 0xf8,&local_c);
  iVar3 = *(int *)((int)fVar7 + 0xe8);
  fVar6 = local_c * *(float *)(iVar2 + 0x58e0) +
          local_8 * *(float *)(iVar2 + 0x58e4) + local_4 * *(float *)(iVar2 + 0x58e8);
  if (*(int *)(iVar3 + 0x57f0) < 0) {
    if (!bVar8) {
      return (float10)0.0;
    }
    fVar12 = (float10)4.0;
    if (*(int *)(iVar3 + 0x573c) != 0) {
      fVar12 = (float10)6.0;
    }
    fVar7 = SQRT(*(float *)(iVar2 + 0x58e0) * *(float *)(iVar2 + 0x58e0) +
                 *(float *)(iVar2 + 0x58e4) * *(float *)(iVar2 + 0x58e4) +
                 *(float *)(iVar2 + 0x58e8) * *(float *)(iVar2 + 0x58e8));
    fVar12 = -(ABS((float10)local_10) * fVar12);
    if (fVar7 < 2.8333335) {
      return fVar12;
    }
    if (fVar7 <= 3.3333335) {
      return -fVar12 * ((float10)fVar7 - (float10)2.8333335) * (float10)2.0000002 + fVar12;
    }
    return (float10)0.0;
  }
  if (bVar11) {
    if (DAT_014b0bb0 == 0) goto LAB_004eeb76;
  }
  else if (DAT_014b0bb0 == 0) {
    return (float10)0.0;
  }
  param_1 = 1.0;
LAB_004eeb76:
  puVar9 = FUN_004f73d0(*(int *)(iVar3 + 0x54));
  pfVar4 = *(float **)puVar9[6];
  if (pfVar4 == (float *)0x0) {
    return (float10)0.0;
  }
  iVar2 = *(int *)((int)fVar7 + 0xe8);
  if (*(int *)(iVar2 + 0x573c) == 0) {
    puVar9 = FUN_004f73d0(*(int *)(iVar2 + 0x54));
    fVar13 = (float)puVar9[2];
    local_18 = 1.0;
  }
  else {
    puVar9 = FUN_004f73d0(*(int *)(iVar2 + 0x54));
    fVar13 = (float)puVar9[3];
    puVar9 = FUN_004f73d0(*(int *)(*(int *)((int)fVar7 + 0xe8) + 0x54));
    local_18 = (float)puVar9[4];
  }
  local_10 = fVar13 * 0.2777778;
  iVar2 = *(int *)((int)fVar7 + 0xe8);
  iVar3 = *(int *)(iVar2 + 0x54);
  puVar9 = FUN_004f73d0(*(int *)(iVar2 + 0x54));
  iVar5 = *(int *)((int)fVar7 + 0xe8);
  iVar10 = *(int *)(iVar5 + 0x54);
  fVar13 = ABS(param_1) * local_18 * (float)puVar9[*(int *)(iVar3 * 0xb0 + 0xe454 + iVar2) + 0xb];
  puVar9 = FUN_004f73d0(iVar10);
  if (local_10 * (float)puVar9[*(int *)(iVar10 * 0xb0 + 0xe454 + iVar5) + 0xb] <= fVar6) {
    fVar12 = (float10)0.0;
  }
  else {
    fVar12 = FUN_0040f120(pfVar4,(fVar6 * 3.6) / fVar13,(undefined4 *)0x0);
    param_1 = (float)fVar12;
    if (*(int *)(*(int *)((int)fVar7 + 0xe8) + 0x573c) != 0) {
      param_1 = param_1 * 1.2;
    }
    if (param_1 < 0.0) {
      param_1 = 0.0;
    }
    iVar2 = *(int *)((int)fVar7 + 0xe8);
    iVar10 = *(int *)(iVar2 + 0x54);
    puVar9 = FUN_004f73d0(iVar10);
    fVar12 = (float10)param_1 * (float10)(float)puVar9[*(int *)(iVar10 * 0xb0 + 0xe454 + iVar2) + 0xf];
  }
  fVar1 = (float)fVar12;
  if (param_2 != (float *)0x0) {
    puVar9 = FUN_004f73d0(iVar10);
    if (*(int *)puVar9[5] != 0) {
      if (*(int *)(*(int *)((int)fVar7 + 0xe8) + 0x573c) != 0) {
        puVar9 = FUN_004f73d0(*(int *)(*(int *)((int)fVar7 + 0xe8) + 0x54));
        fVar13 = fVar13 / (float)puVar9[4];
      }
      puVar14 = (undefined4 *)0x0;
      fVar13 = (fVar6 * 3.6) / fVar13;
      puVar9 = FUN_004f73d0(*(int *)(*(int *)((int)fVar7 + 0xe8) + 0x54));
      fVar12 = FUN_0040f120(*(float **)puVar9[5],fVar13,puVar14);
      *param_2 = (float)fVar12;
      if (*(int *)(*(int *)((int)fVar7 + 0xe8) + 0x573c) != 0) {
        FUN_004f73d0(*(int *)(*(int *)((int)fVar7 + 0xe8) + 0x54));
      }
    }
    fVar12 = (float10)fVar1;
  }
  if (fVar12 <= (float10)0.001) {
    return fVar12;
  }
  fVar12 = (float10)FUN_004ed970((int)fVar7,(float *)0x0);
  return fVar12 + (float10)fVar1;
}


/* ===== FUN_004eeea0 @ 004eeea0  size=992  callers: FUN_004ee5e0  ===== */

void __cdecl
FUN_004eeea0(int *param_1,undefined4 param_2,undefined4 param_3,float param_4,int *param_5,int param_6,
            int param_7)

{
  int iVar1;
  float *pfVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  int *piVar8;
  int *piVar9;
  int iVar10;
  undefined4 *puVar11;
  int iVar12;
  float *pfVar13;
  int local_20;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  piVar9 = param_5;
  piVar8 = param_1;
  iVar1 = param_1[0x3a] + 0x57f4;
  local_20 = 0;
  if (0 < *(int *)(param_1[0x3a] + 0x5928)) {
    pfVar13 = (float *)((int)param_4 + 0xc);
    iVar12 = 0;
    do {
      if (pfVar13[0x10] != 0.0) {
        iVar10 = FUN_00531b10((int)piVar8,local_20,(undefined4 *)0x0,(undefined4 *)0x0,(undefined4 *)0x0,
                              (undefined4 *)0x0,&param_1);
        if ((iVar10 == 0) || (*(float *)(piVar8[0x3a] + 0x5934 + iVar12) <= (float)param_1)) {
          iVar10 = FUN_00534fc0((int *)(pfVar13 + -3));
          if (iVar10 == 3) {
            local_c = *pfVar13;
            local_8 = pfVar13[1];
            local_4 = pfVar13[2];
            FUN_00474ed0(iVar1,&local_c,&local_18);
            param_4 = SQRT(local_18 * local_18 + local_14 * local_14 + local_10 * local_10);
            if ((1e-06 <= param_4) && (1e-06 <= ABS(param_4 - 1.0))) {
              fVar3 = 1.0 / param_4;
              local_18 = fVar3 * local_18;
              local_14 = local_14 * fVar3;
              local_10 = local_10 * fVar3;
            }
            pfVar2 = (float *)(param_6 + *piVar9 * 0xc);
            *pfVar2 = local_c;
            pfVar2[1] = local_8;
            pfVar2[2] = local_4;
            iVar10 = piVar8[0x3a];
            fVar3 = *(float *)(iVar10 + 0x58e0);
            fVar4 = *(float *)(iVar10 + 0x58e4);
            fVar5 = *(float *)(iVar10 + 0x58e8);
            pfVar2 = (float *)(param_7 + *piVar9 * 0xc);
            puVar11 = FUN_004f7420(*(int *)(iVar10 + 0x54));
            fVar3 = -((fVar3 * fVar3 + fVar4 * fVar4 + fVar5 * fVar5) * (float)puVar11[6]);
            *pfVar2 = fVar3 * local_18;
            pfVar2[1] = local_14 * fVar3;
            pfVar2[2] = local_10 * fVar3;
            *piVar9 = *piVar9 + 1;
          }
        }
        else {
          param_1 = (int *)(*(float *)(piVar8[0x3a] + 0x5934 + iVar12) - (float)param_1);
          local_c = *pfVar13;
          local_8 = pfVar13[1];
          local_4 = pfVar13[2];
          FUN_00474ed0(iVar1,&local_c,&local_18);
          fVar3 = SQRT(local_18 * local_18 + local_14 * local_14 + local_10 * local_10);
          if ((1e-06 <= fVar3) && (1e-06 <= ABS(fVar3 - 1.0))) {
            fVar3 = 1.0 / fVar3;
            local_18 = local_18 * fVar3;
            local_14 = local_14 * fVar3;
            local_10 = local_10 * fVar3;
          }
          pfVar2 = (float *)(param_6 + *piVar9 * 0xc);
          *pfVar2 = local_c;
          pfVar2[1] = local_8;
          pfVar2[2] = local_4;
          iVar10 = piVar8[0x3a];
          fVar3 = *(float *)(iVar10 + 0x58e0);
          fVar4 = *(float *)(iVar10 + 0x58e4);
          fVar5 = *(float *)(iVar10 + 0x58e8);
          param_5 = param_1;
          fVar7 = *(float *)(iVar10 + 0x5934 + iVar12) * 0.9;
          param_4 = *(float *)(iVar10 + 0x5934 + iVar12) * 0.3;
          puVar11 = FUN_004f7420(*(int *)(iVar10 + 0x54));
          fVar6 = (float)puVar11[7];
          if (param_4 <= (float)param_5) {
            if ((float)param_5 <= fVar7) {
              fVar7 = fVar7 - param_4;
              if (1e-06 <= ABS(fVar7)) {
                fVar6 = (fVar6 * ((float)param_5 - param_4)) / fVar7;
              }
              else {
                fVar6 = 0.0;
              }
            }
          }
          else {
            fVar6 = 0.0;
          }
          fVar3 = -(fVar6 * (fVar3 * fVar3 + fVar4 * fVar4 + fVar5 * fVar5));
          pfVar2 = (float *)(param_7 + *piVar9 * 0xc);
          *pfVar2 = local_18 * fVar3;
          pfVar2[1] = local_14 * fVar3;
          pfVar2[2] = local_10 * fVar3;
          *piVar9 = *piVar9 + 1;
        }
      }
      local_20 = local_20 + 1;
      iVar12 = iVar12 + 0x68;
      pfVar13 = pfVar13 + 0x26;
    } while (local_20 < *(int *)(piVar8[0x3a] + 0x5928));
  }
  return;
}


/* ===== FUN_004ee130 @ 004ee130  size=66  callers: FUN_004edac0 FUN_004ee180  ===== */

undefined4 __cdecl FUN_004ee130(int param_1)

{
  float fVar1;
  int iVar2;
  undefined4 *puVar3;
  
  iVar2 = *(int *)(param_1 + 0xe8);
  if ((*(byte *)(iVar2 + 0x5774) & 1) != 0) {
    fVar1 = *(float *)(iVar2 + 0x5c6c);
    puVar3 = FUN_004f7420(*(int *)(iVar2 + 0x54));
    if ((float)puVar3[3] < ABS(fVar1)) {
      return 1;
    }
  }
  return 0;
}


/* ===== FUN_004ee180 @ 004ee180  size=241  callers: FUN_004edac0 FUN_004ee5e0 FUN_004ee5e0  ===== */

float10 __cdecl FUN_004ee180(int param_1,int param_2)

{
  float fVar1;
  float fVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  undefined4 *puVar6;
  float *pfVar7;
  
  iVar5 = *(int *)(*(int *)(param_1 + 0xe8) + 0x54);
  iVar3 = *(int *)(iVar5 * 0xb0 + 0xe45c + *(int *)(param_1 + 0xe8));
  if ((-1 < iVar3) && (iVar3 < 4)) {
    puVar4 = FUN_004f7420(iVar5);
    fVar1 = (float)puVar4[iVar3 + 8];
    if (param_2 != 0) {
      puVar4 = FUN_004f7420(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
      fVar2 = (float)puVar4[1];
      puVar4 = FUN_004f7420(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
      puVar4[2] = fVar1 * fVar2;
      iVar5 = FUN_004ee130(param_1);
      if (iVar5 != 0) {
        puVar4 = FUN_004f7420(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
        puVar6 = FUN_004f7420(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
        puVar4[2] = (float)puVar6[5] * (float)puVar4[2];
      }
      puVar4 = FUN_004f7420(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
      return (float10)(float)puVar4[2];
    }
    pfVar7 = (float *)FUN_004f7420(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
    return (float10)fVar1 * (float10)*pfVar7;
  }
  return (float10)0.0;
}


/* ===== FUN_004ed6a0 @ 004ed6a0  size=711  callers: FUN_005018f0 FUN_004ee280 FUN_004ee280 FUN_004ee280 FUN_004ee5e0  ===== */

void __cdecl FUN_004ed6a0(int param_1,undefined4 *param_2,float param_3,int param_4)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *pfVar5;
  float *pfVar6;
  int iVar7;
  float *pfVar8;
  float *pfVar9;
  int iVar10;
  undefined4 *puVar11;
  undefined4 *puVar12;
  float local_58 [22];
  
  iVar10 = *(int *)(param_1 + 0xe8);
  puVar11 = param_2;
  puVar12 = (undefined4 *)(param_4 + 0x48);
  for (iVar7 = 0xf; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar12 = *puVar11;
    puVar11 = puVar11 + 1;
    puVar12 = puVar12 + 1;
  }
  *(float *)(param_4 + 0xc) = (float)param_2[0x16];
  *(undefined4 *)(param_4 + 0x10) = param_2[0x17];
  pfVar1 = (float *)(param_4 + 0x24);
  *(undefined4 *)(param_4 + 0x14) = param_2[0x18];
  *pfVar1 = (float)param_2[0x1c];
  *(undefined4 *)(param_4 + 0x28) = param_2[0x1d];
  *(undefined4 *)(param_4 + 0x2c) = param_2[0x1e];
  *(float *)(param_4 + 0x18) = *(float *)(param_4 + 0xc) - *(float *)(iVar10 + 0x5884);
  *(float *)(param_4 + 0x1c) = *(float *)(param_4 + 0x10) - *(float *)(iVar10 + 0x5888);
  *(float *)(param_4 + 0x20) = *(float *)(param_4 + 0x14) - *(float *)(iVar10 + 0x588c);
  local_58[0] = DAT_0055e9b0;
  local_58[1] = (float)DAT_0055e9b4;
  local_58[2] = (float)DAT_0055e9b8;
  FUN_0040cc60(local_58 + 6,param_3);
  iVar10 = 0;
  do {
    *(undefined4 *)((int)local_58 + iVar10 + 0xc) = 0;
    pfVar8 = local_58;
    fVar2 = *(float *)((int)local_58 + iVar10 + 0xc);
    pfVar5 = (float *)((int)local_58 + iVar10 + 0x18);
    iVar7 = 3;
    do {
      fVar3 = *pfVar5;
      fVar4 = *pfVar8;
      pfVar5 = pfVar5 + 4;
      pfVar8 = pfVar8 + 1;
      iVar7 = iVar7 + -1;
      fVar2 = fVar3 * fVar4 + fVar2;
    } while (iVar7 != 0);
    *(float *)((int)local_58 + iVar10 + 0xc) = fVar2;
    iVar10 = iVar10 + 4;
  } while (iVar10 < 0xc);
  local_58[0] = local_58[3];
  local_58[1] = local_58[4];
  local_58[2] = local_58[5];
  pfVar8 = local_58 + 3;
  pfVar5 = (float *)(param_1 + 0xf8);
  param_1 = 3;
  do {
    *pfVar8 = 0.0;
    pfVar9 = local_58;
    fVar2 = *pfVar8;
    iVar10 = 3;
    pfVar6 = pfVar5;
    do {
      fVar3 = *pfVar6;
      fVar4 = *pfVar9;
      pfVar6 = pfVar6 + 4;
      pfVar9 = pfVar9 + 1;
      iVar10 = iVar10 + -1;
      fVar2 = fVar3 * fVar4 + fVar2;
    } while (iVar10 != 0);
    *pfVar8 = fVar2;
    pfVar8 = pfVar8 + 1;
    pfVar5 = pfVar5 + 1;
    param_1 = param_1 + -1;
  } while (param_1 != 0);
  pfVar5 = (float *)(param_4 + 0x30);
  *pfVar5 = local_58[4] * *(float *)(param_4 + 0x2c) - local_58[5] * *(float *)(param_4 + 0x28);
  *(float *)(param_4 + 0x34) = local_58[5] * *pfVar1 - local_58[3] * *(float *)(param_4 + 0x2c);
  *(float *)(param_4 + 0x38) = local_58[3] * *(float *)(param_4 + 0x28) - local_58[4] * *pfVar1;
  fVar2 = SQRT(*pfVar5 * *pfVar5 +
               *(float *)(param_4 + 0x34) * *(float *)(param_4 + 0x34) +
               *(float *)(param_4 + 0x38) * *(float *)(param_4 + 0x38));
  if (fVar2 < 1e-06) {
    *pfVar5 = DAT_0055e990;
    *(undefined4 *)(param_4 + 0x34) = DAT_0055e994;
    *(undefined4 *)(param_4 + 0x38) = DAT_0055e998;
  }
  else if (1e-06 <= ABS(fVar2 - 1.0)) {
    fVar2 = 1.0 / fVar2;
    *pfVar5 = fVar2 * *pfVar5;
    *(float *)(param_4 + 0x34) = fVar2 * *(float *)(param_4 + 0x34);
    *(float *)(param_4 + 0x38) = fVar2 * *(float *)(param_4 + 0x38);
  }
  pfVar8 = (float *)(param_4 + 0x3c);
  *pfVar8 = *(float *)(param_4 + 0x38) * *(float *)(param_4 + 0x28) -
            *(float *)(param_4 + 0x2c) * *(float *)(param_4 + 0x34);
  *(float *)(param_4 + 0x40) = *(float *)(param_4 + 0x2c) * *pfVar5 - *pfVar1 * *(float *)(param_4 + 0x38);
  *(float *)(param_4 + 0x44) = *pfVar1 * *(float *)(param_4 + 0x34) - *pfVar5 * *(float *)(param_4 + 0x28);
  fVar2 = SQRT(*pfVar8 * *pfVar8 +
               *(float *)(param_4 + 0x40) * *(float *)(param_4 + 0x40) +
               *(float *)(param_4 + 0x44) * *(float *)(param_4 + 0x44));
  if (fVar2 < 1e-06) {
    *pfVar8 = DAT_0055e990;
    *(undefined4 *)(param_4 + 0x40) = DAT_0055e994;
    *(undefined4 *)(param_4 + 0x44) = DAT_0055e998;
  }
  else if (1e-06 <= ABS(fVar2 - 1.0)) {
    fVar2 = 1.0 / fVar2;
    *pfVar8 = fVar2 * *pfVar8;
    *(float *)(param_4 + 0x40) = fVar2 * *(float *)(param_4 + 0x40);
    *(float *)(param_4 + 0x44) = fVar2 * *(float *)(param_4 + 0x44);
    return;
  }
  return;
}


