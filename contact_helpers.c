/* ===== FUN_0040cc60 @ 0040cc60  size=68  callers: FUN_004ef680 FUN_004ef680 FUN_00471ca0 FUN_00471ca0 FUN_0040b4c0 carVisualBody FUN_0052b860 FUN_004ed6a0 FUN_00511b50 FUN_005184b0 FUN_005184b0 FUN_005184b0 FUN_00518050 FUN_004b0fa0 FUN_0051eb40 FUN_00510a40 FUN_0045aa50 FUN_00514760 FUN_00514760 FUN_00519b80 FUN_00514100 FUN_00514100 carVisualUpdate FUN_0040d430 FUN_0050d3f0  ===== */

void __cdecl FUN_0040cc60(float *param_1,float param_2)

{
  int iVar1;
  float *pfVar2;
  float10 fVar3;
  float10 fVar4;
  
  iVar1 = 0x10;
  fVar3 = (float10)fcos((float10)param_2 * (float10)0.017453292);
  pfVar2 = param_1;
  for (; iVar1 != 0; iVar1 = iVar1 + -1) {
    *pfVar2 = 0.0;
    pfVar2 = pfVar2 + 1;
  }
  param_1[5] = 1.0;
  param_1[0xf] = 1.0;
  fVar4 = (float10)fsin((float10)(float)((float10)param_2 * (float10)0.017453292));
  *param_1 = (float)fVar3;
  param_1[2] = (float)-fVar4;
  param_1[8] = (float)fVar4;
  param_1[10] = (float)fVar3;
  return;
}


/* ===== FUN_00408c20 @ 00408c20  size=224  callers: carContactSolve  ===== */

undefined4 __cdecl FUN_00408c20(float *param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *pfVar4;
  float *pfVar5;
  float *pfVar6;
  int iVar7;
  int iVar8;
  float local_10 [4];
  
  param_2[2] = 0.0;
  param_2[5] = 0.0;
  param_2[8] = 1.0;
  fVar1 = param_1[4] * *param_1 - param_1[3] * param_1[1];
  if (ABS(fVar1) < 1e-06) {
    return 0;
  }
  fVar1 = 1.0 / fVar1;
  pfVar6 = local_10 + 2;
  iVar7 = 2;
  *param_2 = param_1[4] * fVar1;
  param_2[4] = fVar1 * *param_1;
  param_2[1] = -(param_1[1] * fVar1);
  param_2[3] = -(param_1[3] * fVar1);
  local_10[0] = -param_1[6];
  local_10[1] = -param_1[7];
  do {
    *pfVar6 = 0.0;
    pfVar5 = local_10;
    fVar1 = *pfVar6;
    pfVar4 = (float *)(((int)param_2 - (int)(local_10 + 2)) + (int)pfVar6);
    iVar8 = 2;
    do {
      fVar2 = *pfVar4;
      fVar3 = *pfVar5;
      pfVar4 = pfVar4 + 3;
      pfVar5 = pfVar5 + 1;
      iVar8 = iVar8 + -1;
      fVar1 = fVar2 * fVar3 + fVar1;
    } while (iVar8 != 0);
    *pfVar6 = fVar1;
    pfVar6 = pfVar6 + 1;
    iVar7 = iVar7 + -1;
  } while (iVar7 != 0);
  param_2[6] = local_10[2];
  param_2[7] = local_10[3];
  return 1;
}


/* ===== FUN_0040f120 @ 0040f120  size=181  callers: FUN_0044ba60 FUN_0044ba60 FUN_004aa9f0 FUN_00471a70 FUN_004ab8f0 FUN_004ab8f0 FUN_004ab8f0 FUN_004ab8f0 FUN_004ab8f0 FUN_004abd50 FUN_00528ed0 FUN_00531ff0 FUN_00532d80 FUN_00459ae0 FUN_00459ae0 FUN_00459ae0 FUN_004f2d60 FUN_0049de60 carEngineForce carEngineForce  ===== */

float10 __cdecl FUN_0040f120(float *param_1,float param_2,undefined4 *param_3)

{
  float fVar1;
  undefined4 *puVar2;
  undefined4 **ppuVar3;
  float10 fVar4;
  
  ppuVar3 = (undefined4 **)param_3;
  if (param_3 == (undefined4 *)0x0) {
    ppuVar3 = &param_3;
  }
  *ppuVar3 = (undefined4 *)0x0;
  if ((param_1 != (float *)0x0) && (fVar1 = *param_1, fVar1 != 0.0)) {
    if ((int)fVar1 < 0x312) {
      if (fVar1 == 1.10002e-42) {
        fVar4 = FUN_0040f830((int *)param_1,param_2,ppuVar3);
        return fVar4;
      }
      if (fVar1 == 3.82554e-43) {
        fVar4 = FUN_0040f710((int *)param_1,param_2,ppuVar3);
        return fVar4;
      }
      if (fVar1 == 7.41287e-43) {
        fVar4 = FUN_0040fe20((int *)param_1,param_2,ppuVar3);
        return fVar4;
      }
    }
    else {
      if (fVar1 == 1.48117e-42) {
        puVar2 = (undefined4 *)FUN_0040f440(param_1,param_2,&param_2);
        *ppuVar3 = puVar2;
        return (float10)param_2;
      }
      if (fVar1 == 1.81748e-42) {
        fVar4 = FUN_0040fa80((int *)param_1,param_2,ppuVar3);
        return fVar4;
      }
    }
  }
  return (float10)0.0;
}


/* ===== FUN_00476c50 @ 00476c50  size=624  callers: carContactSolve  ===== */

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
  rbSumForcesTorques(param_1,param_5,param_4,param_6);
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
    rbPointVelocity(param_1,param_2,&local_c);
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


/* ===== FUN_00476ba0 @ 00476ba0  size=162  callers: FUN_00476c50  ===== */

void __cdecl FUN_00476ba0(int param_1,int param_2,float *param_3,int param_4,float *param_5)

{
  float local_c;
  float local_8;
  float local_4;
  
  rbUpdateInvInertiaWorld(param_1);
  rbSumForcesTorques(param_1,param_3,param_2,param_4);
  local_c = (*(float *)(param_1 + 0x100) * *(float *)(param_1 + 0x114) -
            *(float *)(param_1 + 0xfc) * *(float *)(param_1 + 0x118)) + *(float *)(param_1 + 0x128);
  local_8 = (*(float *)(param_1 + 0x118) * *(float *)(param_1 + 0xf8) -
            *(float *)(param_1 + 0x100) * *(float *)(param_1 + 0x110)) + *(float *)(param_1 + 300);
  local_4 = (*(float *)(param_1 + 0xfc) * *(float *)(param_1 + 0x110) -
            *(float *)(param_1 + 0x114) * *(float *)(param_1 + 0xf8)) + *(float *)(param_1 + 0x130);
  rbMat3MulVec3((float *)(param_1 + 0xac),&local_c,param_5);
  return;
}


/* ===== FUN_00531b10 @ 00531b10  size=265  callers: FUN_0050acf0 FUN_0050acf0 FUN_005262e0 FUN_00528ed0 FUN_0052f310 carTireForce  ===== */

undefined4 __cdecl
FUN_00531b10(int param_1,int param_2,undefined4 *param_3,undefined4 *param_4,undefined4 *param_5,
            undefined4 *param_6,undefined4 *param_7)

{
  undefined4 *puVar1;
  int iVar2;
  
  iVar2 = *(int *)(*(int *)(param_1 + 0xe8) + 0x5d40);
  if (iVar2 == 0) {
    if (param_3 != (undefined4 *)0x0) {
      *param_3 = 0;
    }
    if (param_4 != (undefined4 *)0x0) {
      *param_4 = DAT_0070c658;
      param_4[1] = DAT_0070c65c;
      param_4[2] = DAT_0070c660;
    }
    if (param_5 != (undefined4 *)0x0) {
      *param_5 = DAT_0055e9a0;
      param_5[1] = DAT_0055e9a4;
      param_5[2] = DAT_0055e9a8;
    }
    if (param_6 != (undefined4 *)0x0) {
      *param_6 = 0;
    }
    if (param_7 != (undefined4 *)0x0) {
      *param_7 = 0;
    }
    return 0;
  }
  iVar2 = *(int *)(iVar2 + 0xc);
  if (param_3 != (undefined4 *)0x0) {
    *param_3 = *(undefined4 *)(iVar2 + 0x34 + param_2 * 0x28);
  }
  if (param_4 != (undefined4 *)0x0) {
    puVar1 = (undefined4 *)(iVar2 + 0x1c + param_2 * 0x28);
    *param_4 = *puVar1;
    param_4[1] = puVar1[1];
    param_4[2] = puVar1[2];
  }
  if (param_5 != (undefined4 *)0x0) {
    puVar1 = (undefined4 *)(iVar2 + (param_2 * 5 + 5) * 8);
    *param_5 = *puVar1;
    param_5[1] = puVar1[1];
    param_5[2] = puVar1[2];
  }
  if (param_6 != (undefined4 *)0x0) {
    *param_6 = *(undefined4 *)(iVar2 + 0x18 + param_2 * 0x28);
  }
  if (param_7 != (undefined4 *)0x0) {
    *param_7 = *(undefined4 *)(iVar2 + 0x14 + param_2 * 0x28);
  }
  return *(undefined4 *)(iVar2 + 0x10 + param_2 * 0x28);
}


/* ===== FUN_00534fc0 @ 00534fc0  size=158  callers: FUN_0050acf0 FUN_0052f310 FUN_004f68a0 carTireForce  ===== */

int __cdecl FUN_00534fc0(int *param_1)

{
  byte bVar1;
  byte *pbVar2;
  int3 extraout_var;
  int iVar4;
  int iVar5;
  int iVar3;
  
  if ((*(byte *)(param_1 + 7) & 4) == 0) {
    return 0;
  }
  pbVar2 = (byte *)FUN_00455730(DAT_0148f908,param_1);
  if (pbVar2 != (byte *)0x0) {
    if ((*pbVar2 & 2) != 0) {
      return 7;
    }
    iVar5 = 0;
    iVar4 = 0xb;
    iVar3 = *(int *)(pbVar2 + 0xf0) + param_1[2] * 0x18;
    if (*(char *)(*(int *)(pbVar2 + 0xf0) + 1 + param_1[2] * 0x18) != '\0') {
      do {
        iVar3 = *(int *)(*(int *)(iVar3 + 8) + iVar5 * 4);
        if (iVar3 != 0) {
          bVar1 = FUN_00535060(*(uint *)(iVar3 + 0x3c));
          iVar3 = CONCAT31(extraout_var,bVar1);
          if ((iVar3 != 0 && -1 < extraout_var) && (iVar3 < iVar4)) {
            iVar4 = iVar3;
          }
        }
        iVar5 = iVar5 + 1;
        iVar3 = *(int *)(pbVar2 + 0xf0) + param_1[2] * 0x18;
      } while (iVar5 < (int)(uint)*(byte *)(*(int *)(pbVar2 + 0xf0) + 1 + param_1[2] * 0x18));
      if (iVar4 != 0xb) {
        return iVar4;
      }
    }
    return 0;
  }
  return 0;
}


