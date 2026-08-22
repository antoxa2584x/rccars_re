/* ===== FUN_00504100 @ 00504100  size=685  callers: FUN_00504020 FUN_00504020 FUN_00504020 FUN_00504020  ===== */

undefined4 __cdecl FUN_00504100(int param_1,float *param_2,int param_3)

{
  float fVar1;
  float fVar2;
  float *pfVar3;
  float *pfVar4;
  float *pfVar5;
  int iVar6;
  float *pfVar7;
  undefined **ppuVar8;
  int iVar9;
  int iVar10;
  float *local_14;
  int local_10;
  float local_c;
  float local_8 [2];
  
  if (param_3 == 2) {
    return 1;
  }
  ppuVar8 = &PTR_s_WHEEL_FRONT_LEFT_00572fcc;
  pfVar7 = param_2 + 0x12;
  pfVar3 = pfVar7;
  do {
    fVar2 = (float)FUN_00451420(*(int *)(param_1 + 0x10),*ppuVar8);
    *pfVar3 = fVar2;
    if (fVar2 == 0.0) {
      FUN_00401220((byte *)s_CAR__can_t_find_object__s_for_pr_0057314c);
      return 0;
    }
    ppuVar8 = ppuVar8 + 1;
    pfVar3 = pfVar3 + 1;
  } while ((int)ppuVar8 < 0x572fdc);
  local_14 = param_2;
  local_10 = 4;
  do {
    FUN_0044e610((int)*pfVar7,&local_c);
    FUN_00448d50(param_1,0x40);
    iVar6 = 0;
    iVar10 = (param_1 + 0x138) - (int)local_14;
    pfVar3 = local_14;
    do {
      pfVar5 = local_8;
      pfVar4 = (float *)(iVar10 + 0x10 + (int)pfVar3);
      iVar9 = 2;
      *pfVar3 = *(float *)(iVar10 + (int)pfVar3) * local_c;
      do {
        fVar2 = *pfVar5;
        fVar1 = *pfVar4;
        pfVar4 = pfVar4 + 4;
        pfVar5 = pfVar5 + 1;
        iVar9 = iVar9 + -1;
        *pfVar3 = fVar2 * fVar1 + *pfVar3;
      } while (iVar9 != 0);
      iVar6 = iVar6 + 1;
      *pfVar3 = *(float *)(param_1 + 0x164 + iVar6 * 4) + *pfVar3;
      pfVar3 = pfVar3 + 1;
    } while (iVar6 < 3);
    pfVar7 = pfVar7 + 1;
    local_14 = local_14 + 3;
    local_10 = local_10 + -1;
  } while (local_10 != 0);
  if (param_3 == 1) {
    fVar2 = (float)FUN_00451420(*(int *)(param_1 + 0x10),PTR_s_WHEEL_FRONT_LEFT_00572fcc);
    param_2[0x78] = fVar2;
    fVar2 = (float)FUN_00451420(*(int *)(param_1 + 0x10),PTR_s_WHEEL_FRONT_RIGHT_00572fd0);
    param_2[0x79] = fVar2;
  }
  else {
    fVar2 = (float)FUN_00451420(*(int *)(param_1 + 0x10),PTR_s_AXLE_FRONT_LEFT_SUPPORT_00572fdc);
    param_2[0x78] = fVar2;
    fVar2 = (float)FUN_00451420(*(int *)(param_1 + 0x10),PTR_s_AXLE_FRONT_RIGHT_SUPPORT_00572fe0);
    param_2[0x79] = fVar2;
  }
  if (param_2[0x78] == 0.0) {
    FUN_00401220((byte *)s_CAR__can_t_find_object__s_for_pr_0057314c);
    return 0;
  }
  if (param_2[0x79] != 0.0) {
    fVar2 = (float)FUN_00451420(*(int *)(param_1 + 0x10),PTR_s_FRONT_AXLE_00572fe4);
    param_2[0xbb] = fVar2;
    fVar2 = (float)FUN_00451420(*(int *)(param_1 + 0x10),PTR_s_REAR_AXLE_00572fe8);
    param_2[0xba] = fVar2;
    if (param_2[0xbb] == 0.0) {
      FUN_00401220((byte *)s_CAR__can_t_find_object__s_for_pr_0057314c);
      return 0;
    }
    if (fVar2 != 0.0) {
      if (0 < param_3) {
        return 1;
      }
      param_2[0xbc] = 1.12104e-44;
      iVar6 = 0;
      ppuVar8 = &PTR_s_SPRING_REAR_1_00572fec;
      pfVar7 = param_2 + 0xce;
      do {
        fVar2 = (float)FUN_00451420(*(int *)(param_1 + 0x10),*ppuVar8);
        *pfVar7 = fVar2;
        if (fVar2 == 0.0) {
          FUN_00401220((byte *)s_CAR__can_t_find_object__s_for_pr_0057314c);
          return 0;
        }
        iVar6 = iVar6 + 1;
        ppuVar8 = ppuVar8 + 1;
        pfVar7 = pfVar7 + 0x16;
      } while (iVar6 < (int)param_2[0xbc]);
      return 1;
    }
  }
  FUN_00401220((byte *)s_CAR__can_t_find_object__s_for_pr_0057314c);
  return 0;
}


/* ===== FUN_00505150 @ 00505150  size=638  callers: FUN_00505070 FUN_00505070 FUN_00505070 FUN_00505070  ===== */

undefined4 __cdecl FUN_00505150(int param_1,float *param_2,int param_3)

{
  float fVar1;
  undefined *puVar2;
  float fVar3;
  float *pfVar4;
  float *pfVar5;
  float *pfVar6;
  int iVar7;
  undefined **ppuVar8;
  int iVar9;
  int iVar10;
  float *local_18;
  float *local_14;
  int local_10;
  float local_c;
  float local_8 [2];
  
  if (param_3 != 2) {
    ppuVar8 = &PTR_s_WHEEL_FRONT_LEFT_005731b8;
    local_18 = param_2 + 0x12;
    pfVar4 = local_18;
    do {
      fVar3 = (float)FUN_00451420(*(int *)(param_1 + 0x10),*ppuVar8);
      *pfVar4 = fVar3;
      if (fVar3 == 0.0) goto LAB_005053a9;
      ppuVar8 = ppuVar8 + 1;
      pfVar4 = pfVar4 + 1;
    } while ((int)ppuVar8 < 0x5731c8);
    local_14 = param_2;
    local_10 = 4;
    do {
      FUN_0044e610((int)*local_18,&local_c);
      FUN_00448d50(param_1,0x40);
      iVar7 = 0;
      iVar10 = (param_1 + 0x138) - (int)local_14;
      pfVar4 = local_14;
      do {
        pfVar6 = local_8;
        pfVar5 = (float *)(iVar10 + 0x10 + (int)pfVar4);
        iVar9 = 2;
        *pfVar4 = *(float *)(iVar10 + (int)pfVar4) * local_c;
        do {
          fVar3 = *pfVar5;
          fVar1 = *pfVar6;
          pfVar5 = pfVar5 + 4;
          pfVar6 = pfVar6 + 1;
          iVar9 = iVar9 + -1;
          *pfVar4 = fVar3 * fVar1 + *pfVar4;
        } while (iVar9 != 0);
        iVar9 = iVar7 * 4;
        iVar7 = iVar7 + 1;
        *pfVar4 = *(float *)(param_1 + 0x168 + iVar9) + *pfVar4;
        pfVar4 = pfVar4 + 1;
      } while (iVar7 < 3);
      local_18 = local_18 + 1;
      local_14 = local_14 + 3;
      local_10 = local_10 + -1;
    } while (local_10 != 0);
    local_18 = (float *)0x0;
    ppuVar8 = &PTR_s_AXLE_FRONT_LEFT_UP_005731c8;
    pfVar4 = param_2 + 0x89;
    do {
      fVar3 = (float)FUN_00451420(*(int *)(param_1 + 0x10),*ppuVar8);
      pfVar4[-0x11] = fVar3;
      if (fVar3 == 0.0) {
LAB_005053a9:
        FUN_00401220((byte *)s_CAR__object__s_for_proc_ani_not_f_005733a8);
        return 0;
      }
      fVar3 = (float)FUN_00451420(*(int *)(param_1 + 0x10),ppuVar8[1]);
      *pfVar4 = fVar3;
      if (fVar3 == 0.0) {
        FUN_00401220((byte *)s_CAR__object__s_for_proc_ani_not_f_005733a8);
        return 0;
      }
      fVar3 = (float)FUN_00451420(*(int *)(param_1 + 0x10),ppuVar8[2]);
      pfVar4[0x11] = fVar3;
      if (fVar3 == 0.0) {
        FUN_00401220((byte *)s_CAR__object__s_for_proc_ani_not_f_005733a8);
        return 0;
      }
      ppuVar8 = ppuVar8 + 3;
      local_18 = (float *)((int)local_18 + 1);
      pfVar4 = pfVar4 + 0x33;
    } while ((int)local_18 < 4);
    if (param_3 < 1) {
      iVar7 = 0;
      param_2 = param_2 + 0x144;
      local_18 = (float *)0x0;
      do {
        iVar10 = 0;
        ppuVar8 = &PTR_s_Spring1_1_005731f8 + iVar7;
        pfVar4 = param_2;
        do {
          puVar2 = *ppuVar8;
          iVar7 = iVar7 + 1;
          ppuVar8 = ppuVar8 + 1;
          fVar3 = (float)FUN_00451420(*(int *)(param_1 + 0x10),puVar2);
          *pfVar4 = fVar3;
          if (fVar3 == 0.0) goto LAB_005053a9;
          iVar10 = iVar10 + 1;
          pfVar4 = pfVar4 + 1;
        } while (iVar10 < 2);
        local_18 = (float *)((int)local_18 + 1);
        param_2 = param_2 + 0x22;
        if (3 < (int)local_18) {
          return 1;
        }
      } while( true );
    }
  }
  return 1;
}


/* ===== FUN_00504820 @ 00504820  size=2090  callers: FUN_00504780 FUN_00504780 FUN_00505050  ===== */

void __cdecl FUN_00504820(int param_1,int param_2,int param_3,int param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined4 *puVar5;
  int *piVar6;
  float *pfVar7;
  int iVar8;
  int iVar9;
  float *pfVar10;
  float *pfVar11;
  int iVar12;
  int *piVar13;
  undefined4 *puVar14;
  int iVar15;
  int *piVar16;
  float10 fVar17;
  float10 fVar18;
  int local_d0;
  undefined4 *local_cc;
  float local_c8;
  int *local_c4;
  float local_bc [4];
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0 [4];
  float local_90;
  float local_8c [19];
  float local_40 [16];
  
  if (param_3 != 2) {
    FUN_00448d50(param_1,4);
    puVar5 = (undefined4 *)(param_4 + 0x1e8);
    puVar14 = (undefined4 *)(*(int *)(param_4 + 0x1e0) + 0xac);
    for (iVar8 = 0x10; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar14 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar14 = puVar14 + 1;
    }
    puVar5 = (undefined4 *)(param_4 + 0x228);
    puVar14 = (undefined4 *)(*(int *)(param_4 + 0x1e4) + 0xac);
    for (iVar8 = 0x10; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar14 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar14 = puVar14 + 1;
    }
    FUN_0040cab0((float *)(*(int *)(param_4 + 0x1e0) + 0xac),*(float *)(*(int *)(param_2 + 0xe8) + 0x5c6c),1);
    FUN_0040cab0((float *)(*(int *)(param_4 + 0x1e4) + 0xac),*(float *)(*(int *)(param_2 + 0xe8) + 0x5c6c),1);
    local_c8 = 0.0;
    if (0 < *(int *)(*(int *)(param_2 + 0xe8) + 0x5928)) {
      local_cc = (undefined4 *)(param_4 + 0x60);
      local_c4 = (int *)(param_4 + 0x48);
      local_d0 = 0;
      do {
        iVar8 = *local_c4;
        if ((param_3 != 1) || (0x68 < local_d0)) {
          puVar5 = local_cc;
          puVar14 = (undefined4 *)(iVar8 + 0xac);
          for (iVar9 = 0x10; iVar9 != 0; iVar9 = iVar9 + -1) {
            *puVar14 = *puVar5;
            puVar5 = puVar5 + 1;
            puVar14 = puVar14 + 1;
          }
        }
        FUN_0040cb30((float *)(iVar8 + 0xac),
                     *(float *)(*(int *)(param_2 + 0xe8) + 0x5938 + local_d0) * 57.295776,1);
        local_d0 = local_d0 + 0x68;
        local_c8 = (float)((int)local_c8 + 1);
        local_c4 = local_c4 + 1;
        local_cc = local_cc + 0x10;
      } while ((int)local_c8 < *(int *)(*(int *)(param_2 + 0xe8) + 0x5928));
    }
    iVar8 = *(int *)(param_4 + 0x2e8);
    FUN_004ef9b0(param_2,2,&local_a4,(undefined4 *)0x0,(float *)0x0,(float *)0x0);
    FUN_00448d50(param_2,0x40);
    iVar9 = 0;
    iVar15 = (param_2 + 0x138) - (int)local_bc;
    pfVar7 = local_bc;
    do {
      pfVar11 = local_a0;
      pfVar10 = (float *)(iVar15 + 0x10 + (int)pfVar7);
      iVar12 = 2;
      *pfVar7 = *(float *)(iVar15 + (int)pfVar7) * local_a4;
      fVar1 = *pfVar7;
      do {
        fVar2 = *pfVar10;
        fVar3 = *pfVar11;
        pfVar10 = pfVar10 + 4;
        pfVar11 = pfVar11 + 1;
        iVar12 = iVar12 + -1;
        fVar1 = fVar2 * fVar3 + fVar1;
      } while (iVar12 != 0);
      *pfVar7 = fVar1;
      iVar12 = iVar9 * 4;
      iVar9 = iVar9 + 1;
      *pfVar7 = *(float *)(param_2 + 0x168 + iVar12) + *pfVar7;
      fVar1 = local_bc[1];
      pfVar7 = pfVar7 + 1;
    } while (iVar9 < 3);
    FUN_004ef9b0(param_2,3,&local_a4,(undefined4 *)0x0,(float *)0x0,(float *)0x0);
    FUN_00448d50(param_2,0x40);
    iVar9 = 0;
    iVar15 = (param_2 + 0x138) - (int)local_bc;
    pfVar7 = local_bc;
    do {
      pfVar11 = local_a0;
      pfVar10 = (float *)(iVar15 + 0x10 + (int)pfVar7);
      iVar12 = 2;
      *pfVar7 = *(float *)(iVar15 + (int)pfVar7) * local_a4;
      fVar2 = *pfVar7;
      do {
        fVar3 = *pfVar10;
        fVar4 = *pfVar11;
        pfVar10 = pfVar10 + 4;
        pfVar11 = pfVar11 + 1;
        iVar12 = iVar12 + -1;
        fVar2 = fVar3 * fVar4 + fVar2;
      } while (iVar12 != 0);
      *pfVar7 = fVar2;
      iVar12 = iVar9 * 4;
      iVar9 = iVar9 + 1;
      *pfVar7 = *(float *)(param_2 + 0x168 + iVar12) + *pfVar7;
      pfVar7 = pfVar7 + 1;
    } while (iVar9 < 3);
    fVar17 = (float10)fpatan(((float10)fVar1 - (float10)local_bc[1]) * (float10)4.0,(float10)1);
    fVar1 = local_bc[1] + fVar1;
    fVar2 = *(float *)(*(int *)(param_2 + 0xe8) + 0x5d48);
    puVar5 = cfgSpringParams(*(int *)(*(int *)(param_2 + 0xe8) + 0x54));
    fVar18 = (float10)fpatan((float10)(fVar1 * 0.5 - fVar2) / (float10)(float)puVar5[0xb],(float10)1);
    local_c8 = (float)-(fVar18 * (float10)57.295776);
    if ((float10)-90.0 <= -(fVar18 * (float10)57.295776)) {
      if (30.0 < local_c8) {
        local_c8 = 30.0;
      }
    }
    else {
      local_c8 = -90.0;
    }
    pfVar7 = (float *)(iVar8 + 0xac);
    pfVar10 = (float *)(param_4 + 0x268);
    pfVar11 = pfVar7;
    for (iVar8 = 0x10; iVar8 != 0; iVar8 = iVar8 + -1) {
      *pfVar11 = *pfVar10;
      pfVar10 = pfVar10 + 1;
      pfVar11 = pfVar11 + 1;
    }
    FUN_0040cb30(pfVar7,local_c8,1);
    FUN_0040ca30(pfVar7,-(float)-(fVar17 * (float10)57.295776),1);
    iVar8 = *(int *)(param_4 + 0x2ec);
    FUN_004ef9b0(param_2,0,&local_a4,(undefined4 *)0x0,(float *)0x0,(float *)0x0);
    FUN_00448d50(param_2,0x40);
    iVar9 = 0;
    iVar15 = (param_2 + 0x138) - (int)local_bc;
    pfVar7 = local_bc;
    do {
      pfVar11 = local_a0;
      pfVar10 = (float *)(iVar15 + 0x10 + (int)pfVar7);
      iVar12 = 2;
      *pfVar7 = *(float *)(iVar15 + (int)pfVar7) * local_a4;
      fVar1 = *pfVar7;
      do {
        fVar2 = *pfVar10;
        fVar3 = *pfVar11;
        pfVar10 = pfVar10 + 4;
        pfVar11 = pfVar11 + 1;
        iVar12 = iVar12 + -1;
        fVar1 = fVar2 * fVar3 + fVar1;
      } while (iVar12 != 0);
      *pfVar7 = fVar1;
      iVar12 = iVar9 * 4;
      iVar9 = iVar9 + 1;
      *pfVar7 = *(float *)(param_2 + 0x168 + iVar12) + *pfVar7;
      fVar1 = local_bc[1];
      pfVar7 = pfVar7 + 1;
    } while (iVar9 < 3);
    FUN_004ef9b0(param_2,1,&local_a4,(undefined4 *)0x0,(float *)0x0,(float *)0x0);
    FUN_00448d50(param_2,0x40);
    iVar9 = 0;
    iVar15 = (param_2 + 0x138) - (int)local_bc;
    pfVar7 = local_bc;
    do {
      pfVar11 = local_a0;
      pfVar10 = (float *)(iVar15 + 0x10 + (int)pfVar7);
      iVar12 = 2;
      *pfVar7 = *(float *)(iVar15 + (int)pfVar7) * local_a4;
      fVar2 = *pfVar7;
      do {
        fVar3 = *pfVar10;
        fVar4 = *pfVar11;
        pfVar10 = pfVar10 + 4;
        pfVar11 = pfVar11 + 1;
        iVar12 = iVar12 + -1;
        fVar2 = fVar3 * fVar4 + fVar2;
      } while (iVar12 != 0);
      *pfVar7 = fVar2;
      iVar12 = iVar9 * 4;
      iVar9 = iVar9 + 1;
      *pfVar7 = *(float *)(param_2 + 0x168 + iVar12) + *pfVar7;
      pfVar7 = pfVar7 + 1;
    } while (iVar9 < 3);
    fVar18 = (float10)fpatan(((float10)fVar1 - (float10)local_bc[1]) * (float10)4.0,(float10)1);
    fVar1 = fVar1 + local_bc[1];
    fVar2 = *(float *)(*(int *)(param_2 + 0xe8) + 0x5d48);
    puVar5 = cfgSpringParams(*(int *)(*(int *)(param_2 + 0xe8) + 0x54));
    fVar17 = (float10)fpatan((float10)(fVar1 * 0.5 - fVar2) / (float10)(float)puVar5[0xb],(float10)1);
    fVar17 = fVar17 * (float10)57.295776;
    if ((float10)-30.0 <= fVar17) {
      if ((float10)90.0 < fVar17) {
        fVar17 = (float10)90.0;
      }
    }
    else {
      fVar17 = (float10)-30.0;
    }
    pfVar7 = (float *)(iVar8 + 0xac);
    pfVar10 = (float *)(param_4 + 0x2a8);
    pfVar11 = pfVar7;
    for (iVar8 = 0x10; iVar8 != 0; iVar8 = iVar8 + -1) {
      *pfVar11 = *pfVar10;
      pfVar10 = pfVar10 + 1;
      pfVar11 = pfVar11 + 1;
    }
    FUN_0040cb30(pfVar7,(float)-fVar17,1);
    FUN_0040ca30(pfVar7,-(float)(fVar18 * (float10)57.295776),1);
    if (0 < param_3) {
      FUN_00448cb0(param_1,4);
      return;
    }
    iVar8 = 0;
    if (0 < *(int *)(param_4 + 0x2f0)) {
      piVar6 = (int *)(param_4 + 0x338);
      do {
        piVar13 = piVar6 + -0x10;
        piVar16 = (int *)(*piVar6 + 0xac);
        for (iVar9 = 0x10; iVar9 != 0; iVar9 = iVar9 + -1) {
          *piVar16 = *piVar13;
          piVar13 = piVar13 + 1;
          piVar16 = piVar16 + 1;
        }
        iVar8 = iVar8 + 1;
        piVar6 = piVar6 + 0x16;
      } while (iVar8 < *(int *)(param_4 + 0x2f0));
    }
    FUN_00448cb0(param_1,4);
    FUN_00448d50(param_1,4);
    local_c8 = 0.0;
    if (0 < *(int *)(param_4 + 0x2f0)) {
      piVar6 = (int *)(param_4 + 0x348);
      do {
        iVar8 = piVar6[-4];
        mat4GetRow3(iVar8 + 0x6c,local_a0 + 2);
        mat4GetRow2(iVar8 + 0x6c,local_bc);
        fVar1 = SQRT(local_bc[0] * local_bc[0] + local_bc[1] * local_bc[1] + local_bc[2] * local_bc[2]);
        if ((1e-06 <= fVar1) && (1e-06 <= ABS(fVar1 - 1.0))) {
          fVar1 = 1.0 / fVar1;
          local_bc[0] = local_bc[0] * fVar1;
          local_bc[1] = local_bc[1] * fVar1;
          local_bc[2] = local_bc[2] * fVar1;
        }
        iVar9 = *piVar6;
        local_d0 = 0;
        iVar15 = (iVar9 + 0x6c) - (int)local_8c;
        pfVar7 = local_8c;
        do {
          pfVar10 = (float *)(piVar6 + -2);
          pfVar11 = (float *)(iVar15 + 0x10 + (int)pfVar7);
          iVar12 = 2;
          *pfVar7 = *(float *)(iVar15 + (int)pfVar7) * (float)piVar6[-3];
          fVar1 = *pfVar7;
          do {
            fVar2 = *pfVar10;
            fVar3 = *pfVar11;
            pfVar11 = pfVar11 + 4;
            pfVar10 = pfVar10 + 1;
            iVar12 = iVar12 + -1;
            fVar1 = fVar2 * fVar3 + fVar1;
          } while (iVar12 != 0);
          *pfVar7 = fVar1;
          iVar12 = local_d0 * 4;
          local_d0 = local_d0 + 1;
          *pfVar7 = *(float *)(iVar9 + 0x9c + iVar12) + *pfVar7;
          pfVar7 = pfVar7 + 1;
        } while (local_d0 < 3);
        local_bc[3] = local_8c[0] - local_a0[2];
        local_ac = local_8c[1] - local_a0[3];
        local_a8 = local_8c[2] - local_90;
        local_cc = (undefined4 *)SQRT(local_ac * local_ac + local_a8 * local_a8 + local_bc[3] * local_bc[3]);
        fVar1 = SQRT(local_ac * local_ac + local_a8 * local_a8 + local_bc[3] * local_bc[3]);
        if (fVar1 < 1e-06) {
          local_bc[3] = local_bc[0];
          local_ac = local_bc[1];
          local_a8 = local_bc[2];
        }
        else if (1e-06 <= ABS(fVar1 - 1.0)) {
          fVar1 = 1.0 / fVar1;
          local_bc[3] = fVar1 * local_bc[3];
          local_ac = local_ac * fVar1;
          local_a8 = local_a8 * fVar1;
        }
        fVar1 = (float)piVar6[-0x15];
        if ((float)local_cc < 1e-06) {
          local_cc = (undefined4 *)fVar1;
        }
        FUN_00407620(local_bc,local_bc + 3,local_40);
        mat4Mul(iVar8 + 0x6c,(int)local_40,local_8c + 3);
        FUN_0040c880(local_8c + 3,local_a0[2],local_a0[3],local_90,0);
        FUN_00503cb0(iVar8,(int)(local_8c + 3));
        FUN_0040c9a0((float *)(iVar8 + 0xac),1.0,1.0,(float)local_cc / fVar1,1);
        local_c8 = (float)((int)local_c8 + 1);
        piVar6 = piVar6 + 0x16;
      } while ((int)local_c8 < *(int *)(param_4 + 0x2f0));
    }
    FUN_00448cb0(param_1,4);
  }
  return;
}


/* ===== FUN_00504470 @ 00504470  size=776  callers: FUN_005043b0 FUN_005043b0 FUN_005043b0 FUN_005043b0  ===== */

/* WARNING: Removing unreachable block (ram,0x005046a3) */

undefined4 __cdecl FUN_00504470(int param_1,int param_2)

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
  undefined1 auStackY_36c [716];
  undefined4 uStackY_a0;
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
  float local_40 [12];
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
        local_74 = auStackY_36c + -param_1;
        do {
          if (local_78 < 4) {
            *piVar4 = *(int *)(param_1 + 0x2e8);
          }
          else {
            *piVar4 = *(int *)(param_1 + 0x2ec);
          }
          iVar11 = piVar4[-4];
          mat4GetRow3(iVar11 + 0x6c,local_54 + 2);
          uStackY_a0 = 0x5045bc;
          mat4GetRow2(iVar11 + 0x6c,&local_64);
          fVar1 = SQRT(local_64 * local_64 + local_60 * local_60 + local_5c * local_5c);
          if ((1e-06 <= fVar1) && (1e-06 <= ABS(fVar1 - 1.0))) {
            fVar1 = 1.0 / fVar1;
            local_64 = local_64 * fVar1;
            local_60 = local_60 * fVar1;
            local_5c = local_5c * fVar1;
          }
          uStackY_a0 = 0x504647;
          cfgGetInt(3,s_WheelsVisual_00573198,s_SpringLength_005731a8,&local_70);
          iVar11 = 0;
          local_6c = local_70;
          uStack_68 = 0;
          fVar1 = (float)local_70;
          if (1.0 <= fVar1) {
            if (fVar1 <= 100.0) {
              fVar1 = (fVar1 - 1.0) * 0.0009090909 + 0.01;
            }
            else {
              fVar1 = 0.1;
            }
          }
          else {
            fVar1 = 0.01;
          }
          piVar4[-0x15] = (int)fVar1;
          local_58 = fVar1 * local_64 + local_54[2];
          local_54[0] = local_60 * fVar1 + local_54[3];
          local_54[1] = local_5c * fVar1 + local_44;
          FUN_0040b1f0(*piVar4 + 0x6c,local_40);
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


/* ===== FUN_005043b0 @ 005043b0  size=186  callers: FUN_00503ec0  ===== */

undefined4 __cdecl FUN_005043b0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_00504470(*(int *)(param_1 + 0xe8) + 0x5d44,-1);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR___carInitAniProc1_return_FAL_00573174);
    return 0;
  }
  iVar1 = FUN_00504470(*(int *)(param_1 + 0xe8) + 0x7168,0);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR___carInitAniProc1_return_FAL_00573174);
    return 0;
  }
  iVar1 = FUN_00504470(*(int *)(param_1 + 0xe8) + 0x858c,1);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR___carInitAniProc1_return_FAL_00573174);
    return 0;
  }
  iVar1 = FUN_00504470(*(int *)(param_1 + 0xe8) + 0x99b0,2);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR___carInitAniProc1_return_FAL_00573174);
    return 0;
  }
  return 1;
}


/* ===== FUN_00504020 @ 00504020  size=209  callers: FUN_00503cf0  ===== */

undefined4 __cdecl FUN_00504020(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_00504100(param_1,(float *)(*(int *)(param_1 + 0xe8) + 0x5d44),-1);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR___carCreateAniProc1_return_F_00573124);
    return 0;
  }
  iVar1 = FUN_00504100(*(int *)(*(int *)(param_1 + 0xe8) + 0xe718),
                       (float *)(*(int *)(param_1 + 0xe8) + 0x7168),0);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR___carCreateAniProc1_return_F_00573124);
    return 0;
  }
  iVar1 = FUN_00504100(*(int *)(*(int *)(param_1 + 0xe8) + 0xe71c),
                       (float *)(*(int *)(param_1 + 0xe8) + 0x858c),1);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR___carCreateAniProc1_return_F_00573124);
    return 0;
  }
  iVar1 = FUN_00504100(*(int *)(*(int *)(param_1 + 0xe8) + 0xe720),
                       (float *)(*(int *)(param_1 + 0xe8) + 0x99b0),2);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR___carCreateAniProc1_return_F_00573124);
    return 0;
  }
  return 1;
}


/* ===== FUN_00503190 @ 00503190  size=675  callers: FUN_00503880  ===== */

/* WARNING: Removing unreachable block (ram,0x0050338d) */

undefined4 __cdecl FUN_00503190(int param_1,int param_2,float *param_3)

{
  int iVar1;
  undefined4 *puVar2;
  float *pfVar3;
  undefined4 *puVar4;
  float *pfVar5;
  float local_184;
  undefined4 local_180;
  float local_17c;
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
  float local_14c [3];
  float local_140 [16];
  float local_100 [16];
  undefined4 local_c0 [16];
  float local_80 [16];
  float local_40 [16];
  
  local_180 = 0;
  if ((*(float *)(*(int *)(param_1 + 0xe8) + 0x443c) <= 1e-06) ||
     (*(int *)(*(int *)(param_1 + 0xe8) + 0x50) != 1)) {
    pfVar3 = local_140;
    for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
      *pfVar3 = *param_3;
      param_3 = param_3 + 1;
      pfVar3 = pfVar3 + 1;
    }
  }
  else {
    mat4GetRow3(param_2,&local_17c);
    mat4GetRow3((int)param_3,&local_170);
    local_164 = local_170 - local_17c;
    local_160 = local_16c - local_178;
    local_15c = local_168 - local_174;
    FUN_0040b1f0(param_2,local_80);
    mat4Mul((int)local_80,(int)param_3,local_40);
    puVar2 = (undefined4 *)(*(int *)(param_1 + 0xe8) + 0x445c);
    puVar4 = local_c0;
    for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar4 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar4 = puVar4 + 1;
    }
    mat4GetRow3((int)local_c0,&local_158);
    mat4Mul((int)local_c0,(int)local_40,local_100);
    FUN_0040c880(local_100,local_164 + local_158,local_160 + local_154,local_15c + local_150,0);
    mat4GetRow3((int)local_100,&local_17c);
    mat4GetRow3((int)param_3,&local_170);
    if (SQRT((local_17c - local_170) * (local_17c - local_170) +
             (local_178 - local_16c) * (local_178 - local_16c) +
             (local_174 - local_168) * (local_174 - local_168)) <= 2.0) {
      local_184 = *(float *)(*(int *)(param_1 + 0xe8) + 0x443c);
      if (0.0 <= local_184) {
        if (local_184 <= 1.0) {
          local_184 = 1.0 - local_184;
        }
        else {
          local_184 = 0.0;
        }
      }
      else {
        local_184 = 1.0;
      }
      FUN_00408240(local_100,param_3,local_184,local_140);
    }
    else {
      local_180 = 1;
      pfVar3 = local_140;
      for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
        *pfVar3 = *param_3;
        param_3 = param_3 + 1;
        pfVar3 = pfVar3 + 1;
      }
    }
  }
  pfVar3 = local_140;
  pfVar5 = (float *)(*(int *)(param_1 + 0xe8) + 0x445c);
  for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
    *pfVar5 = *pfVar3;
    pfVar3 = pfVar3 + 1;
    pfVar5 = pfVar5 + 1;
  }
  FUN_0044c8e0(param_1,local_140,0);
  FUN_004f1150(param_1,local_14c);
  FUN_00474f70(*(int *)(param_1 + 0xe8) + 0x57f4,(float *)(param_1 + 0xf8),local_14c);
  return local_180;
}


/* ===== FUN_00503440 @ 00503440  size=929  callers: FUN_00503880  ===== */

undefined4 __cdecl FUN_00503440(int param_1,int *param_2,float param_3,float param_4)

{
  float *pfVar1;
  float fVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  int iVar7;
  float10 fVar8;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  if (0.0 <= param_3) {
    if (*param_2 == 0) {
      *param_2 = 1;
      param_2[1] = *(int *)param_2[6];
    }
    if (param_3 < 0.1) {
      param_3 = 0.1;
    }
    fVar5 = param_3 * param_4;
    FUN_00501cf0((int)param_2,*param_2 + -1,&local_18,(undefined4 *)0x0);
    FUN_00501cf0((int)param_2,*param_2,&local_c,(undefined4 *)0x0);
    iVar3 = *param_2;
    fVar2 = (float)param_2[1];
    pfVar1 = (float *)(param_2[6] + iVar3 * 0x88);
    fVar4 = pfVar1[-0x22];
    if (fVar4 <= fVar2) {
      if (fVar2 < *pfVar1) {
        fVar6 = *pfVar1 - fVar4;
        if (1e-06 <= ABS(fVar6)) {
          fVar2 = 1.0 - (fVar2 - fVar4) / fVar6;
        }
        else {
          fVar2 = 0.0;
        }
      }
      else {
        fVar2 = 0.0;
      }
    }
    else {
      fVar2 = 1.0;
    }
    fVar2 = fVar2 * SQRT((local_18 - local_c) * (local_18 - local_c) +
                         (local_14 - local_8) * (local_14 - local_8) +
                         (local_10 - local_4) * (local_10 - local_4));
    if (fVar2 < fVar5) {
      param_3 = fVar5 - fVar2;
      param_2[1] = (int)*pfVar1;
      if (param_2[5] + -1 <= iVar3) {
        param_2[1] = (int)*pfVar1;
        return 1;
      }
      while( true ) {
        *param_2 = iVar3 + 1;
        FUN_00501cf0((int)param_2,iVar3,&local_18,(undefined4 *)0x0);
        FUN_00501cf0((int)param_2,*param_2,&local_c,(undefined4 *)0x0);
        fVar2 = SQRT((local_18 - local_c) * (local_18 - local_c) +
                     (local_14 - local_8) * (local_14 - local_8) + (local_10 - local_4) * (local_10 - local_4)
                    );
        iVar3 = *param_2;
        if (param_3 < fVar2) break;
        if (param_2[5] + -1 <= iVar3) {
          param_2[1] = *(int *)(param_2[6] + *param_2 * 0x88);
          return 1;
        }
        param_3 = param_3 - fVar2;
        param_2[1] = *(int *)(param_2[6] + iVar3 * 0x88);
      }
      if (param_3 < 0.0) {
        param_2[1] = (int)((float)param_2[1] + 0.0);
        return 0;
      }
      if (1e-06 <= ABS(fVar2)) {
        param_2[1] = (int)(((*(float *)(param_2[6] + iVar3 * 0x88) -
                            *(float *)(param_2[6] + -0x88 + iVar3 * 0x88)) * param_3) / fVar2 +
                          (float)param_2[1]);
        return 0;
      }
      param_2[1] = (int)((float)param_2[1] + 0.0);
      return 0;
    }
    if (fVar5 < 0.0) {
      param_2[1] = (int)((float)param_2[1] + 0.0);
      return 0;
    }
    if (ABS(fVar2) < 1e-06) {
      param_2[1] = (int)((float)param_2[1] + 0.0);
      return 0;
    }
    param_2[1] = (int)(((*pfVar1 - (float)param_2[1]) * fVar5) / fVar2 + (float)param_2[1]);
  }
  else {
    fVar8 = FUN_004fd4c0(param_1);
    fVar2 = (float)(fVar8 * (float10)param_4 + (float10)(float)param_2[1]);
    param_2[1] = (int)fVar2;
    iVar3 = param_2[6];
    if (*(float *)(iVar3 + *param_2 * 0x88) < fVar2) {
      do {
        if (param_2[5] + -1 <= *param_2) {
          param_2[1] = *(int *)(iVar3 + *param_2 * 0x88);
          return 1;
        }
        iVar7 = *param_2 + 1;
        *param_2 = iVar7;
      } while (*(float *)(iVar3 + iVar7 * 0x88) < fVar2);
      return 0;
    }
  }
  return 0;
}


/* ===== FUN_004f3800 @ 004f3800  size=477  callers: FUN_00503880 FUN_004f6ea0  ===== */

void __cdecl FUN_004f3800(int param_1,int param_2,int param_3,float param_4)

{
  int iVar1;
  bool bVar2;
  float *pfVar3;
  undefined4 *puVar4;
  float10 fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  
  fVar5 = FUN_0050b7f0(param_1);
  fVar7 = (float)fVar5;
  if (*(int *)(*(int *)(param_2 + 0x5d3c) + 4) != 1) {
    return;
  }
  fVar8 = param_4 + *(float *)(param_2 + 0x5734);
  *(float *)(param_2 + 0x5734) = fVar8;
  if (param_3 == 0) {
    *(undefined4 *)(param_2 + 0x5728) = 0;
  }
  bVar2 = true;
  if (param_3 == 0) {
    *(undefined4 *)(param_2 + 0x572c) = 0;
    if (*(int *)(param_2 + 0x5728) != 0) {
      *(undefined4 *)(param_2 + 0x5728) = 0;
      *(undefined4 *)(param_2 + 0x5734) = 0;
    }
  }
  else if (0.9 < fVar8) {
    if (*(int *)(param_2 + 0x572c) == 0) {
      if (*(float *)(param_2 + 0x5740) < 0.01) {
        *(undefined4 *)(param_2 + 0x572c) = 1;
        *(undefined4 *)(param_2 + 0x5734) = 0;
        *(undefined4 *)(param_2 + 0x5728) = 1;
        goto LAB_004f38d7;
      }
      bVar2 = false;
    }
    else if (10.000001 < *(float *)(param_2 + 0x5740)) {
      bVar2 = false;
      *(undefined4 *)(param_2 + 0x572c) = 0;
      *(undefined4 *)(param_2 + 0x5728) = 1;
      goto LAB_004f38d7;
    }
    *(undefined4 *)(param_2 + 0x5728) = 1;
  }
LAB_004f38d7:
  pfVar3 = (float *)cfgEngineParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
  fVar8 = *pfVar3;
  iVar1 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgEngineParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
  fVar8 = fVar8 * (float)puVar4[*(int *)(*(int *)(iVar1 + 0x54) * 0xb0 + 0xe458 + iVar1) + 7] * 0.95;
  if (fVar8 <= -1e-06) {
    fVar6 = *(float *)(param_2 + 0x5740);
    fVar8 = ABS(fVar8);
    fVar7 = 0.0;
  }
  else {
    fVar6 = *(float *)(param_2 + 0x5740);
  }
  fVar5 = moveTowards(fVar6,fVar7,fVar8,param_4);
  *(float *)(param_2 + 0x5740) = (float)fVar5;
  if (!bVar2) {
    puVar4 = cfgEngineParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
    fVar7 = (float)puVar4[1];
    *(float *)(param_2 + 0x5738) = param_4 + *(float *)(param_2 + 0x5738);
    fVar5 = moveTowards(*(float *)(param_2 + 0x5740),0.0,fVar7,param_4);
    *(float *)(param_2 + 0x5740) = (float)fVar5;
    *(undefined4 *)(param_2 + 0x573c) = 1;
    return;
  }
  *(undefined4 *)(param_2 + 0x573c) = 0;
  *(undefined4 *)(param_2 + 0x5738) = 0;
  return;
}


/* ===== FUN_00503880 @ 00503880  size=1064  callers: FUN_004f69e0 FUN_004f72f0 FUN_004f72f0 FUN_004f27a0  ===== */

void __cdecl FUN_00503880(int param_1,undefined4 param_2,float param_3)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  int iVar8;
  float10 fVar9;
  float10 fVar10;
  float local_8c;
  float local_80 [16];
  float local_40 [16];
  
  *(float *)(*(int *)(param_1 + 0xe8) + 0x443c) = *(float *)(*(int *)(param_1 + 0xe8) + 0x443c) - param_3;
  iVar8 = *(int *)(param_1 + 0xe8);
  piVar1 = (int *)(iVar8 + 0x3fb8 + *(int *)(iVar8 + 0x6dc) * 0xf4);
  if ((*(int *)(iVar8 + 0x3fd0 + *(int *)(iVar8 + 0x6dc) * 0xf4) != 0) && (piVar1[5] != 0)) {
    fVar2 = *(float *)(iVar8 + 0x58e0);
    fVar3 = *(float *)(iVar8 + 0x58e4);
    fVar4 = *(float *)(iVar8 + 0x58e8);
    FUN_00502ea0(param_1,piVar1,local_40);
    iVar8 = *(int *)(param_1 + 0xe8);
    fVar5 = *(float *)(iVar8 + 0x58e0);
    fVar6 = *(float *)(iVar8 + 0x58e4);
    fVar7 = *(float *)(iVar8 + 0x58e8);
    fVar9 = FUN_004fd4c0(param_1);
    iVar8 = *(int *)(param_1 + 0xe8);
    fVar10 = moveTowards(SQRT(fVar2 * fVar2 + fVar3 * fVar3 + fVar4 * fVar4),
                         SQRT(*(float *)(iVar8 + 0x58e0) * *(float *)(iVar8 + 0x58e0) +
                              *(float *)(iVar8 + 0x58e4) * *(float *)(iVar8 + 0x58e4) +
                              *(float *)(iVar8 + 0x58e8) * *(float *)(iVar8 + 0x58e8)) * (float)fVar9,5.0,
                         param_3);
    local_8c = (float)fVar10;
    if (*(int *)(*(int *)(param_1 + 0xe8) + 0x50) != 1) {
      local_8c = -1.0;
    }
    iVar8 = FUN_00503440(param_1,piVar1,local_8c,param_3);
    if (iVar8 != 0) {
      piVar1[1] = *(int *)(piVar1[6] + -0x88 + piVar1[5] * 0x88);
      FUN_00502ea0(param_1,piVar1,local_80);
      FUN_004fda10(param_1,*(int *)(param_1 + 0xe8) + 0x4390);
      if (*(int *)(*(int *)(param_1 + 0xe8) + 0x50) != 2) {
        FUN_00502c70(param_1);
        iVar8 = *(int *)(param_1 + 0xe8);
        piVar1 = (int *)(iVar8 + 0x3fb8 + *(int *)(iVar8 + 0x6dc) * 0xf4);
        *(int **)(iVar8 + 0x4388) = piVar1;
        iVar8 = *(int *)(*(int *)(param_1 + 0xe8) + 0x43b8);
        if ((iVar8 < 0) || (piVar1[5] + -1 < iVar8)) {
          *(undefined4 *)(*(int *)(param_1 + 0xe8) + 0x43b8) = 0;
        }
        iVar8 = *(int *)(*(int *)(param_1 + 0xe8) + 0x43b8);
        *piVar1 = iVar8;
        piVar1[1] = *(int *)(piVar1[6] + iVar8 * 0x88);
        return;
      }
      FUN_004f2580(param_1);
      return;
    }
    FUN_00502e60(param_1,(int)piVar1,*piVar1 - 1U & ((int)(*piVar1 - 1U) < 0) - 1);
    FUN_00502de0(param_1,(int)piVar1,*piVar1 - 1U & ((int)(*piVar1 - 1U) < 0) - 1);
    FUN_004f3800(param_1,*(int *)(param_1 + 0xe8),*(int *)(*(int *)(param_1 + 0xe8) + 0x573c),param_3);
    FUN_00502ea0(param_1,piVar1,local_80);
    iVar8 = *(int *)(param_1 + 0xe8);
    fVar2 = SQRT(*(float *)(iVar8 + 0x58e0) * *(float *)(iVar8 + 0x58e0) +
                 *(float *)(iVar8 + 0x58e4) * *(float *)(iVar8 + 0x58e4) +
                 *(float *)(iVar8 + 0x58e8) * *(float *)(iVar8 + 0x58e8));
    iVar8 = FUN_00503190(param_1,(int)local_40,local_80);
    if (iVar8 == 0) {
      FUN_005037f0(param_1,(float *)(*(int *)(param_1 + 0xe8) + 0x57f4),local_40,local_80,param_3);
      iVar8 = *(int *)(param_1 + 0xe8);
      if ((fVar2 + SQRT(fVar5 * fVar5 + fVar6 * fVar6 + fVar7 * fVar7)) * 0.65 <
          SQRT(*(float *)(iVar8 + 0x58e0) * *(float *)(iVar8 + 0x58e0) +
               *(float *)(iVar8 + 0x58e4) * *(float *)(iVar8 + 0x58e4) +
               *(float *)(iVar8 + 0x58e8) * *(float *)(iVar8 + 0x58e8)) * (float)fVar9) {
        fVar3 = SQRT(*(float *)(iVar8 + 0x58e0) * *(float *)(iVar8 + 0x58e0) +
                     *(float *)(iVar8 + 0x58e4) * *(float *)(iVar8 + 0x58e4) +
                     *(float *)(iVar8 + 0x58e8) * *(float *)(iVar8 + 0x58e8));
        if ((1e-06 <= fVar3) && (1e-06 <= ABS(fVar3 - 1.0))) {
          fVar3 = 1.0 / fVar3;
          *(float *)(iVar8 + 0x58e0) = fVar3 * *(float *)(iVar8 + 0x58e0);
          *(float *)(iVar8 + 0x58e4) = fVar3 * *(float *)(iVar8 + 0x58e4);
          *(float *)(iVar8 + 0x58e8) = fVar3 * *(float *)(iVar8 + 0x58e8);
        }
        iVar8 = *(int *)(param_1 + 0xe8);
        *(float *)(iVar8 + 0x58e0) = fVar2 * *(float *)(iVar8 + 0x58e0);
        *(float *)(iVar8 + 0x58e4) = fVar2 * *(float *)(iVar8 + 0x58e4);
        *(float *)(iVar8 + 0x58e8) = fVar2 * *(float *)(iVar8 + 0x58e8);
        iVar8 = *(int *)(param_1 + 0xe8);
        fVar2 = *(float *)(iVar8 + 0x57f4);
        *(float *)(iVar8 + 0x58f8) = fVar2 * *(float *)(iVar8 + 0x58e0);
        *(float *)(iVar8 + 0x58fc) = fVar2 * *(float *)(iVar8 + 0x58e4);
        *(float *)(iVar8 + 0x5900) = fVar2 * *(float *)(iVar8 + 0x58e8);
      }
    }
  }
  return;
}


/* ===== FUN_00503cf0 @ 00503cf0  size=400  callers: FUN_004f16a0  ===== */

undefined4 __cdecl FUN_00503cf0(int param_1,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint local_8;
  undefined4 local_4;
  
  uVar4 = 0;
  uVar3 = 0;
  uVar5 = 0;
  local_4 = 0;
  local_8 = 0;
  if ((param_2 != 0) && (iVar1 = *(int *)(param_2 + 0xe0), iVar1 != 0)) {
    uVar5 = *(uint *)(iVar1 + 0x24);
    uVar3 = *(uint *)(iVar1 + 0x20);
    uVar4 = *(uint *)(iVar1 + 0x1c);
    local_8 = *(uint *)(iVar1 + 0x28);
  }
  iVar1 = FUN_0050bb60(0,(void *)0x0,param_1,*(float *)(*(int *)(param_1 + 0xe8) + 0xe718),
                       *(undefined ***)(*(int *)(param_1 + 0xe8) + 0x54),uVar5,uVar3,uVar4,local_8);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR__carSetupUpgrades_return_FAL_00572fa8);
    return 0;
  }
  iVar1 = FUN_0050bb60(0,(void *)0x1,param_1,*(float *)(*(int *)(param_1 + 0xe8) + 0xe71c),
                       *(undefined ***)(*(int *)(param_1 + 0xe8) + 0x54),uVar5,uVar3,uVar4,local_8);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR__carSetupUpgrades_return_FAL_00572fa8);
    return 0;
  }
  iVar1 = FUN_0050bb60(0,(void *)0x2,param_1,*(float *)(*(int *)(param_1 + 0xe8) + 0xe720),
                       *(undefined ***)(*(int *)(param_1 + 0xe8) + 0x54),uVar5,uVar3,uVar4,local_8);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR__carSetupUpgrades_return_FAL_00572fa8);
    return 0;
  }
  iVar1 = FUN_0050bb60(0,(void *)0xffffffff,param_1,(float)param_1,
                       *(undefined ***)(*(int *)(param_1 + 0xe8) + 0x54),uVar5,uVar3,uVar4,local_8);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s_CAR__carSetupUpgrades_return_FAL_00572fa8);
    return 0;
  }
  iVar1 = *(int *)(*(int *)(param_1 + 0xe8) + 0x54);
  if (iVar1 == 0) {
    local_4 = FUN_00504020(param_1);
  }
  else {
    if (iVar1 == 1) {
      uVar2 = FUN_00505070(param_1);
      return uVar2;
    }
    if (iVar1 == 2) {
      uVar2 = FUN_00505ed0(param_1);
      return uVar2;
    }
  }
  return local_4;
}


/* ===== FUN_00503f00 @ 00503f00  size=288  callers: carVisualUpdate  ===== */

void __cdecl FUN_00503f00(int *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int *piVar4;
  float10 fVar5;
  
  iVar1 = param_1[0x3a];
  iVar2 = *(int *)(iVar1 + 0x54);
  if (iVar2 == 0) {
    FUN_00504780((int)param_1,param_2);
    iVar1 = param_1[0x3a];
    if (param_2 < 0) {
      piVar4 = (int *)(iVar1 + 0x5d8c);
    }
    else {
      piVar4 = (int *)(iVar1 + 0x71b0 + param_2 * 0x1424);
    }
  }
  else if (iVar2 == 1) {
    FUN_005056e0((int)param_1,param_2);
    iVar1 = param_1[0x3a];
    if (param_2 < 0) {
      piVar4 = (int *)(iVar1 + 0xae1c);
    }
    else {
      piVar4 = (int *)(param_2 * 0x730 + 0xb54c + iVar1);
    }
  }
  else {
    piVar4 = param_1;
    if (iVar2 == 2) {
      FUN_00506840((int)param_1,param_2);
      iVar1 = param_1[0x3a];
      if (param_2 < 0) {
        piVar4 = (int *)(iVar1 + 0xcadc);
      }
      else {
        piVar4 = (int *)(param_2 * 0x670 + 0xd14c + iVar1);
      }
    }
  }
  if (param_2 != 2) {
    iVar2 = 0;
    piVar3 = piVar4;
    if (0 < *(int *)(iVar1 + 0x5928)) {
      do {
        if (*piVar3 == 0) {
          return;
        }
        iVar2 = iVar2 + 1;
        piVar3 = piVar3 + 1;
      } while (iVar2 < *(int *)(iVar1 + 0x5928));
    }
    fVar5 = FUN_0050bde0(*(int *)(*(int *)(iVar1 + 0x54) * 0xb0 + 0xe45c + iVar1));
    FUN_0050be00(*(int *)(param_1[0x3a] + 0x5928),piVar4,(float)fVar5,1);
  }
  return;
}


/* ===== FUN_00504780 @ 00504780  size=149  callers: FUN_00503f00  ===== */

void __cdecl FUN_00504780(int param_1,int param_2)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 0xe8);
  if (*(int *)(iVar1 + 0x50) != 1) {
    if (param_2 < 0) {
      FUN_00504820(param_1,param_1,param_2,iVar1 + 0x5d44);
      return;
    }
    FUN_00504820(*(int *)(iVar1 + 0xe718 + param_2 * 4),param_1,param_2,iVar1 + 0x7168 + param_2 * 0x1424);
    return;
  }
  if (param_2 < 0) {
    FUN_00505050(param_1,param_1,param_2,iVar1 + 0x5d44);
    return;
  }
  FUN_00505050(*(int *)(iVar1 + 0xe718 + param_2 * 4),param_1,param_2,iVar1 + 0x7168 + param_2 * 0x1424);
  return;
}


/* ===== FUN_00505070 @ 00505070  size=209  callers: FUN_00503cf0  ===== */

undefined4 __cdecl FUN_00505070(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_00505150(param_1,(float *)(*(int *)(param_1 + 0xe8) + 0xadd4),-1);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s__carCreateAniProc2_return_FALSE_00573384);
    return 0;
  }
  iVar1 = FUN_00505150(*(int *)(*(int *)(param_1 + 0xe8) + 0xe718),
                       (float *)(*(int *)(param_1 + 0xe8) + 0xb504),0);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s__carCreateAniProc2_return_FALSE_00573384);
    return 0;
  }
  iVar1 = FUN_00505150(*(int *)(*(int *)(param_1 + 0xe8) + 0xe71c),
                       (float *)(*(int *)(param_1 + 0xe8) + 0xbc34),1);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s__carCreateAniProc2_return_FALSE_00573384);
    return 0;
  }
  iVar1 = FUN_00505150(*(int *)(*(int *)(param_1 + 0xe8) + 0xe720),
                       (float *)(*(int *)(param_1 + 0xe8) + 0xc364),2);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s__carCreateAniProc2_return_FALSE_00573384);
    return 0;
  }
  return 1;
}


/* ===== FUN_005053d0 @ 005053d0  size=190  callers: FUN_00503ec0  ===== */

undefined4 __cdecl FUN_005053d0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_00505490(param_1,*(int *)(param_1 + 0xe8) + 0xadd4,-1);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s__carInitAniProc2_return_FALSE_005733d0);
    return 0;
  }
  iVar1 = FUN_00505490(param_1,*(int *)(param_1 + 0xe8) + 0xb504,0);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s__carInitAniProc2_return_FALSE_005733d0);
    return 0;
  }
  iVar1 = FUN_00505490(param_1,*(int *)(param_1 + 0xe8) + 0xbc34,1);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s__carInitAniProc2_return_FALSE_005733d0);
    return 0;
  }
  iVar1 = FUN_00505490(param_1,*(int *)(param_1 + 0xe8) + 0xc364,2);
  if (iVar1 == 0) {
    FUN_00401220((byte *)s__carInitAniProc2_return_FALSE_005733d0);
    return 0;
  }
  return 1;
}


/* ===== FUN_00505490 @ 00505490  size=276  callers: FUN_005053d0 FUN_005053d0 FUN_005053d0 FUN_005053d0  ===== */

undefined4 __cdecl FUN_00505490(int param_1,int param_2,int param_3)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int iVar6;
  float *pfVar7;
  undefined4 *puVar8;
  float *pfVar9;
  undefined4 *puVar10;
  int *local_4;
  
  if (param_3 != 2) {
    iVar6 = 4;
    piVar1 = (int *)(param_2 + 0x48);
    puVar5 = (undefined4 *)(param_2 + 0x60);
    do {
      iVar3 = *piVar1;
      piVar1 = piVar1 + 1;
      iVar6 = iVar6 + -1;
      puVar4 = (undefined4 *)(iVar3 + 0xac);
      puVar8 = puVar5;
      for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar8 = *puVar4;
        puVar4 = puVar4 + 1;
        puVar8 = puVar8 + 1;
      }
      puVar5 = puVar5 + 0x10;
    } while (iVar6 != 0);
    local_4 = (int *)(param_2 + 0x48);
    iVar6 = 0;
    puVar5 = (undefined4 *)(param_2 + 0x228);
    do {
      pfVar7 = (float *)(puVar5[-0x12] + 0xac);
      pfVar9 = (float *)(puVar5 + -0x11);
      for (iVar3 = 0x10; iVar3 != 0; iVar3 = iVar3 + -1) {
        *pfVar9 = *pfVar7;
        pfVar7 = pfVar7 + 1;
        pfVar9 = pfVar9 + 1;
      }
      puVar4 = (undefined4 *)(puVar5[-1] + 0xac);
      puVar8 = puVar5;
      for (iVar3 = 0x10; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar8 = *puVar4;
        puVar4 = puVar4 + 1;
        puVar8 = puVar8 + 1;
      }
      puVar4 = (undefined4 *)(puVar5[0x10] + 0xac);
      puVar8 = puVar5 + 0x11;
      for (iVar3 = 0x10; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar8 = *puVar4;
        puVar4 = puVar4 + 1;
        puVar8 = puVar8 + 1;
      }
      FUN_005055b0(param_1,iVar6,puVar5[-0x12],(float *)(puVar5 + -0x11),*local_4);
      iVar6 = iVar6 + 1;
      local_4 = local_4 + 1;
      puVar5 = puVar5 + 0x33;
    } while (iVar6 < 4);
    if (param_3 < 1) {
      param_3 = 4;
      puVar5 = (undefined4 *)(param_2 + 0x518);
      do {
        piVar1 = puVar5 + -2;
        iVar6 = 2;
        puVar4 = puVar5;
        do {
          iVar3 = *piVar1;
          piVar1 = piVar1 + 1;
          iVar6 = iVar6 + -1;
          puVar8 = (undefined4 *)(iVar3 + 0xac);
          puVar10 = puVar4;
          for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
            *puVar10 = *puVar8;
            puVar8 = puVar8 + 1;
            puVar10 = puVar10 + 1;
          }
          puVar4 = puVar4 + 0x10;
        } while (iVar6 != 0);
        puVar5 = puVar5 + 0x22;
        param_3 = param_3 + -1;
      } while (param_3 != 0);
    }
  }
  return 1;
}


