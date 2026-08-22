/* ===== FUN_004ef680 @ 004ef680  size=801  callers: FUN_004ef650 FUN_004ef9b0  ===== */

void __cdecl
FUN_004ef680(int param_1,int param_2,int param_3,float *param_4,undefined4 *param_5,float *param_6,
            float *param_7)

{
  float fVar1;
  float fVar2;
  float fVar3;
  undefined4 *puVar4;
  float *pfVar5;
  float *pfVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  float local_60;
  float local_58 [22];
  
  local_60 = 0.0;
  iVar10 = *(int *)(param_1 + 0xe8);
  iVar8 = param_2 * 0x68;
  iVar7 = iVar10 + 0x5948 + iVar8;
  local_58[0] = *(float *)(iVar10 + 0x5948 + iVar8);
  local_58[1] = *(float *)(iVar7 + 4) - *(float *)(iVar10 + 0x595c + iVar8);
  local_58[2] = (float)*(undefined4 *)(iVar7 + 8);
  if (param_3 != 0) {
    local_58[1] = local_58[1] - *(float *)(iVar10 + 0x5974 + iVar8);
  }
  if (param_2 == 0) {
    puVar4 = cfgCdtParams(*(int *)(iVar10 + 0x54));
    local_58[0] = local_58[0] + (float)puVar4[0xf];
  }
  else {
    if (param_2 != 1) goto LAB_004ef725;
    puVar4 = cfgCdtParams(*(int *)(iVar10 + 0x54));
    local_58[0] = local_58[0] - (float)puVar4[0xf];
  }
LAB_004ef725:
  if (param_4 != (float *)0x0) {
    iVar7 = 0;
    iVar10 = (param_1 + 0xf8) - (int)param_4;
    do {
      pfVar6 = local_58;
      pfVar5 = (float *)(iVar10 + 0x10 + (int)param_4);
      iVar9 = 2;
      *param_4 = *(float *)(iVar10 + (int)param_4) * local_58[0];
      do {
        pfVar6 = pfVar6 + 1;
        fVar1 = *pfVar5;
        pfVar5 = pfVar5 + 4;
        iVar9 = iVar9 + -1;
        *param_4 = fVar1 * *pfVar6 + *param_4;
      } while (iVar9 != 0);
      iVar9 = iVar7 * 4;
      iVar7 = iVar7 + 1;
      *param_4 = *(float *)(param_1 + 0x128 + iVar9) + *param_4;
      param_4 = param_4 + 1;
    } while (iVar7 < 3);
  }
  if (param_5 != (undefined4 *)0x0) {
    *param_5 = *(undefined4 *)(iVar8 + 0x5934 + *(int *)(param_1 + 0xe8));
  }
  if ((param_2 == 0) || (param_2 == 1)) {
    local_60 = *(float *)(*(int *)(param_1 + 0xe8) + 0x5c6c);
  }
  if ((*(int *)(*(int *)(param_1 + 0xe8) + 0x54) == 2) && ((param_2 == 4 || (param_2 == 5)))) {
    local_60 = *(float *)(*(int *)(param_1 + 0xe8) + 0x5c6c) * 0.33333334;
  }
  if (param_6 != (float *)0x0) {
    local_58[0] = DAT_0055e9b0;
    local_58[1] = (float)DAT_0055e9b4;
    local_58[2] = (float)DAT_0055e9b8;
    mat4RotY(local_58 + 6,local_60);
    iVar7 = 0;
    do {
      *(undefined4 *)((int)local_58 + iVar7 + 0xc) = 0;
      pfVar6 = local_58;
      fVar1 = *(float *)((int)local_58 + iVar7 + 0xc);
      pfVar5 = (float *)((int)local_58 + iVar7 + 0x18);
      iVar10 = 3;
      do {
        fVar2 = *pfVar5;
        fVar3 = *pfVar6;
        pfVar5 = pfVar5 + 4;
        pfVar6 = pfVar6 + 1;
        iVar10 = iVar10 + -1;
        fVar1 = fVar2 * fVar3 + fVar1;
      } while (iVar10 != 0);
      *(float *)((int)local_58 + iVar7 + 0xc) = fVar1;
      iVar7 = iVar7 + 4;
    } while (iVar7 < 0xc);
    local_58[0] = local_58[3];
    local_58[2] = local_58[5];
    local_58[1] = local_58[4];
    iVar10 = (param_1 + 0xf8) - (int)param_6;
    iVar7 = 3;
    do {
      pfVar6 = local_58;
      pfVar5 = (float *)(iVar10 + 0x10 + (int)param_6);
      iVar8 = 2;
      *param_6 = *(float *)(iVar10 + (int)param_6) * local_58[3];
      do {
        pfVar6 = pfVar6 + 1;
        fVar1 = *pfVar5;
        pfVar5 = pfVar5 + 4;
        iVar8 = iVar8 + -1;
        *param_6 = *pfVar6 * fVar1 + *param_6;
      } while (iVar8 != 0);
      param_6 = param_6 + 1;
      iVar7 = iVar7 + -1;
    } while (iVar7 != 0);
  }
  if (param_7 != (float *)0x0) {
    if ((param_2 == 0) || (param_2 == 2)) {
      local_60 = local_60 + 90.0;
    }
    else {
      local_60 = local_60 - 90.0;
    }
    local_58[0] = DAT_0055e9b0;
    local_58[1] = (float)DAT_0055e9b4;
    local_58[2] = (float)DAT_0055e9b8;
    mat4RotY(local_58 + 6,local_60);
    iVar7 = 0;
    do {
      *(undefined4 *)((int)local_58 + iVar7 + 0xc) = 0;
      pfVar6 = local_58;
      fVar1 = *(float *)((int)local_58 + iVar7 + 0xc);
      pfVar5 = (float *)((int)local_58 + iVar7 + 0x18);
      iVar10 = 3;
      do {
        fVar2 = *pfVar6;
        fVar3 = *pfVar5;
        pfVar5 = pfVar5 + 4;
        pfVar6 = pfVar6 + 1;
        iVar10 = iVar10 + -1;
        fVar1 = fVar2 * fVar3 + fVar1;
      } while (iVar10 != 0);
      *(float *)((int)local_58 + iVar7 + 0xc) = fVar1;
      iVar7 = iVar7 + 4;
    } while (iVar7 < 0xc);
    local_58[0] = local_58[3];
    local_58[2] = local_58[5];
    local_58[1] = local_58[4];
    iVar10 = (param_1 + 0xf8) - (int)param_7;
    iVar7 = 3;
    do {
      pfVar6 = local_58;
      pfVar5 = (float *)(iVar10 + 0x10 + (int)param_7);
      iVar8 = 2;
      *param_7 = *(float *)(iVar10 + (int)param_7) * local_58[3];
      do {
        pfVar6 = pfVar6 + 1;
        fVar1 = *pfVar5;
        pfVar5 = pfVar5 + 4;
        iVar8 = iVar8 + -1;
        *param_7 = fVar1 * *pfVar6 + *param_7;
      } while (iVar8 != 0);
      param_7 = param_7 + 1;
      iVar7 = iVar7 + -1;
    } while (iVar7 != 0);
  }
  return;
}


/* ===== FUN_004ef650 @ 004ef650  size=41  callers: FUN_004f2580 carGatherCollSpheres FUN_004ef280 FUN_004fb340 FUN_004fb340 FUN_004fb340 FUN_004fb8b0 FUN_004fb9e0 FUN_004fbc50  ===== */

void __cdecl
FUN_004ef650(int param_1,int param_2,float *param_3,undefined4 *param_4,float *param_5,float *param_6)

{
  FUN_004ef680(param_1,param_2,0,param_3,param_4,param_5,param_6);
  return;
}


/* ===== carGatherCollSpheres @ 004ef9e0  size=989  callers: FUN_004efe00 FUN_00508600 FUN_00508600 FUN_00534d00 FUN_005015c0 carSubstepCCD carSubstepCCD carSubstepCCD  ===== */

/* Collision spheres as (x,y,z,radius) quads. */

void __cdecl carGatherCollSpheres(int param_1,int *param_2,int param_3)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int *piVar4;
  undefined4 *puVar5;
  float *pfVar6;
  float *pfVar7;
  float *pfVar8;
  int iVar9;
  int iVar10;
  int local_14;
  undefined4 local_10;
  float local_c;
  float local_8 [2];
  
  piVar4 = param_2;
  iVar3 = param_1;
  iVar9 = 0;
  *param_2 = 0;
  if (0 < *(int *)(*(int *)(param_1 + 0xe8) + 0x5928)) {
    do {
      FUN_004ef650(param_1,iVar9,(float *)(*param_2 * 0x10 + param_3),&local_10,(float *)0x0,(float *)0x0);
      *(undefined4 *)(*param_2 * 0x10 + 0xc + param_3) = local_10;
      iVar9 = iVar9 + 1;
      *param_2 = *param_2 + 1;
    } while (iVar9 < *(int *)(*(int *)(param_1 + 0xe8) + 0x5928));
  }
  param_2 = (int *)0x0;
  local_14 = 0x2c;
  do {
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_c = *(float *)((int)(puVar5 + 0x12) + (int)param_2);
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_8[0] = *(float *)((int)(puVar5 + 0x13) + (int)param_2);
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_8[1] = *(float *)((int)(puVar5 + 0x14) + (int)param_2);
    if (*(int *)(*(int *)(iVar3 + 0xe8) + 0x54) == 1) {
      puVar5 = cfgCdtParams(1);
      local_8[1] = local_8[1] + *(float *)((int)(puVar5 + 0x20) + (int)param_2);
    }
    pfVar6 = (float *)(*piVar4 * 0x10 + param_3);
    param_1 = 0;
    iVar9 = (iVar3 + 0xf8) - (int)pfVar6;
    do {
      pfVar8 = local_8;
      pfVar7 = (float *)((int)pfVar6 + iVar9 + 0x10);
      iVar10 = 2;
      *pfVar6 = *(float *)((int)pfVar6 + iVar9) * local_c;
      do {
        fVar1 = *pfVar7;
        fVar2 = *pfVar8;
        pfVar7 = pfVar7 + 4;
        pfVar8 = pfVar8 + 1;
        iVar10 = iVar10 + -1;
        *pfVar6 = fVar1 * fVar2 + *pfVar6;
      } while (iVar10 != 0);
      iVar10 = param_1 * 4;
      param_1 = param_1 + 1;
      *pfVar6 = *(float *)(iVar3 + 0x128 + iVar10) + *pfVar6;
      pfVar6 = pfVar6 + 1;
    } while (param_1 < 3);
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    puVar5 = (undefined4 *)((int)puVar5 + local_14);
    local_14 = local_14 + 4;
    *(undefined4 *)(*piVar4 * 0x10 + 0xc + param_3) = *puVar5;
    param_2 = param_2 + 3;
    *piVar4 = *piVar4 + 1;
  } while (local_14 < 0x38);
  local_14 = 0x2c;
  param_2 = (int *)0x0;
  do {
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_c = *(float *)((int)(puVar5 + 0x12) + (int)param_2);
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_c = local_c + (float)puVar5[0x2b];
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_8[0] = *(float *)((int)(puVar5 + 0x13) + (int)param_2);
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_8[1] = *(float *)((int)(puVar5 + 0x14) + (int)param_2);
    pfVar6 = (float *)(*piVar4 * 0x10 + param_3);
    param_1 = 0;
    iVar9 = (iVar3 + 0xf8) - (int)pfVar6;
    do {
      pfVar8 = local_8;
      pfVar7 = (float *)((int)pfVar6 + iVar9 + 0x10);
      iVar10 = 2;
      *pfVar6 = *(float *)((int)pfVar6 + iVar9) * local_c;
      do {
        fVar1 = *pfVar8;
        fVar2 = *pfVar7;
        pfVar7 = pfVar7 + 4;
        pfVar8 = pfVar8 + 1;
        iVar10 = iVar10 + -1;
        *pfVar6 = fVar1 * fVar2 + *pfVar6;
      } while (iVar10 != 0);
      iVar10 = param_1 * 4;
      param_1 = param_1 + 1;
      *pfVar6 = *(float *)(iVar3 + 0x128 + iVar10) + *pfVar6;
      pfVar6 = pfVar6 + 1;
    } while (param_1 < 3);
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    puVar5 = (undefined4 *)((int)puVar5 + local_14);
    local_14 = local_14 + 4;
    *(undefined4 *)(*piVar4 * 0x10 + 0xc + param_3) = *puVar5;
    param_2 = param_2 + 3;
    *piVar4 = *piVar4 + 1;
  } while (local_14 < 0x38);
  local_14 = 0x2c;
  param_2 = (int *)0x0;
  do {
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_c = *(float *)((int)(puVar5 + 0x12) + (int)param_2);
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_c = local_c - (float)puVar5[0x2b];
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_8[0] = *(float *)((int)(puVar5 + 0x13) + (int)param_2);
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    local_8[1] = *(float *)((int)(puVar5 + 0x14) + (int)param_2);
    pfVar6 = (float *)(*piVar4 * 0x10 + param_3);
    param_1 = 0;
    iVar9 = (iVar3 + 0xf8) - (int)pfVar6;
    do {
      pfVar8 = local_8;
      pfVar7 = (float *)((int)pfVar6 + iVar9 + 0x10);
      iVar10 = 2;
      *pfVar6 = *(float *)((int)pfVar6 + iVar9) * local_c;
      do {
        fVar1 = *pfVar8;
        fVar2 = *pfVar7;
        pfVar7 = pfVar7 + 4;
        pfVar8 = pfVar8 + 1;
        iVar10 = iVar10 + -1;
        *pfVar6 = fVar1 * fVar2 + *pfVar6;
      } while (iVar10 != 0);
      iVar10 = param_1 * 4;
      param_1 = param_1 + 1;
      *pfVar6 = *(float *)(iVar3 + 0x128 + iVar10) + *pfVar6;
      pfVar6 = pfVar6 + 1;
    } while (param_1 < 3);
    puVar5 = cfgCdtParams(*(int *)(*(int *)(iVar3 + 0xe8) + 0x54));
    puVar5 = (undefined4 *)((int)puVar5 + local_14);
    local_14 = local_14 + 4;
    *(undefined4 *)(*piVar4 * 0x10 + 0xc + param_3) = *puVar5;
    param_2 = param_2 + 3;
    *piVar4 = *piVar4 + 1;
  } while (local_14 < 0x38);
  return;
}


/* ===== FUN_004efe00 @ 004efe00  size=998  callers: carCheckPenetration FUN_004f0230 carCheckContact  ===== */

undefined4 __cdecl
FUN_004efe00(int param_1,float param_2,float param_3,int *param_4,int param_5,int param_6,int param_7,
            int param_8,int param_9,undefined4 *param_10)

{
  int iVar1;
  undefined4 uVar2;
  float fVar3;
  float *pfVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  undefined4 *puVar8;
  int *piVar9;
  float *pfVar10;
  float local_66c;
  float local_668;
  float local_664;
  int local_660;
  int local_65c;
  undefined4 local_654;
  int *local_650;
  int local_64c;
  int local_648 [4];
  float local_638;
  float local_634;
  float local_630;
  float local_628;
  float local_624;
  float local_620;
  float local_61c;
  float local_618;
  float local_614;
  undefined4 local_5f8;
  float local_5f0 [80];
  int local_4b0 [5];
  float local_49c [295];
  
  if (param_10 != (undefined4 *)0x0) {
    puVar8 = param_10;
    for (iVar1 = 0xe4; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
  }
  if (param_9 < 0) {
    local_65c = *(int *)(*(int *)(param_1 + 0xe8) + 0x5928);
  }
  else {
    local_65c = *(int *)(*(int *)(param_1 + 0xe8) + 0x5928);
    if (param_9 <= local_65c) {
      local_65c = param_9;
    }
  }
  *param_4 = 0;
  carGatherCollSpheres(param_1,local_648,(int)local_5f0);
  uVar2 = 0;
  local_654 = 0;
  local_660 = 0;
  if (0 < local_648[0]) {
    local_650 = param_10 + 0x25;
    pfVar5 = local_5f0;
    do {
      if (param_7 == 0) {
        if (local_660 <= local_65c + -1) goto LAB_004efee6;
      }
      else if ((param_7 != 1) || (*(int *)(*(int *)(param_1 + 0xe8) + 0x5928) <= local_660)) {
LAB_004efee6:
        piVar9 = local_648;
        for (iVar1 = 0x15; piVar9 = piVar9 + 1, iVar1 != 0; iVar1 = iVar1 + -1) {
          *piVar9 = 0;
        }
        local_630 = pfVar5[3];
        local_5f8 = DAT_0148f908;
        local_61c = *pfVar5;
        local_618 = pfVar5[1];
        local_648[1] = 4;
        local_614 = pfVar5[2];
        local_628 = *pfVar5;
        local_624 = pfVar5[1];
        local_620 = pfVar5[2];
        if (param_8 == 0) {
          local_630 = local_630 + param_3;
        }
        else {
          local_634 = local_630 * 0.2;
          local_648[1] = 0x404;
          local_638 = param_3;
        }
        local_648[3] = 5;
        iVar1 = FUN_00454970((uint *)(local_648 + 1),local_4b0);
        if (iVar1 != 0) {
          local_654 = 1;
          local_64c = 0;
          if (0 < local_648[2]) {
            pfVar4 = local_49c;
            do {
              pfVar7 = (float *)(param_5 + *param_4 * 0x98);
              pfVar6 = pfVar7;
              for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
                *pfVar6 = 0.0;
                pfVar6 = pfVar6 + 1;
              }
              pfVar6 = pfVar4 + -5;
              pfVar10 = pfVar7;
              for (iVar1 = 0xf; iVar1 != 0; iVar1 = iVar1 + -1) {
                *pfVar10 = *pfVar6;
                pfVar6 = pfVar6 + 1;
                pfVar10 = pfVar10 + 1;
              }
              pfVar7[0xf] = param_2;
              pfVar7[0x10] = 0.0;
              pfVar7[0x13] = 1.4013e-45;
              if (local_660 < *(int *)(*(int *)(param_1 + 0xe8) + 0x5928)) {
                pfVar7[0x25] = 1.4013e-45;
              }
              pfVar7[0x19] = pfVar4[-2];
              pfVar7[0x1a] = pfVar4[-1];
              pfVar7[0x1b] = *pfVar4;
              local_66c = *pfVar5 - pfVar4[-2];
              local_668 = pfVar5[1] - pfVar4[-1];
              local_664 = pfVar5[2] - *pfVar4;
              fVar3 = SQRT(local_66c * local_66c + local_668 * local_668 + local_664 * local_664);
              if (fVar3 < 1e-06) {
                pfVar7[0x1c] = DAT_0055e9a0;
                pfVar7[0x1d] = DAT_0055e9a4;
                fVar3 = DAT_0055e9a8;
              }
              else {
                if (1e-06 <= ABS(fVar3 - 1.0)) {
                  fVar3 = 1.0 / fVar3;
                  local_66c = fVar3 * local_66c;
                  local_668 = local_668 * fVar3;
                  local_664 = local_664 * fVar3;
                }
                pfVar7[0x1c] = local_66c;
                pfVar7[0x1d] = local_668;
                fVar3 = local_664;
              }
              pfVar7[0x1e] = fVar3;
              fVar3 = -local_630;
              pfVar7[0x16] = fVar3 * local_66c + *pfVar5;
              pfVar7[0x17] = local_668 * fVar3 + pfVar5[1];
              pfVar7[0x18] = local_664 * fVar3 + pfVar5[2];
              if ((param_10 != (undefined4 *)0x0) && (local_660 < local_65c)) {
                pfVar6 = (float *)(local_650 + -0x25);
                for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
                  *pfVar6 = *pfVar7;
                  pfVar7 = pfVar7 + 1;
                  pfVar6 = pfVar6 + 1;
                }
                if (*local_650 == 0) {
                  *local_650 = 1;
                }
              }
              iVar1 = *param_4;
              *param_4 = iVar1 + 1;
              if (iVar1 + 1 == param_6) break;
              local_64c = local_64c + 1;
              pfVar4 = pfVar4 + 0xf;
            } while (local_64c < local_648[2]);
          }
        }
        uVar2 = local_654;
        if (*param_4 == param_6) {
          return local_654;
        }
      }
      local_660 = local_660 + 1;
      pfVar5 = pfVar5 + 4;
      local_650 = local_650 + 0x26;
    } while (local_660 < local_648[0]);
  }
  return uVar2;
}


/* ===== carCheckPenetration @ 004efdc0  size=50  callers: FUN_004fe310 FUN_004f3b80 FUN_004f3b80 carPhysTick carPhysTick carPhysTick carPhysTick carPhysTick carSubstepContact FUN_00509080  ===== */

void __cdecl
carCheckPenetration(int param_1,float param_2,float param_3,int *param_4,int param_5,int param_6,int param_7)

{
  FUN_004efe00(param_1,param_2,param_3,param_4,param_5,param_6,param_7,0,-1,(undefined4 *)0x0);
  return;
}


/* ===== carCheckContact @ 004f01f0  size=50  callers: carSubstepContact carSubstepContact  ===== */

void __cdecl
carCheckContact(int param_1,float param_2,float param_3,int *param_4,int param_5,int param_6,int param_7)

{
  FUN_004efe00(param_1,param_2,param_3,param_4,param_5,param_6,param_7,1,-1,(undefined4 *)0x0);
  return;
}


/* ===== FUN_004f0230 @ 004f0230  size=53  callers: FUN_004ef280 carPhysTick  ===== */

void __cdecl
FUN_004f0230(int param_1,float param_2,float param_3,int *param_4,int param_5,int param_6,int param_7,
            undefined4 *param_8)

{
  FUN_004efe00(param_1,param_2,param_3,param_4,param_5,param_6,0,0,param_7,param_8);
  return;
}


/* ===== FUN_00454970 @ 00454970  size=46  callers: FUN_004efe00 FUN_00508600 FUN_005015c0 FUN_00531ff0 FUN_00532d80 FUN_00473190 FUN_0050e380 FUN_004736e0 FUN_004fb8b0 FUN_00510ea0 FUN_004fbd70  ===== */

void __cdecl FUN_00454970(uint *param_1,int *param_2)

{
  int iVar1;
  int *piVar2;
  int local_3c [15];
  
  if (param_2 == (int *)0x0) {
    param_2 = local_3c;
  }
  piVar2 = param_2;
  for (iVar1 = 0xf; iVar1 != 0; iVar1 = iVar1 + -1) {
    *piVar2 = 0;
    piVar2 = piVar2 + 1;
  }
  FUN_004549a0(param_1,param_2);
  return;
}


/* ===== FUN_004549a0 @ 004549a0  size=66  callers: FUN_00454970 FUN_004542b0 FUN_004542b0 FUN_004542b0  ===== */

bool __cdecl FUN_004549a0(uint *param_1,int *param_2)

{
  uint uVar1;
  uint uVar2;
  int local_3c [15];
  
  if (param_2 == (int *)0x0) {
    param_2 = local_3c;
  }
  uVar1 = FUN_004549f0(param_1,param_2);
  if ((*param_1 & 4) == 0) {
    uVar2 = FUN_00455270((byte *)param_1,param_2);
    uVar1 = uVar1 | uVar2;
  }
  return uVar1 != 0;
}


/* ===== collSpheresMoved @ 004f5df0  size=83  callers: FUN_00508600 carSubstepCCD  ===== */

/* Compare two collision-sphere sets for excessive motion. */

undefined4 __cdecl collSpheresMoved(int param_1,float *param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar3 = 0;
  if (0 < param_1) {
    iVar2 = param_3 - (int)param_2;
    do {
      iVar1 = FUN_004557e0(DAT_0148f908,param_2,(float *)(iVar2 + (int)param_2),(int *)0x0,(int *)0x0,0,4,
                           (int *)0x0);
      if (iVar1 != 0) {
        return 1;
      }
      iVar3 = iVar3 + 1;
      param_2 = param_2 + 4;
    } while (iVar3 < param_1);
  }
  return 0;
}


/* ===== FUN_004f10d0 @ 004f10d0  size=125  callers: carSetupSprings carSetupSprings carSetupSprings carSetupSprings carSetupSprings carSetupSprings  ===== */

void __cdecl FUN_004f10d0(int param_1,int param_2,undefined4 *param_3)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  
  if (param_3 != (undefined4 *)0x0) {
    iVar2 = *(int *)(param_1 + 0xe8);
    iVar3 = *(int *)(iVar2 + 0x54);
    if (iVar3 == 0) {
      puVar1 = (undefined4 *)(iVar2 + 0x5d44 + param_2 * 0xc);
      *param_3 = *puVar1;
      param_3[1] = puVar1[1];
      param_3[2] = puVar1[2];
    }
    else {
      if (iVar3 == 1) {
        puVar1 = (undefined4 *)(iVar2 + 0xadd4 + param_2 * 0xc);
        *param_3 = *puVar1;
        param_3[1] = puVar1[1];
        param_3[2] = puVar1[2];
        return;
      }
      if (iVar3 == 2) {
        puVar1 = (undefined4 *)(iVar2 + 0xca94 + param_2 * 0xc);
        *param_3 = *puVar1;
        param_3[1] = puVar1[1];
        param_3[2] = puVar1[2];
        return;
      }
    }
  }
  return;
}


/* ===== FUN_004f1150 @ 004f1150  size=81  callers: carPostSetState FUN_00503190 FUN_00508600 FUN_00508600 FUN_004f39e0 FUN_004f3700 FUN_004f27a0  ===== */

float10 __cdecl FUN_004f1150(int param_1,undefined4 *param_2)

{
  undefined4 *puVar1;
  
  if (param_2 != (undefined4 *)0x0) {
    param_2[2] = 0;
    *param_2 = 0;
    puVar1 = cfgCdtParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
    param_2[1] = puVar1[0x2a];
  }
  puVar1 = cfgCdtParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
  return (float10)(float)puVar1[0x2a];
}


