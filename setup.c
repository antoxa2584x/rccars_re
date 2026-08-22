/* ===== FUN_004f1930 @ 004f1930  size=1038  callers: FUN_004f6b20 FUN_004f27a0  ===== */

undefined4 __cdecl FUN_004f1930(int param_1)

{
  float fVar1;
  float fVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  
  iVar5 = *(int *)(param_1 + 0xe8);
  fVar1 = *(float *)(iVar5 + 0x57f4);
  iVar3 = *(int *)(iVar5 + 0x5928);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  iVar6 = 0;
  fVar2 = (float)puVar4[5];
  if (0 < iVar3) {
    iVar7 = 0;
    do {
      *(float *)(iVar7 + 0x5954 + iVar5) = (fVar1 * 10.0) / ((float)iVar3 * fVar2);
      puVar4 = cfgSpringParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
      *(undefined4 *)(iVar7 + 0x5958 + *(int *)(param_1 + 0xe8)) = puVar4[7];
      puVar4 = cfgSpringParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
      *(undefined4 *)(iVar7 + 0x5960 + *(int *)(param_1 + 0xe8)) = puVar4[2];
      puVar4 = cfgSpringParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
      *(undefined4 *)(iVar7 + 0x5964 + *(int *)(param_1 + 0xe8)) = puVar4[3];
      puVar4 = cfgSpringParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
      *(undefined4 *)(iVar7 + 0x5968 + *(int *)(param_1 + 0xe8)) = puVar4[4];
      puVar4 = cfgSpringParams(*(int *)(*(int *)(param_1 + 0xe8) + 0x54));
      iVar6 = iVar6 + 1;
      *(undefined4 *)(iVar7 + 0x596c + *(int *)(param_1 + 0xe8)) = puVar4[5];
      iVar5 = *(int *)(param_1 + 0xe8);
      iVar7 = iVar7 + 0x68;
    } while (iVar6 < *(int *)(iVar5 + 0x5928));
  }
  FUN_004f10d0(param_1,0,(undefined4 *)(*(int *)(param_1 + 0xe8) + 0x5948));
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(iVar5 + 0x5948) = (float)puVar4[8] + *(float *)(iVar5 + 0x5948);
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(iVar5 + 0x594c) = (float)puVar4[9] + *(float *)(iVar5 + 0x594c);
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(iVar5 + 0x5950) = (float)puVar4[10] + *(float *)(iVar5 + 0x5950);
  FUN_004f10d0(param_1,1,(undefined4 *)(*(int *)(param_1 + 0xe8) + 0x59b0));
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(*(int *)(param_1 + 0xe8) + 0x59b0) = *(float *)(iVar5 + 0x59b0) - (float)puVar4[8];
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(iVar5 + 0x59b4) = (float)puVar4[9] + *(float *)(iVar5 + 0x59b4);
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(iVar5 + 0x59b8) = (float)puVar4[10] + *(float *)(iVar5 + 0x59b8);
  FUN_004f10d0(param_1,2,(undefined4 *)(*(int *)(param_1 + 0xe8) + 0x5a18));
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(iVar5 + 0x5a18) = (float)puVar4[8] + *(float *)(iVar5 + 0x5a18);
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(iVar5 + 0x5a1c) = (float)puVar4[9] + *(float *)(iVar5 + 0x5a1c);
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(*(int *)(param_1 + 0xe8) + 0x5a20) = *(float *)(iVar5 + 0x5a20) - (float)puVar4[10];
  FUN_004f10d0(param_1,3,(undefined4 *)(*(int *)(param_1 + 0xe8) + 0x5a80));
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(*(int *)(param_1 + 0xe8) + 0x5a80) = *(float *)(iVar5 + 0x5a80) - (float)puVar4[8];
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(iVar5 + 0x5a84) = (float)puVar4[9] + *(float *)(iVar5 + 0x5a84);
  iVar5 = *(int *)(param_1 + 0xe8);
  puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
  *(float *)(*(int *)(param_1 + 0xe8) + 0x5a88) = *(float *)(iVar5 + 0x5a88) - (float)puVar4[10];
  if (*(int *)(*(int *)(param_1 + 0xe8) + 0x54) == 2) {
    FUN_004f10d0(param_1,4,(undefined4 *)(*(int *)(param_1 + 0xe8) + 0x5ae8));
    iVar5 = *(int *)(param_1 + 0xe8);
    puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
    *(float *)(*(int *)(param_1 + 0xe8) + 0x5ae8) = *(float *)(iVar5 + 0x5ae8) - (float)puVar4[8];
    iVar5 = *(int *)(param_1 + 0xe8);
    puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
    *(float *)(iVar5 + 0x5aec) = (float)puVar4[9] + *(float *)(iVar5 + 0x5aec);
    iVar5 = *(int *)(param_1 + 0xe8);
    puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
    *(float *)(*(int *)(param_1 + 0xe8) + 0x5af0) = *(float *)(iVar5 + 0x5af0) - (float)puVar4[10];
    FUN_004f10d0(param_1,5,(undefined4 *)(*(int *)(param_1 + 0xe8) + 0x5b50));
    iVar5 = *(int *)(param_1 + 0xe8);
    puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
    *(float *)(*(int *)(param_1 + 0xe8) + 0x5b50) = *(float *)(iVar5 + 0x5b50) - (float)puVar4[8];
    iVar5 = *(int *)(param_1 + 0xe8);
    puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
    *(float *)(iVar5 + 0x5b54) = (float)puVar4[9] + *(float *)(iVar5 + 0x5b54);
    iVar5 = *(int *)(param_1 + 0xe8);
    puVar4 = cfgSpringParams(*(int *)(iVar5 + 0x54));
    *(float *)(*(int *)(param_1 + 0xe8) + 0x5b58) = *(float *)(iVar5 + 0x5b58) - (float)puVar4[10];
  }
  return 1;
}


