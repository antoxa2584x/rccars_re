/* ===== FUN_004fb340 @ 004fb340  size=1383  callers: carPhysTick carPhysTick  ===== */

/* WARNING: Removing unreachable block (ram,0x004fb6b4) */

int __cdecl FUN_004fb340(int param_1,float param_2,int param_3,uint *param_4)

{
  float fVar1;
  float fVar2;
  bool bVar3;
  float *pfVar4;
  undefined3 extraout_var;
  int iVar5;
  float *pfVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  float10 fVar10;
  float local_d4;
  float local_d0;
  float local_cc;
  int local_c8;
  int local_c4;
  float local_c0;
  float local_bc;
  float local_b8;
  int *local_b4;
  float *local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c [3];
  float local_90 [3];
  undefined4 local_84 [3];
  float local_78 [6];
  undefined4 local_60 [3];
  int local_54 [15];
  int local_18 [6];
  
  local_c4 = 0;
  mat4GetRow1(param_1 + 0xf8,local_84);
  mat4GetRow2(param_1 + 0xf8,local_60);
  iVar5 = *(int *)(param_1 + 0xe8);
  iVar8 = *(int *)(iVar5 + 0x5928);
  if (0 < iVar8) {
    pfVar6 = local_78;
    pfVar4 = (float *)(iVar5 + 0x595c);
    do {
      fVar1 = *pfVar4;
      pfVar4 = pfVar4 + 0x1a;
      *pfVar6 = fVar1;
      pfVar6 = pfVar6 + 1;
      iVar8 = iVar8 + -1;
    } while (iVar8 != 0);
  }
  *(undefined4 *)(iVar5 + 0xc) = 0;
  iVar5 = *(int *)(param_1 + 0xe8);
  iVar8 = 0;
  local_a4 = SQRT(*(float *)(iVar5 + 0x58e0) * *(float *)(iVar5 + 0x58e0) +
                  *(float *)(iVar5 + 0x58e4) * *(float *)(iVar5 + 0x58e4) +
                  *(float *)(iVar5 + 0x58e8) * *(float *)(iVar5 + 0x58e8));
  if (0 < *(int *)(iVar5 + 0x5928)) {
    do {
      FUN_004ef650(param_1,iVar8,local_9c,&local_cc,(float *)0x0,(float *)0x0);
      bVar3 = FUN_004fbd70(param_1,local_9c,local_cc);
      *(uint *)(*(int *)(param_1 + 0xe8) + 0xc) = CONCAT31(extraout_var,bVar3);
      if (*(int *)(*(int *)(param_1 + 0xe8) + 0xc) != 0) break;
      iVar8 = iVar8 + 1;
    } while (iVar8 < *(int *)(*(int *)(param_1 + 0xe8) + 0x5928));
  }
  if (param_4 != (uint *)0x0) {
    *param_4 = (uint)(*(int *)(*(int *)(param_1 + 0xe8) + 0xc) != 0);
  }
  iVar5 = *(int *)(param_1 + 0xe8);
  iVar9 = 0;
  iVar8 = 0;
  if (0 < *(int *)(iVar5 + 0x5928)) {
    local_b4 = local_18;
    local_c8 = 0;
    do {
      local_b0 = (float *)(iVar5 + 0x5960 + local_c8);
      pfVar4 = (float *)(iVar5 + 0x595c + local_c8);
      local_bc = *local_b0 - *(float *)(iVar5 + 0x596c + local_c8) * 0.95;
      local_a0 = *pfVar4;
      FUN_004ef650(param_1,iVar9,local_9c,&local_cc,(float *)0x0,(float *)0x0);
      local_d4 = local_cc * 0.95;
      if (0.0 <= local_d4) {
        if (local_cc < local_d4) {
          local_d4 = local_cc;
        }
      }
      else {
        local_d4 = 0.0;
      }
      iVar8 = FUN_004fb8b0(param_1,iVar9,0,0,(float *)0x0,1,local_54);
      *local_b4 = iVar8;
      if (iVar8 != 0) {
        if (*(int *)(*(int *)(param_1 + 0xe8) + 0xc) == 0) {
          iVar5 = FUN_004fb9e0(param_1,iVar9,local_d4,local_54);
          if (iVar5 != 0) goto LAB_004fb7b3;
        }
        else {
          local_ac = local_d4;
          if (0.0 <= local_d4) {
            if (local_cc * 0.99 < local_d4) {
              local_ac = local_cc * 0.99;
            }
          }
          else {
            local_ac = 0.0;
          }
          fVar10 = moveTowards(*pfVar4,local_bc,local_ac,param_2);
          *pfVar4 = (float)fVar10;
          iVar8 = FUN_004fb8b0(param_1,iVar9,0,0,(float *)0x0,1,local_54);
          if (iVar8 == 0) {
            if (local_a4 < 1.388889) {
              *(undefined4 *)(*(int *)(param_1 + 0xe8) + 8) = 0;
            }
            goto LAB_004fb7b3;
          }
          *(float *)(local_c8 + 0x595c + iVar5) = local_a0;
        }
LAB_004fb840:
        local_c4 = 1;
        iVar8 = local_c4;
        break;
      }
      iVar5 = *(int *)(param_1 + 0xe8);
      fVar1 = param_2 * *(float *)(local_c8 + 0x5934 + iVar5);
      local_d0 = fVar1;
      if (((param_3 == 0) && (*(int *)(iVar5 + 0xc) == 0)) || (1.388889 <= local_a4)) {
        local_b8 = fVar1 * 10.0;
        local_a8 = *(float *)(iVar5 + 8);
        local_c0 = *local_b0;
        if ((0.0 <= local_a8) && (local_d0 = local_b8, local_a8 <= 0.5)) {
          fVar2 = (local_b8 - fVar1) * local_a8;
          local_d0 = fVar2 + fVar2 + fVar1;
        }
      }
      else {
        *(undefined4 *)(iVar5 + 8) = 0;
        local_c0 = local_bc;
      }
      if (*(int *)(*(int *)(param_1 + 0xe8) + 0xa4) != 0) {
        local_d0 = local_b8;
      }
      if (0.0 <= local_d0) {
        if (local_cc * 0.99 < local_d0) {
          local_d0 = local_cc * 0.99;
        }
      }
      else {
        local_d0 = 0.0;
      }
      if (*pfVar4 < local_c0) {
        FUN_004fbc50(param_1,iVar9,local_d0);
      }
      local_bc = *(float *)(local_c8 + 0x595c + *(int *)(param_1 + 0xe8));
      local_b0 = (float *)*pfVar4;
      if (local_c0 < (float)local_b0) {
        fVar10 = moveTowards((float)local_b0,local_c0,local_d0,1.0);
        *pfVar4 = (float)fVar10;
        iVar5 = FUN_004fb8b0(param_1,iVar9,0,0,(float *)0x0,1,local_54);
        if (iVar5 != 0) {
          *(float *)(local_c8 + 0x595c + *(int *)(param_1 + 0xe8)) = local_bc;
        }
      }
LAB_004fb7b3:
      FUN_004ef650(param_1,iVar9,local_90,(undefined4 *)0x0,(float *)0x0,(float *)0x0);
      iVar5 = FUN_004557e0(DAT_0148f908,local_9c,local_90,(int *)0x0,(int *)0x0,0,4,(int *)0x0);
      if (iVar5 != 0) {
        *(float *)(*(int *)(param_1 + 0xe8) + 0x595c + iVar9 * 0x68) = local_a0;
        goto LAB_004fb840;
      }
      iVar5 = *(int *)(param_1 + 0xe8);
      iVar9 = iVar9 + 1;
      local_b4 = local_b4 + 1;
      local_c8 = local_c8 + 0x68;
      iVar8 = local_c4;
    } while (iVar9 < *(int *)(iVar5 + 0x5928));
  }
  iVar5 = *(int *)(param_1 + 0xe8);
  iVar9 = 0;
  if (0 < *(int *)(iVar5 + 0x5928)) {
    iVar7 = 0;
    pfVar4 = local_78;
    do {
      if (iVar8 == 0) {
        *(float *)(iVar7 + 0x5970 + iVar5) = *(float *)(iVar7 + 0x595c + iVar5) - *pfVar4;
      }
      else {
        *(undefined4 *)(iVar7 + 0x5970 + iVar5) = 0;
      }
      iVar5 = *(int *)(param_1 + 0xe8);
      iVar9 = iVar9 + 1;
      pfVar4 = pfVar4 + 1;
      iVar7 = iVar7 + 0x68;
    } while (iVar9 < *(int *)(iVar5 + 0x5928));
  }
  return iVar8;
}


/* ===== FUN_004fb8b0 @ 004fb8b0  size=296  callers: FUN_004fb340 FUN_004fb340 FUN_004fb340 FUN_004fb9e0  ===== */

undefined4 __cdecl
FUN_004fb8b0(int param_1,int param_2,int param_3,undefined4 param_4,float *param_5,int param_6,int *param_7)

{
  int iVar1;
  uint *puVar2;
  float local_2bc;
  float local_2b8;
  undefined4 local_2b4;
  undefined4 local_2b0;
  uint local_2ac;
  float local_2a8;
  int local_2a4;
  undefined4 local_2a0;
  float local_29c;
  float local_298;
  float local_290;
  undefined4 local_28c;
  undefined4 local_288;
  float local_284;
  undefined4 local_280;
  undefined4 local_27c;
  undefined4 local_260;
  int local_258 [150];
  
  if (param_5 == (float *)0x0) {
    param_5 = &local_2bc;
  }
  if ((param_7 == (int *)0x0) && (param_7 = local_258, 10 < param_6)) {
    param_6 = 10;
  }
  *param_5 = 0.0;
  FUN_004ef650(param_1,param_2,&local_2b8,&local_2bc,(float *)0x0,(float *)0x0);
  puVar2 = &local_2ac;
  for (iVar1 = 0x15; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  local_284 = local_2b8;
  local_290 = local_2b8;
  local_260 = DAT_0148f908;
  local_298 = local_2bc;
  local_2ac = 4;
  local_280 = local_2b4;
  local_27c = local_2b0;
  local_28c = local_2b4;
  local_288 = local_2b0;
  if (param_3 != 0) {
    local_29c = local_2bc * 0.2;
    local_2ac = 0x404;
    local_2a0 = param_4;
  }
  DAT_014b0bb4 = DAT_014b0bb4 + 1;
  local_2a4 = param_6;
  iVar1 = FUN_00454970(&local_2ac,param_7);
  if (iVar1 == 0) {
    return 0;
  }
  *param_5 = local_2a8;
  return 1;
}


