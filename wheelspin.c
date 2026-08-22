/* ===== FUN_004ef280 @ 004ef280  size=973  callers: FUN_004f6ea0  ===== */

/* WARNING: Removing unreachable block (ram,0x004ef563) */

void FUN_004ef280(void)

{
  int iVar1;
  undefined4 *puVar2;
  int *piVar3;
  int iVar4;
  int iVar5;
  float10 fVar6;
  float fVar7;
  float fVar8;
  float fStack00000004;
  int iStack0000000c;
  float in_stack_00000010;
  float in_stack_00000014;
  float in_stack_00000018;
  float in_stack_0000001c;
  float in_stack_00000020;
  float in_stack_00000024;
  float in_stack_00000028;
  int iStack0000002c;
  float in_stack_00000030;
  float in_stack_00000034;
  float in_stack_00000038;
  float in_stack_0000003c;
  float in_stack_00000040;
  float in_stack_00000044;
  int in_stack_00003bbc;
  float in_stack_00003bc0;
  
  FUN_005462b0();
  iStack0000000c = 1;
  iVar4 = *(int *)(in_stack_00003bbc + 0xe8);
  if (*(int *)(iVar4 + 0x50) == 0) {
    iStack0000000c =
         carGatherWheelContacts
                   (in_stack_00003bbc,(float)(iVar4 + 0x57f4),0.006,(int *)&stack0x00000048,
                    (int)&stack0x00001e08,0x32,*(int *)(iVar4 + 0x5928),(undefined4 *)&stack0x00000058);
  }
  else {
    puVar2 = (undefined4 *)&stack0x00000058;
    for (iVar4 = 0x76c; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
  }
  mat4GetRow1(in_stack_00003bbc + 0xf8,&stack0x0000003c);
  mat4GetRow2(in_stack_00003bbc + 0xf8,(undefined4 *)&stack0x0000004c);
  iVar4 = 0;
  iStack0000002c = *(int *)(in_stack_00003bbc + 0xe8) + 0x57f4;
  if (0 < *(int *)(*(int *)(in_stack_00003bbc + 0xe8) + 0x5928)) {
    iVar5 = 0;
    piVar3 = (int *)&stack0x000000a4;
    do {
      carWheelFrameNoExtra
                (in_stack_00003bbc,iVar4,&stack0x00000010,&stack0x00000028,&stack0x00000030,(float *)0x0);
      fVar8 = -in_stack_00000028;
      in_stack_00000010 = in_stack_0000003c * fVar8 + in_stack_00000010;
      in_stack_00000014 = in_stack_00000040 * fVar8 + in_stack_00000014;
      in_stack_00000018 = in_stack_00000044 * fVar8 + in_stack_00000018;
      rbPointVelocity(iStack0000002c,&stack0x00000010,&stack0x0000001c);
      iVar1 = *(int *)(in_stack_00003bbc + 0xe8);
      if ((*(int *)(iVar1 + 0x50) == 0) && (*piVar3 == 0)) {
        in_stack_0000001c = DAT_0070c658;
        in_stack_00000020 = DAT_0070c65c;
        in_stack_00000024 = DAT_0070c660;
      }
      fVar7 = 200.0;
      fVar8 = (in_stack_00000030 * in_stack_0000001c +
              in_stack_00000034 * in_stack_00000020 + in_stack_00000038 * in_stack_00000024) /
              *(float *)(iVar5 + 0x5934 + iVar1);
      *(float *)(iVar5 + 0x5940 + iVar1) = fVar8;
      if (iStack0000000c == 0) {
        fVar7 = 5.0;
      }
      iVar1 = *(int *)(in_stack_00003bbc + 0xe8);
      if (*(int *)(iVar1 + 0x57ec) == 0) {
        fVar6 = moveTowards(*(float *)(iVar5 + 0x593c + iVar1),fVar8,fVar7,in_stack_00003bc0);
        *(float *)(iVar5 + 0x593c + *(int *)(in_stack_00003bbc + 0xe8)) = (float)fVar6;
      }
      else {
        *(undefined4 *)(iVar5 + 0x593c + iVar1) = 0;
      }
      iVar1 = *(int *)(in_stack_00003bbc + 0xe8);
      if (((*(byte *)(iVar1 + 0x576c) & 1) == 0) || (*(float *)(iVar1 + 0x5770) <= 0.3)) {
        if (((*(byte *)(iVar1 + 0x5774) & 1) == 0) || (*(float *)(iVar1 + 0x5778) <= 0.3)) {
          fStack00000004 = 0.0;
          fVar8 = 35.0;
        }
        else {
          puVar2 = cfgTireParams(*(int *)(iVar1 + 0x54));
          fStack00000004 = -(float)puVar2[0xc];
          fVar8 = 70.0;
        }
      }
      else {
        puVar2 = cfgTireParams(*(int *)(iVar1 + 0x54));
        fStack00000004 = (float)puVar2[0xc];
        fVar8 = 70.0;
      }
      if (iStack0000000c != 0) {
        iVar1 = *(int *)(in_stack_00003bbc + 0xe8);
        fVar7 = SQRT(*(float *)(iVar1 + 0x58e0) * *(float *)(iVar1 + 0x58e0) +
                     *(float *)(iVar1 + 0x58e4) * *(float *)(iVar1 + 0x58e4) +
                     *(float *)(iVar1 + 0x58e8) * *(float *)(iVar1 + 0x58e8));
        if (0.0 <= fVar7) {
          if (fVar7 <= 4.0) {
            fVar7 = 1.0 - fVar7 * 0.25;
          }
          else {
            fVar7 = 0.0;
          }
        }
        else {
          fVar7 = 1.0;
        }
        fStack00000004 = fVar7 * fStack00000004;
      }
      iVar1 = *(int *)(in_stack_00003bbc + 0xe8);
      if (*(int *)(iVar1 + 0x57ec) == 0) {
        fVar6 = moveTowards(*(float *)(iVar5 + 0x5944 + iVar1),fStack00000004,fVar8,in_stack_00003bc0);
        *(float *)(iVar5 + 0x5944 + *(int *)(in_stack_00003bbc + 0xe8)) = (float)fVar6;
      }
      else {
        *(undefined4 *)(iVar5 + 0x5944 + iVar1) = 0;
      }
      iVar1 = *(int *)(in_stack_00003bbc + 0xe8);
      *(float *)(iVar5 + iVar1 + 0x5938) =
           (*(float *)(iVar5 + 0x5944 + iVar1) + *(float *)(iVar5 + 0x593c + iVar1)) * in_stack_00003bc0 +
           *(float *)(iVar5 + iVar1 + 0x5938);
      fVar6 = FUN_0040c6d0(*(float *)(iVar5 + 0x5938 + *(int *)(in_stack_00003bbc + 0xe8)),-6.2831855,
                           6.2831855);
      iVar4 = iVar4 + 1;
      piVar3 = piVar3 + 0x26;
      *(float *)(iVar5 + 0x5938 + *(int *)(in_stack_00003bbc + 0xe8)) = (float)fVar6;
      iVar5 = iVar5 + 0x68;
    } while (iVar4 < *(int *)(*(int *)(in_stack_00003bbc + 0xe8) + 0x5928));
  }
  return;
}


/* ===== carWheelFrame @ 004ef680  size=801  callers: carWheelFrameNoExtra FUN_004ef9b0  ===== */

/* Wheel world frame. centre = mount - (0,len,0) then car matrix; front pair offset by cdt[15]. roll = local
   +Z steered about Y; side = same +-90. Hummer's middle axle steers 1/3. */

void __cdecl
carWheelFrame(int param_1,int param_2,int param_3,float *param_4,undefined4 *param_5,float *param_6,
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


/* ===== FUN_004ef9b0 @ 004ef9b0  size=41  callers: FUN_00504820 FUN_00504820 FUN_00504820 FUN_00504820 FUN_00505780 FUN_005068e0 FUN_005068e0 FUN_005262e0 FUN_00531df0  ===== */

void __cdecl
FUN_004ef9b0(int param_1,int param_2,float *param_3,undefined4 *param_4,float *param_5,float *param_6)

{
  carWheelFrame(param_1,param_2,1,param_3,param_4,param_5,param_6);
  return;
}


/* ===== FUN_0040cab0 @ 0040cab0  size=116  callers: FUN_004089b0 FUN_00405980 FUN_00405ad0 FUN_0044c870 FUN_00504820 FUN_00504820 FUN_00505780 FUN_00505780 FUN_005068e0 FUN_005068e0 FUN_005068e0 FUN_005068e0 FUN_00516f50 FUN_00518050 FUN_00482900 FUN_00482900 FUN_0047f030 FUN_004062c0  ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_0040cab0(float *param_1,float param_2,int param_3)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  float10 fVar4;
  float10 fVar5;
  
  if (DAT_0055e984 != 0) {
    DAT_0055e984 = 0;
    puVar2 = &DAT_0055e940;
    puVar3 = &DAT_00585bf0;
    for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar3 = puVar3 + 1;
    }
  }
  fVar4 = (float10)fcos((float10)param_2 * (float10)0.017453292);
  fVar5 = (float10)fsin((float10)(float)((float10)param_2 * (float10)0.017453292));
  DAT_00585bf0 = (float)fVar4;
  _DAT_00585bf8 = (float)-fVar5;
  _DAT_00585c10 = (float)fVar5;
  _DAT_00585c18 = (float)fVar4;
  FUN_0040c940(param_1,&DAT_00585bf0,param_3);
  return;
}


/* ===== FUN_0040cb30 @ 0040cb30  size=114  callers: FUN_004089b0 FUN_00504820 FUN_00504820 FUN_00504820 FUN_00505780 FUN_005068e0 FUN_005068e0 FUN_005068e0 FUN_00516f50 FUN_004fe9d0 FUN_004ff2d0 FUN_005065c0 FUN_0047f030  ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_0040cb30(float *param_1,float param_2,int param_3)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  float10 fVar4;
  float10 fVar5;
  
  if (DAT_0055e988 != 0) {
    DAT_0055e988 = 0;
    puVar2 = &DAT_0055e940;
    puVar3 = &DAT_00585b70;
    for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar3 = puVar3 + 1;
    }
  }
  fVar4 = (float10)fcos((float10)param_2 * (float10)0.017453292);
  fVar5 = (float10)fsin((float10)(float)((float10)param_2 * (float10)0.017453292));
  DAT_00585b70 = (float)fVar4;
  DAT_00585b74 = (float)fVar5;
  _DAT_00585b80 = (float)-fVar5;
  _DAT_00585b84 = (float)fVar4;
  FUN_0040c940(param_1,&DAT_00585b70,param_3);
  return;
}


/* ===== FUN_0040ca30 @ 0040ca30  size=114  callers: FUN_004089b0 FUN_00504820 FUN_00504820 FUN_00505780 FUN_005068e0 FUN_00516f50 FUN_0044c840 FUN_00482900 FUN_00482900 FUN_005055b0 FUN_005065c0 FUN_0047f030 FUN_004062a0  ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_0040ca30(float *param_1,float param_2,int param_3)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  float10 fVar4;
  float10 fVar5;
  
  if (DAT_0055e980 != 0) {
    DAT_0055e980 = 0;
    puVar2 = &DAT_0055e940;
    puVar3 = &DAT_00585bb0;
    for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar3 = puVar3 + 1;
    }
  }
  fVar4 = (float10)fcos((float10)param_2 * (float10)0.017453292);
  fVar5 = (float10)fsin((float10)(float)((float10)param_2 * (float10)0.017453292));
  _DAT_00585bc4 = (float)fVar4;
  _DAT_00585bc8 = (float)fVar5;
  _DAT_00585bd4 = (float)-fVar5;
  _DAT_00585bd8 = (float)fVar4;
  FUN_0040c940(param_1,(float *)&DAT_00585bb0,param_3);
  return;
}


/* ===== FUN_0040c9a0 @ 0040c9a0  size=138  callers: FUN_004089b0 FUN_00408240 FUN_00407370 FUN_00448860 FUN_0050be40 FUN_00504820 FUN_0050be00 FUN_00407420 FUN_004b0fa0 FUN_0051b450 FUN_00466d80  ===== */

void __cdecl FUN_0040c9a0(float *param_1,float param_2,float param_3,float param_4,int param_5)

{
  float *pfVar1;
  int iVar2;
  
  if (param_5 == 0) {
    *param_1 = param_2;
    param_1[5] = param_3;
    param_1[10] = param_4;
  }
  else {
    if (param_5 == 1) {
      iVar2 = 3;
      pfVar1 = param_1 + 8;
      do {
        iVar2 = iVar2 + -1;
        pfVar1[-8] = param_2 * pfVar1[-8];
        pfVar1[-4] = param_3 * pfVar1[-4];
        *pfVar1 = param_4 * *pfVar1;
        pfVar1 = pfVar1 + 1;
      } while (iVar2 != 0);
      return;
    }
    if (param_5 == 2) {
      iVar2 = 4;
      pfVar1 = param_1 + 2;
      do {
        iVar2 = iVar2 + -1;
        pfVar1[-2] = param_2 * pfVar1[-2];
        pfVar1[-1] = param_3 * pfVar1[-1];
        *pfVar1 = param_4 * *pfVar1;
        pfVar1 = pfVar1 + 4;
      } while (iVar2 != 0);
      return;
    }
  }
  return;
}


/* ===== FUN_0040c880 @ 0040c880  size=181  callers: FUN_004089b0 FUN_00408540 FUN_00406b80 FUN_00408240 FUN_00408240 FUN_0044c8a0 FUN_00475030 FUN_00475030 FUN_00503190 FUN_00474f70 FUN_00504820 FUN_00505780 FUN_00505780 FUN_005068e0 FUN_005068e0 FUN_005068e0 FUN_0052b860 FUN_0052b860 FUN_0052c180 FUN_00516f50 FUN_00516f50 FUN_00518050 FUN_00518050 FUN_00482900 FUN_00482900 FUN_00482900 FUN_00482900 FUN_00482900 FUN_00482900 FUN_004083f0 FUN_004083f0 FUN_004fe9d0 FUN_004fe9d0 FUN_004fe9d0 FUN_0044e280 FUN_0044e2f0 FUN_005072f0 FUN_005072f0 FUN_004ff2d0 FUN_004ec770 FUN_00407eb0 FUN_00406210 FUN_00406240 FUN_00406270  ===== */

void __cdecl FUN_0040c880(float *param_1,float param_2,float param_3,float param_4,int param_5)

{
  if (param_5 == 0) {
    param_1[0xc] = param_2;
    param_1[0xd] = param_3;
    param_1[0xe] = param_4;
  }
  else {
    if (param_5 == 1) {
      param_1[0xc] = param_2 * *param_1 + param_1[8] * param_4 + param_1[4] * param_3 + param_1[0xc];
      param_1[0xd] = param_1[9] * param_4 + param_1[5] * param_3 + param_1[1] * param_2 + param_1[0xd];
      param_1[0xe] = param_1[10] * param_4 + param_1[6] * param_3 + param_1[2] * param_2 + param_1[0xe];
      return;
    }
    if (param_5 == 2) {
      param_1[0xc] = param_2 + param_1[0xc];
      param_1[0xd] = param_3 + param_1[0xd];
      param_1[0xe] = param_4 + param_1[0xe];
      return;
    }
  }
  return;
}


/* ===== FUN_00407620 @ 00407620  size=613  callers: camFollow FUN_00504820 FUN_00505780 FUN_005068e0 FUN_0044e2f0  ===== */

undefined4 __cdecl FUN_00407620(float *param_1,float *param_2,float *param_3)

{
  float fVar1;
  int iVar2;
  float *pfVar3;
  float10 fVar4;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_c;
  float local_8;
  float local_4;
  
  local_1c = *param_1;
  local_18 = param_1[1];
  local_14 = param_1[2];
  local_28 = *param_2;
  local_24 = param_2[1];
  local_20 = param_2[2];
  fVar1 = SQRT(local_1c * local_1c + local_18 * local_18 + local_14 * local_14);
  if ((1e-06 <= fVar1) && (1e-06 <= ABS(fVar1 - 1.0))) {
    fVar1 = 1.0 / fVar1;
    local_1c = local_1c * fVar1;
    local_18 = local_18 * fVar1;
    local_14 = local_14 * fVar1;
  }
  fVar1 = SQRT(local_28 * local_28 + local_24 * local_24 + local_20 * local_20);
  if ((1e-06 <= fVar1) && (1e-06 <= ABS(fVar1 - 1.0))) {
    fVar1 = 1.0 / fVar1;
    local_28 = local_28 * fVar1;
    local_24 = local_24 * fVar1;
    local_20 = local_20 * fVar1;
  }
  local_c = local_20 * local_18 - local_24 * local_14;
  local_8 = local_14 * local_28 - local_20 * local_1c;
  local_4 = local_24 * local_1c - local_18 * local_28;
  fVar1 = SQRT(local_c * local_c + local_8 * local_8 + local_4 * local_4);
  if (1e-06 <= fVar1) {
    if (1e-06 <= ABS(fVar1 - 1.0)) {
      fVar1 = 1.0 / fVar1;
      local_c = local_c * fVar1;
      local_8 = local_8 * fVar1;
      local_4 = local_4 * fVar1;
    }
    fVar4 = vec3AngleDeg(param_1,param_2);
    FUN_00407cf0((float)fVar4,&local_c,&local_1c);
    quatToMatrix(&local_1c,param_3);
    return 1;
  }
  if (local_28 * local_1c + local_24 * local_18 + local_20 * local_14 < 0.0) {
    FUN_0040cc10(param_3,180.0);
    return 0;
  }
  pfVar3 = (float *)&DAT_0055e940;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *param_3 = *pfVar3;
    pfVar3 = pfVar3 + 1;
    param_3 = param_3 + 1;
  }
  return 1;
}


/* ===== FUN_0044e610 @ 0044e610  size=110  callers: FUN_00449fc0 FUN_00449ec0 FUN_0044a0b0 FUN_0044a210 FUN_0050c0e0 FUN_00518820 FUN_00518820 FUN_00518820 FUN_00518820 FUN_00518820 FUN_00517f10 FUN_00516f50 FUN_00516f50 FUN_00510b90 FUN_00504100 FUN_00505150 FUN_00505fb0 FUN_005055b0 FUN_005065c0  ===== */

void __cdecl FUN_0044e610(int param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  float *pfVar3;
  float *pfVar4;
  float *pfVar5;
  int iVar6;
  int iVar7;
  float local_8 [2];
  
  iVar7 = 0;
  local_8[1] = 0.0;
  local_8[0] = 0.0;
  pfVar3 = param_2;
  do {
    *pfVar3 = 0.0;
    pfVar5 = local_8;
    pfVar4 = (float *)((param_1 - (int)param_2) + 0x7c + (int)pfVar3);
    iVar6 = 2;
    do {
      fVar1 = *pfVar4;
      fVar2 = *pfVar5;
      pfVar4 = pfVar4 + 4;
      pfVar5 = pfVar5 + 1;
      iVar6 = iVar6 + -1;
      *pfVar3 = fVar1 * fVar2 + *pfVar3;
    } while (iVar6 != 0);
    iVar6 = iVar7 * 4;
    iVar7 = iVar7 + 1;
    *pfVar3 = *(float *)(param_1 + 0x9c + iVar6) + *pfVar3;
    pfVar3 = pfVar3 + 1;
  } while (iVar7 < 3);
  return;
}


/* ===== FUN_00451420 @ 00451420  size=49  callers: FUN_00504100 FUN_00504100 FUN_00504100 FUN_00504100 FUN_00504100 FUN_00504100 FUN_00504100 FUN_00504100 FUN_00505150 FUN_00505150 FUN_00505150 FUN_00505150 FUN_00505150 FUN_00505fb0 FUN_00505fb0 FUN_00505fb0 FUN_00505fb0 FUN_00505fb0 FUN_00505fb0 FUN_00505fb0  ===== */

undefined4 __cdecl FUN_00451420(int param_1,undefined4 param_2)

{
  int local_4;
  
  local_4 = 0;
  if (param_1 == 0) {
    return 0;
  }
  DAT_007225ac = param_2;
  FUN_004514d0(param_1,&local_4);
  return local_4;
}


/* ===== FUN_00503cb0 @ 00503cb0  size=55  callers: FUN_00504820 FUN_00505780 FUN_005068e0  ===== */

void __cdecl FUN_00503cb0(int param_1,int param_2)

{
  float local_40 [16];
  
  FUN_0040b1f0(*(int *)(param_1 + 0x5c) + 0x6c,local_40);
  mat4Mul(param_2,(int)local_40,(float *)(param_1 + 0xac));
  return;
}


