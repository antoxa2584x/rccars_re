/* ===== FUN_0040ef60 @ 0040ef60  size=176  callers: r3dSplImportTxt r3dSplImportACV FUN_0042e2e0 FUN_0040f090 FUN_0049d8d0  ===== */

int * __cdecl FUN_0040ef60(int param_1,int param_2,int param_3)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_00402ab0(1,0x1c);
  if (piVar1 == (int *)0x0) {
    return (int *)0x0;
  }
  piVar1[1] = param_2;
  piVar1[2] = param_3;
  *piVar1 = param_1;
  piVar1[3] = 0;
  if (param_1 < 0x232) {
    if (param_1 == 0x231) {
LAB_0040efca:
      piVar1[4] = 0x14;
      return piVar1;
    }
    if (param_1 < 0x212) {
      if (param_1 != 0x211) {
        if (param_1 == 0) {
          piVar1[4] = 0;
          return piVar1;
        }
        if (param_1 != 0x111) goto LAB_0040f004;
      }
LAB_0040efae:
      piVar1[4] = 0xc;
      return piVar1;
    }
    if (param_1 == 0x221) {
      piVar1[4] = 0x10;
      return piVar1;
    }
  }
  else if (param_1 < 0x432) {
    if (param_1 == 0x431) {
      piVar1[4] = 0x2c;
      return piVar1;
    }
    if (param_1 == 0x311) goto LAB_0040efca;
    if (param_1 == 0x421) {
      piVar1[4] = 0x20;
      return piVar1;
    }
  }
  else if (param_1 == 0x511) goto LAB_0040efae;
LAB_0040f004:
  FUN_00402ad0((undefined *)piVar1);
  return (int *)0x0;
}


/* ===== FUN_0040f1e0 @ 0040f1e0  size=212  callers: FUN_0040f320 FUN_0040f710 FUN_0040f710 FUN_0040f830 FUN_0040f830 FUN_0040fa80 FUN_0040fa80 FUN_0040fe20 FUN_0040fe20  ===== */

int __cdecl FUN_0040f1e0(int param_1,float param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  float *pfVar5;
  
  iVar4 = *(int *)(param_1 + 0xc);
  if (iVar4 == 0) {
    return -4;
  }
  iVar2 = *(int *)(param_1 + 0x14);
  if (param_2 < *(float *)(iVar2 + 4)) {
    return -3;
  }
  iVar3 = *(int *)(param_1 + 0x10);
  iVar1 = iVar4 + -1;
  if (*(float *)(iVar3 * iVar1 + 4 + iVar2) <= param_2) {
    return -2;
  }
  if ((*(int *)(param_1 + 0x18) < 0) || (iVar4 <= *(int *)(param_1 + 0x18))) {
    *(undefined4 *)(param_1 + 0x18) = 0;
  }
  iVar4 = *(int *)(param_1 + 0x18);
  if (param_2 <= *(float *)(iVar4 * iVar3 + 4 + iVar2)) {
    iVar4 = 0;
    if (0 < iVar1) {
      pfVar5 = (float *)(iVar3 + 4 + iVar2);
      while (*pfVar5 <= param_2) {
        iVar4 = iVar4 + 1;
        pfVar5 = (float *)((int)pfVar5 + iVar3);
        if (iVar1 <= iVar4) {
          return -1;
        }
      }
LAB_0040f2ab:
      *(int *)(param_1 + 0x18) = iVar4;
      return iVar4;
    }
  }
  else if (iVar4 < iVar1) {
    pfVar5 = (float *)((iVar4 + 1) * iVar3 + 4 + iVar2);
    do {
      if (param_2 < *pfVar5) goto LAB_0040f2ab;
      iVar4 = iVar4 + 1;
      pfVar5 = (float *)((int)pfVar5 + iVar3);
    } while (iVar4 < iVar1);
  }
  return -1;
}


/* ===== FUN_0040c6d0 @ 0040c6d0  size=118  callers: FUN_0040f710 FUN_0040f830 FUN_0040fa80 FUN_0040fe20 FUN_004ef280  ===== */

float10 __cdecl FUN_0040c6d0(float param_1,float param_2,float param_3)

{
  float fVar1;
  float10 fVar2;
  
  fVar1 = param_3 - param_2;
  fVar2 = (float10)param_1 - (float10)param_2;
  if (1e-06 <= ABS(fVar1)) {
    fVar2 = FUN_00548015((double)(fVar2 / (float10)fVar1));
    fVar2 = fVar2 * (float10)fVar1;
  }
  fVar2 = (float10)param_1 - fVar2;
  if (fVar2 < (float10)param_2) {
    return (float10)param_2;
  }
  if ((float10)(param_3 - 1e-06) < fVar2) {
    fVar2 = (float10)(param_3 - 1e-06);
  }
  return fVar2;
}


