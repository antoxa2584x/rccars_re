/* ===== FUN_00474910 @ 00474910  size=67  callers: FUN_004f5290 FUN_005037f0 FUN_004757a0 FUN_004754a0 FUN_004754a0 FUN_0050d990 FUN_00475c80 FUN_004edac0 FUN_00476ba0  ===== */

void __cdecl FUN_00474910(float *param_1,float *param_2,float *param_3)

{
  float fVar1;
  float fVar2;
  float *pfVar3;
  float *pfVar4;
  int iVar5;
  int iVar6;
  
  iVar5 = 3;
  do {
    *param_3 = 0.0;
    iVar6 = 3;
    pfVar3 = param_1;
    pfVar4 = param_2;
    do {
      fVar1 = *pfVar3;
      fVar2 = *pfVar4;
      pfVar3 = pfVar3 + 1;
      pfVar4 = pfVar4 + 1;
      iVar6 = iVar6 + -1;
      *param_3 = fVar1 * fVar2 + *param_3;
    } while (iVar6 != 0);
    param_3 = param_3 + 1;
    param_1 = param_1 + 4;
    iVar5 = iVar5 + -1;
  } while (iVar5 != 0);
  return;
}


/* ===== FUN_00474e00 @ 00474e00  size=207  callers: FUN_004f5290 FUN_004757a0 FUN_004754a0 FUN_004754a0 FUN_0050d990 FUN_004edac0 FUN_00476ba0  ===== */

void __cdecl FUN_00474e00(int param_1)

{
  float fVar1;
  float fVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  float *pfVar6;
  float *pfVar7;
  float local_c0 [4];
  undefined4 local_b0;
  undefined4 local_a8;
  undefined4 local_a0;
  undefined4 local_9c;
  float local_80 [4];
  undefined4 local_70;
  undefined4 local_68;
  undefined4 local_60;
  undefined4 local_5c;
  float local_40 [16];
  
  FUN_00407d40((float *)(param_1 + 0x9c),local_c0);
  uVar4 = local_a8;
  uVar3 = local_b0;
  fVar2 = local_c0[2];
  fVar1 = local_c0[1];
  local_c0[1] = (float)local_b0;
  local_b0 = fVar1;
  local_c0[2] = (float)local_a0;
  local_a0 = fVar2;
  local_a8 = local_9c;
  local_9c = uVar4;
  pfVar6 = local_c0;
  pfVar7 = local_80;
  for (iVar5 = 0x10; uVar4 = local_68, fVar2 = local_80[2], iVar5 != 0; iVar5 = iVar5 + -1) {
    *pfVar7 = *pfVar6;
    pfVar6 = pfVar6 + 1;
    pfVar7 = pfVar7 + 1;
  }
  local_80[1] = fVar1;
  local_70 = uVar3;
  local_80[2] = (float)local_60;
  local_60 = fVar2;
  local_68 = local_5c;
  local_5c = uVar4;
  FUN_0040c7f0((int)local_c0,param_1 + 0x50,local_40);
  FUN_0040c7f0((int)local_40,(int)local_80,(float *)(param_1 + 0xac));
  return;
}


/* ===== FUN_00407d40 @ 00407d40  size=362  callers: FUN_00408110 FUN_00408110 FUN_00474e00 FUN_00475030 FUN_00474d30 FUN_00407620 FUN_00415890 FUN_00476420 FUN_004083f0 FUN_004ec770  ===== */

void __cdecl FUN_00407d40(float *param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  int iVar9;
  float *pfVar10;
  
  fVar1 = *param_1;
  fVar2 = param_1[3];
  fVar3 = param_1[2];
  fVar4 = param_1[1];
  fVar8 = fVar1 * fVar1 + fVar4 * fVar4 + fVar3 * fVar3 + fVar2 * fVar2;
  if (fVar8 < 1e-06) {
    pfVar10 = (float *)&DAT_0055e940;
    for (iVar9 = 0x10; iVar9 != 0; iVar9 = iVar9 + -1) {
      *param_2 = *pfVar10;
      pfVar10 = pfVar10 + 1;
      param_2 = param_2 + 1;
    }
    return;
  }
  fVar8 = 2.0 / fVar8;
  param_2[3] = 0.0;
  param_2[7] = 0.0;
  param_2[0xb] = 0.0;
  param_2[0xf] = 1.0;
  param_2[0xc] = 0.0;
  param_2[0xd] = 0.0;
  param_2[0xe] = 0.0;
  fVar7 = fVar8 * fVar3;
  fVar6 = fVar8 * fVar2;
  fVar5 = fVar8 * fVar4 * fVar1;
  fVar8 = fVar8 * fVar4 * fVar4;
  *param_2 = 1.0 - (fVar6 * fVar2 + fVar7 * fVar3);
  param_2[1] = fVar7 * fVar4 + fVar6 * fVar1;
  param_2[2] = fVar6 * fVar4 - fVar7 * fVar1;
  param_2[4] = fVar7 * fVar4 - fVar6 * fVar1;
  param_2[5] = 1.0 - (fVar6 * fVar2 + fVar8);
  param_2[6] = fVar6 * fVar3 + fVar5;
  param_2[8] = fVar6 * fVar4 + fVar7 * fVar1;
  param_2[9] = fVar6 * fVar3 - fVar5;
  param_2[10] = 1.0 - (fVar7 * fVar3 + fVar8);
  return;
}


/* ===== FUN_004754a0 @ 004754a0  size=543  callers: FUN_004f0840  ===== */

void __cdecl FUN_004754a0(float param_1,float param_2,float *param_3)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  fVar6 = param_1;
  iVar1 = *(int *)((int)param_1 + 0x3c);
  fVar2 = *(float *)((int)param_1 + 0x58) - *(float *)(iVar1 + 0x90);
  fVar3 = *(float *)((int)param_1 + 0x5c) - *(float *)(iVar1 + 0x94);
  fVar4 = *(float *)((int)param_1 + 0x60) - *(float *)(iVar1 + 0x98);
  local_c = fVar3 * *(float *)((int)param_1 + 0x78) - fVar4 * *(float *)((int)param_1 + 0x74);
  local_8 = fVar4 * *(float *)((int)param_1 + 0x70) - fVar2 * *(float *)((int)param_1 + 0x78);
  local_4 = fVar2 * *(float *)((int)param_1 + 0x74) - fVar3 * *(float *)((int)param_1 + 0x70);
  FUN_00474e00(iVar1);
  FUN_00474910((float *)(*(int *)((int)param_1 + 0x3c) + 0xac),&local_c,&local_18);
  fVar2 = (local_14 * fVar4 - local_10 * fVar3) * *(float *)((int)param_1 + 0x70) +
          (local_10 * fVar2 - fVar4 * local_18) * *(float *)((int)param_1 + 0x74) +
          (fVar3 * local_18 - local_14 * fVar2) * *(float *)((int)param_1 + 0x78) +
          *(float *)(*(int *)((int)param_1 + 0x3c) + 4);
  iVar1 = *(int *)((int)param_1 + 0x40);
  if (iVar1 != 0) {
    fVar3 = *(float *)((int)param_1 + 100) - *(float *)(iVar1 + 0x90);
    fVar4 = *(float *)((int)param_1 + 0x68) - *(float *)(iVar1 + 0x94);
    fVar5 = *(float *)((int)param_1 + 0x6c) - *(float *)(iVar1 + 0x98);
    local_c = fVar4 * *(float *)((int)param_1 + 0x78) - fVar5 * *(float *)((int)param_1 + 0x74);
    local_8 = fVar5 * *(float *)((int)param_1 + 0x70) - fVar3 * *(float *)((int)param_1 + 0x78);
    local_4 = fVar3 * *(float *)((int)param_1 + 0x74) - fVar4 * *(float *)((int)param_1 + 0x70);
    FUN_00474e00(iVar1);
    FUN_00474910((float *)(*(int *)((int)param_1 + 0x40) + 0xac),&local_c,&local_18);
    fVar2 = (local_14 * fVar5 - local_10 * fVar4) * *(float *)((int)param_1 + 0x70) +
            (local_10 * fVar3 - fVar5 * local_18) * *(float *)((int)param_1 + 0x74) +
            (fVar4 * local_18 - local_14 * fVar3) * *(float *)((int)param_1 + 0x78) +
            *(float *)(*(int *)((int)param_1 + 0x40) + 4) + fVar2;
  }
  param_1 = fVar2;
  if (ABS(param_1) < 1e-06) {
    *param_3 = DAT_0070c658;
    param_3[1] = DAT_0070c65c;
    param_3[2] = DAT_0070c660;
    return;
  }
  fVar2 = param_2 / param_1;
  *param_3 = fVar2 * *(float *)((int)fVar6 + 0x70);
  param_3[1] = fVar2 * *(float *)((int)fVar6 + 0x74);
  param_3[2] = fVar2 * *(float *)((int)fVar6 + 0x78);
  return;
}


/* ===== FUN_004757a0 @ 004757a0  size=158  callers: FUN_00475840 FUN_004756c0  ===== */

void __cdecl FUN_004757a0(int param_1,float param_2)

{
  float *pfVar1;
  float fVar2;
  
  FUN_00474e00(param_1);
  pfVar1 = (float *)(param_1 + 0x110);
  *(float *)(param_1 + 0x104) = param_2 * *(float *)(param_1 + 0x11c) + *(float *)(param_1 + 0x104);
  *(float *)(param_1 + 0x108) = param_2 * *(float *)(param_1 + 0x120) + *(float *)(param_1 + 0x108);
  *(float *)(param_1 + 0x10c) = param_2 * *(float *)(param_1 + 0x124) + *(float *)(param_1 + 0x10c);
  *pfVar1 = param_2 * *(float *)(param_1 + 0x128) + *pfVar1;
  *(float *)(param_1 + 0x114) = param_2 * *(float *)(param_1 + 300) + *(float *)(param_1 + 0x114);
  *(float *)(param_1 + 0x118) = param_2 * *(float *)(param_1 + 0x130) + *(float *)(param_1 + 0x118);
  fVar2 = *(float *)(param_1 + 4);
  *(float *)(param_1 + 0xec) = fVar2 * *(float *)(param_1 + 0x104);
  *(float *)(param_1 + 0xf0) = fVar2 * *(float *)(param_1 + 0x108);
  *(float *)(param_1 + 0xf4) = fVar2 * *(float *)(param_1 + 0x10c);
  FUN_00474910((float *)(param_1 + 0xac),pfVar1,(float *)(param_1 + 0xf8));
  return;
}


/* ===== FUN_00476ba0 @ 00476ba0  size=162  callers: FUN_00476c50  ===== */

void __cdecl FUN_00476ba0(int param_1,int param_2,float *param_3,int param_4,float *param_5)

{
  float local_c;
  float local_8;
  float local_4;
  
  FUN_00474e00(param_1);
  FUN_00475100(param_1,param_3,param_2,param_4);
  local_c = (*(float *)(param_1 + 0x100) * *(float *)(param_1 + 0x114) -
            *(float *)(param_1 + 0xfc) * *(float *)(param_1 + 0x118)) + *(float *)(param_1 + 0x128);
  local_8 = (*(float *)(param_1 + 0x118) * *(float *)(param_1 + 0xf8) -
            *(float *)(param_1 + 0x100) * *(float *)(param_1 + 0x110)) + *(float *)(param_1 + 300);
  local_4 = (*(float *)(param_1 + 0xfc) * *(float *)(param_1 + 0x110) -
            *(float *)(param_1 + 0x114) * *(float *)(param_1 + 0xf8)) + *(float *)(param_1 + 0x130);
  FUN_00474910((float *)(param_1 + 0xac),&local_c,param_5);
  return;
}


/* ===== FUN_00476c50 @ 00476c50  size=624  callers: FUN_004edac0  ===== */

void __cdecl
FUN_00476c50(int param_1,float *param_2,float *param_3,int param_4,float *param_5,int param_6,float *param_7)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  *param_7 = DAT_0070c658;
  param_7[1] = DAT_0070c65c;
  param_7[2] = DAT_0070c660;
  fVar4 = *param_2 - *(float *)(param_1 + 0x90);
  fVar5 = param_2[1] - *(float *)(param_1 + 0x94);
  fVar6 = param_2[2] - *(float *)(param_1 + 0x98);
  FUN_00475100(param_1,param_5,param_4,param_6);
  fVar1 = *(float *)(param_1 + 4);
  fVar2 = *(float *)(param_1 + 0x120);
  fVar3 = *(float *)(param_1 + 0x124);
  *param_7 = fVar1 * *(float *)(param_1 + 0x11c) + *param_7;
  param_7[1] = fVar1 * fVar2 + param_7[1];
  param_7[2] = fVar1 * fVar3 + param_7[2];
  FUN_00476ba0(param_1,param_4,param_5,param_6,&local_18);
  *param_7 = (local_14 * fVar6 - local_10 * fVar5) + *param_7;
  param_7[1] = (local_10 * fVar4 - fVar6 * local_18) + param_7[1];
  param_7[2] = (local_18 * fVar5 - local_14 * fVar4) + param_7[2];
  fVar7 = fVar6 * *(float *)(param_1 + 0xfc) - fVar5 * *(float *)(param_1 + 0x100);
  fVar6 = fVar4 * *(float *)(param_1 + 0x100) - fVar6 * *(float *)(param_1 + 0xf8);
  fVar5 = fVar5 * *(float *)(param_1 + 0xf8) - fVar4 * *(float *)(param_1 + 0xfc);
  fVar1 = *(float *)(param_1 + 0x100);
  fVar2 = *(float *)(param_1 + 0xf8);
  fVar3 = *(float *)(param_1 + 0xf8);
  fVar4 = *(float *)(param_1 + 0xfc);
  fVar18 = (fVar5 * *(float *)(param_1 + 0xfc) - fVar6 * *(float *)(param_1 + 0x100)) + *param_7;
  *param_7 = fVar18;
  fVar2 = (fVar7 * fVar1 - fVar5 * fVar2) + param_7[1];
  param_7[1] = fVar2;
  fVar1 = (fVar6 * fVar3 - fVar7 * fVar4) + param_7[2];
  param_7[2] = fVar1;
  if (param_3 != (float *)0x0) {
    fVar3 = param_3[2];
    fVar4 = param_3[1];
    fVar5 = *param_3;
    fVar6 = *(float *)(param_1 + 0xfc);
    fVar7 = param_3[2];
    fVar8 = *(float *)(param_1 + 0x100);
    fVar9 = param_3[1];
    fVar10 = *(float *)(param_1 + 0x100);
    fVar11 = *param_3;
    fVar12 = param_3[2];
    fVar13 = *(float *)(param_1 + 0xf8);
    fVar14 = param_3[1];
    fVar15 = *(float *)(param_1 + 0xf8);
    fVar16 = *(float *)(param_1 + 0xfc);
    fVar17 = *param_3;
    FUN_00474ed0(param_1,param_2,&local_c);
    fVar1 = local_c * (fVar6 * fVar7 - fVar8 * fVar9) +
            local_8 * (fVar10 * fVar11 - fVar12 * fVar13) + local_4 * (fVar14 * fVar15 - fVar16 * fVar17) +
            fVar18 * fVar5 + fVar2 * fVar4 + fVar1 * fVar3;
    *param_7 = fVar1 * *param_3;
    param_7[1] = fVar1 * param_3[1];
    param_7[2] = fVar1 * param_3[2];
    return;
  }
  return;
}


/* ===== FUN_0040f690 @ 0040f690  size=37  callers: FUN_005353b0 FUN_0052a030 FUN_004e9980 FUN_0044d080 FUN_0044d3a0 FUN_0044e2f0 FUN_004106d0 FUN_004749d0 FUN_004504b0 FUN_0045a220 FUN_0045a220 FUN_0045a220  ===== */

void __cdecl FUN_0040f690(float *param_1,float *param_2,float *param_3)

{
  *param_3 = *param_1 - *param_2;
  param_3[1] = param_1[1] - param_2[1];
  param_3[2] = param_1[2] - param_2[2];
  return;
}


/* ===== FUN_0040f6c0 @ 0040f6c0  size=37  callers: FUN_004f0560 FUN_0052a030 FUN_0052a030 FUN_004e9980 FUN_0044d080 FUN_0044d3a0 FUN_0044d970 FUN_0044e2f0 FUN_0044e2f0 FUN_004106d0 FUN_004749d0 FUN_0045a220 FUN_0045a220  ===== */

void __cdecl FUN_0040f6c0(float *param_1,float param_2,float *param_3)

{
  *param_3 = param_2 * *param_1;
  param_3[1] = param_2 * param_1[1];
  param_3[2] = param_2 * param_1[2];
  return;
}


/* ===== FUN_0040f6f0 @ 0040f6f0  size=29  callers: FUN_0052a030 FUN_004e9980 FUN_0044d080 FUN_0044d3a0 FUN_004749d0 FUN_0045a220 FUN_0045a220  ===== */

float10 __cdecl FUN_0040f6f0(float *param_1,float *param_2)

{
  return (float10)*param_1 * (float10)*param_2 +
         (float10)param_1[1] * (float10)param_2[1] + (float10)param_1[2] * (float10)param_2[2];
}


/* ===== FUN_00410130 @ 00410130  size=30  callers: FUN_00471ca0 FUN_00471ca0 FUN_00471ca0 FUN_00407620 FUN_0040b4c0 FUN_00500840 FUN_00408760 FUN_00406a00 FUN_004fe310 FUN_0050cea0 FUN_0050cea0 FUN_0044c220 FUN_0040d310 FUN_004f3b80 FUN_004f6940 FUN_004f6940 FUN_0050b400 FUN_0050b400 FUN_0044e2f0 carVisualSuspInner carVisualSuspInner FUN_004749d0 FUN_004749d0 FUN_004ee5e0 FUN_004ee5e0  ===== */

float10 __cdecl FUN_00410130(float *param_1,float *param_2)

{
  float10 fVar1;
  
  FUN_00410080(param_1,param_2);
  fVar1 = (float10)FUN_00547be0();
  return fVar1 * (float10)57.295776;
}


