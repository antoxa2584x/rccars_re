/* ===== FUN_00474ed0 @ 00474ed0  size=159  callers: FUN_00501820 FUN_00528ed0 FUN_004f0460 FUN_004f0460 FUN_0050e810 FUN_0050e810 FUN_00475c80 FUN_00476420 FUN_0050e950 FUN_004ef280 FUN_004f0560 FUN_004f5b60 FUN_004f6730 FUN_004edac0 FUN_00476c50 FUN_004ee280 FUN_004eeea0 FUN_004eeea0  ===== */

void __cdecl FUN_00474ed0(int param_1,float *param_2,float *param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  
  *param_3 = *(float *)(param_1 + 0xec);
  param_3[1] = *(float *)(param_1 + 0xf0);
  param_3[2] = *(float *)(param_1 + 0xf4);
  fVar5 = *param_2 - *(float *)(param_1 + 0x90);
  fVar7 = param_2[1] - *(float *)(param_1 + 0x94);
  fVar6 = param_2[2] - *(float *)(param_1 + 0x98);
  fVar1 = *(float *)(param_1 + 0x100);
  fVar2 = *(float *)(param_1 + 0xf8);
  fVar3 = *(float *)(param_1 + 0xf8);
  fVar4 = *(float *)(param_1 + 0xfc);
  *param_3 = (fVar6 * *(float *)(param_1 + 0xfc) - fVar7 * *(float *)(param_1 + 0x100)) + *param_3;
  param_3[1] = (fVar5 * fVar1 - fVar6 * fVar2) + param_3[1];
  param_3[2] = (fVar7 * fVar3 - fVar5 * fVar4) + param_3[2];
  return;
}


/* ===== FUN_0040c7f0 @ 0040c7f0  size=133  callers: modpTRSCalcLTM FUN_0040c940 FUN_00406b80 FUN_00408040 FUN_0044f850 FUN_0044a030 FUN_0044c9a0 FUN_00474e00 FUN_00474e00 FUN_00503190 FUN_00503190 FUN_00474d30 FUN_00474d30 FUN_00452610 FUN_00504820 FUN_00503cb0 FUN_00505780 FUN_005068e0 FUN_00406730 FUN_00406730 FUN_0044d3a0 FUN_0044e2f0 FUN_0044e2f0 FUN_00472cc0 carVisualUpdate FUN_00436520  ===== */

void __cdecl FUN_0040c7f0(int param_1,int param_2,float *param_3)

{
  float *pfVar1;
  float *pfVar2;
  float *pfVar3;
  float *pfVar4;
  int iVar5;
  int iVar6;
  
  iVar5 = 3;
  pfVar2 = (float *)(param_1 + 4);
  pfVar3 = param_3;
  do {
    iVar6 = 4;
    pfVar1 = (float *)(param_2 + 0x10);
    pfVar4 = pfVar3;
    do {
      pfVar3 = pfVar4 + 1;
      iVar6 = iVar6 + -1;
      *pfVar4 = *pfVar1 * *pfVar2 + pfVar1[4] * pfVar2[1] + pfVar1[-4] * pfVar2[-1];
      pfVar1 = pfVar1 + 1;
      pfVar4 = pfVar3;
    } while (iVar6 != 0);
    pfVar2 = pfVar2 + 4;
    iVar5 = iVar5 + -1;
  } while (iVar5 != 0);
  iVar5 = 4;
  pfVar2 = (float *)(param_2 + 0x20);
  pfVar3 = param_3 + 0xc;
  do {
    iVar5 = iVar5 + -1;
    *pfVar3 = *pfVar2 * *(float *)(param_1 + 0x38) +
              pfVar2[-4] * *(float *)(param_1 + 0x34) +
              pfVar2[-8] * *(float *)(param_1 + 0x30) + pfVar2[4] * *(float *)(param_1 + 0x3c);
    pfVar2 = pfVar2 + 1;
    pfVar3 = pfVar3 + 1;
  } while (iVar5 != 0);
  return;
}


/* ===== FUN_0040cbb0 @ 0040cbb0  size=26  callers: FUN_00406430 FUN_0044ab80 FUN_00449fc0 FUN_0044a030 FUN_0044a030 FUN_00503190 FUN_00503190 FUN_00503190 FUN_00503190 FUN_00503190 FUN_00474f70 FUN_00474700 FUN_00474700 FUN_00508b50 FUN_00471ca0 FUN_004fd45c FUN_00406a00 FUN_004fdb50 FUN_004fe3f0 FUN_00508600 FUN_00504820 FUN_00505780 FUN_00505780 FUN_00505780 FUN_005068e0 FUN_005068e0 FUN_005068e0 carVisualBody carVisualBody FUN_004487f0 FUN_004487f0 FUN_0051d9c0 FUN_004063e0 FUN_004a3b80 FUN_0040dc90 FUN_00459cd0 FUN_00459cd0 FUN_00459cd0 FUN_00457650 FUN_0045c2c0 FUN_004eb670 FUN_004eb670 FUN_004ebb40 FUN_00509ff0 FUN_0050acf0 FUN_00531df0 FUN_00531ff0 FUN_00531ff0 FUN_00531ff0 FUN_004a0f60 FUN_0050c7b0 FUN_0050cea0 FUN_00511b50 FUN_00511ef0 FUN_00512830 FUN_00513df0 FUN_0044c220 FUN_0044c220 FUN_00515220 FUN_00519020 FUN_00532d80 FUN_004f3470 FUN_0050dd30 FUN_004e1c20 FUN_004e1c20 FUN_0050dd70 FUN_004e1c20 FUN_0050e190 carWheelPointsToWorld FUN_0050e0b0 FUN_0050df90 FUN_0050de90 FUN_0050de00 FUN_00483860 FUN_00517f10 FUN_005184b0 FUN_00515570 FUN_00515570 FUN_00515840 FUN_00515a60 FUN_00535840 FUN_00516f50 FUN_00516f50 FUN_0050ec10 FUN_0050ec10 FUN_00517530 FUN_00517530 FUN_00518050 FUN_00518050 FUN_005187d0 FUN_00535920 FUN_005359f0 FUN_005359f0 FUN_005358c0 FUN_00512b30 FUN_00513100 FUN_00513100 FUN_00513200 FUN_004e22f0 FUN_004067a0 FUN_0052abc0 FUN_004f32e0 FUN_004f39e0 FUN_004afdc0 FUN_004afdc0 FUN_00522f60 FUN_004f6b20 FUN_004f6b20 FUN_004ea8d0 FUN_004b7910 FUN_004b7910 FUN_004e8f00 FUN_004e8f00 FUN_00479250 FUN_004eb360 FUN_004eb360 FUN_004eb550 FUN_0052a030 FUN_00494f10 FUN_00494fb0 FUN_0051a950 FUN_00510a40 FUN_00510a40 FUN_00510f40 FUN_00510f40 FUN_00510ea0 FUN_00511470 FUN_00511470 FUN_004fe5f0 FUN_004eae50 FUN_004fe9d0 FUN_004fe9d0 FUN_00518b80 FUN_00514760 FUN_00519400 FUN_00519400 FUN_00519400 FUN_00514100 FUN_00514100 FUN_00514100 FUN_00508290 FUN_0052dc50 FUN_00508160 FUN_00508a10 FUN_00509690 FUN_004870f0 FUN_004870f0 FUN_0050f930 FUN_0052bd70 FUN_0052bd70 FUN_00510320 FUN_005107a0 FUN_005107a0 FUN_00510b90 FUN_005111f0 FUN_005111f0 FUN_005111f0 FUN_005072f0 FUN_004fcfe0 FUN_004f27a0 FUN_004ff2d0 FUN_004ff2d0 FUN_004ffe30 FUN_00504470 FUN_00506360 FUN_005065c0 FUN_00531650 FUN_00533340 FUN_00533340 FUN_005335a0 FUN_005335a0 FUN_00501c10 FUN_00501c10 carVisualUpdate carVisualWheels carVisualWheels FUN_0052fd00 FUN_00512b30 FUN_00512b30 FUN_00448990 FUN_00407eb0 FUN_00500d60 FUN_0050d3f0 FUN_0047f030 FUN_00481700 FUN_00481f40 FUN_00481f40 FUN_00480e50  ===== */

void __cdecl FUN_0040cbb0(int param_1,undefined4 *param_2)

{
  *param_2 = *(undefined4 *)(param_1 + 0x30);
  param_2[1] = *(undefined4 *)(param_1 + 0x34);
  param_2[2] = *(undefined4 *)(param_1 + 0x38);
  return;
}


/* ===== FUN_0040cbd0 @ 0040cbd0  size=26  callers: FUN_00471ca0 FUN_004fe310 FUN_00505780 FUN_005262e0 FUN_0050cea0 FUN_0050dd70 FUN_0050e190 carWheelPointsToWorld FUN_0050e0b0 FUN_0050df90 FUN_0050de90 FUN_0050de00 FUN_00516db0 FUN_00516f50 FUN_004067a0 FUN_004ef280 FUN_004f3b80 FUN_00494fb0 FUN_004fb340 FUN_004fb9e0 FUN_00508290 FUN_00508a10 carVisualSuspInner FUN_00510b90 FUN_005111f0 FUN_004f0be0 FUN_004f0d70 FUN_004f1000 FUN_0047f710  ===== */

void __cdecl FUN_0040cbd0(int param_1,undefined4 *param_2)

{
  *param_2 = *(undefined4 *)(param_1 + 0x10);
  param_2[1] = *(undefined4 *)(param_1 + 0x14);
  param_2[2] = *(undefined4 *)(param_1 + 0x18);
  return;
}


/* ===== FUN_0040cbf0 @ 0040cbf0  size=26  callers: FUN_004f2360 FUN_00500840 FUN_00406660 FUN_004fddd0 FUN_00508600 FUN_00504820 FUN_00509ff0 FUN_005262e0 FUN_00528ed0 FUN_004a0f60 FUN_00511b50 FUN_00511b50 FUN_00511ef0 FUN_00512830 FUN_00513df0 FUN_00515220 FUN_00519020 FUN_004e1c20 FUN_0050e190 FUN_0050de90 FUN_0052f310 FUN_00515570 FUN_00516f50 FUN_004067a0 FUN_004ef280 FUN_004f39e0 FUN_004b7910 FUN_004b7910 FUN_004eb360 FUN_00494fb0 FUN_004fb340 FUN_00510a40 FUN_00510f40 FUN_004fe9d0 FUN_00519400 FUN_00519400 FUN_00514100 FUN_00508290 FUN_0052dc50 carVisualSusp carVisualSuspInner FUN_0050f930 FUN_00510b90 FUN_00510b90 FUN_005111f0 FUN_005072f0 FUN_00504470 carVisualUpdate FUN_0049af70 FUN_00512b30 FUN_00512b30 FUN_004eea50 FUN_004f0be0 FUN_0047f710  ===== */

void __cdecl FUN_0040cbf0(int param_1,undefined4 *param_2)

{
  *param_2 = *(undefined4 *)(param_1 + 0x20);
  param_2[1] = *(undefined4 *)(param_1 + 0x24);
  param_2[2] = *(undefined4 *)(param_1 + 0x28);
  return;
}


/* ===== FUN_00406640 @ 00406640  size=25  callers: FUN_005068e0 FUN_00528ed0 FUN_0044c220 FUN_0044c220 FUN_0050e190 FUN_0050de90 FUN_004067a0 FUN_00494f10 FUN_0044d080 FUN_0044d3a0 FUN_00514760 FUN_00508290 FUN_0044e2f0 FUN_0050f930 FUN_005072f0 FUN_004f0be0  ===== */

void __cdecl FUN_00406640(undefined4 *param_1,undefined4 *param_2)

{
  *param_2 = *param_1;
  param_2[1] = param_1[1];
  param_2[2] = param_1[2];
  return;
}


/* ===== FUN_004f66e0 @ 004f66e0  size=69  callers: FUN_004f5290 FUN_00508be0 FUN_004f27a0  ===== */

void __cdecl FUN_004f66e0(int param_1)

{
  float local_4c [3];
  float local_40 [16];
  
  FUN_004f1150(param_1,local_4c);
  FUN_00475030(*(int *)(param_1 + 0xe8) + 0x57f4,local_40,local_4c);
  FUN_0044c8e0(param_1,local_40,0);
  return;
}


/* ===== FUN_004f5df0 @ 004f5df0  size=83  callers: FUN_00508600 FUN_004f5b60  ===== */

undefined4 __cdecl FUN_004f5df0(int param_1,float *param_2,int param_3)

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


/* ===== FUN_004efdc0 @ 004efdc0  size=50  callers: FUN_004fe310 FUN_004f3b80 FUN_004f3b80 FUN_004f5e50 FUN_004f5e50 FUN_004f5e50 FUN_004f5e50 FUN_004f5e50 FUN_004f0270 FUN_00509080  ===== */

void __cdecl
FUN_004efdc0(int param_1,float param_2,float param_3,int *param_4,int param_5,int param_6,int param_7)

{
  FUN_004efe00(param_1,param_2,param_3,param_4,param_5,param_6,param_7,0,-1,(undefined4 *)0x0);
  return;
}


/* ===== FUN_004f01f0 @ 004f01f0  size=50  callers: FUN_004f0270 FUN_004f0270  ===== */

void __cdecl
FUN_004f01f0(int param_1,float param_2,float param_3,int *param_4,int param_5,int param_6,int param_7)

{
  FUN_004efe00(param_1,param_2,param_3,param_4,param_5,param_6,param_7,1,-1,(undefined4 *)0x0);
  return;
}


