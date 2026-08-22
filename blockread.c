/* ===== FUN_10010d40 @ 10010d40  size=127 ===== */

uint __cdecl FUN_10010d40(int param_1,char *param_2,uint param_3,uint param_4)

{
  uint uVar1;
  
  if (param_3 * param_4 == 0) {
    return 0;
  }
  uVar1 = FUN_10011dee(param_2,param_3,param_4,*(int **)(param_1 + 0x22c));
  *(uint *)(param_1 + 0x230) = *(int *)(param_1 + 0x230) + uVar1 * param_3;
  if ((*(uint *)(param_1 + 4) & 0x8000) != 0) {
    if (param_3 == 2) {
      FUN_1000afd0(param_2,param_4);
      return uVar1;
    }
    if (param_3 == 4) {
      FUN_1000aff0((int)param_2,param_4);
    }
  }
  return uVar1;
}



/* ===== FUN_10010520 @ 10010520  size=74 ===== */

uint __cdecl FUN_10010520(int param_1,LPVOID param_2,uint param_3,int param_4)

{
  uint uVar1;
  
  uVar1 = param_3;
  if (param_3 * param_4 == 0) {
    return 0;
  }
  ReadFile(*(HANDLE *)(param_1 + 0x22c),param_2,param_3 * param_4,&param_3,(LPOVERLAPPED)0x0);
  *(uint *)(param_1 + 0x230) = *(int *)(param_1 + 0x230) + param_3;
  return param_3 / uVar1;
}



/* ===== FUN_1000fb40 @ 1000fb40  size=161 ===== */

int __cdecl FUN_1000fb40(int param_1,undefined4 *param_2,int param_3,int param_4)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  
  uVar3 = param_3 * param_4;
  if (uVar3 == 0) {
    return 0;
  }
  iVar1 = *(int *)(param_1 + 0x230);
  if (*(uint *)(param_1 + 0x23c) < iVar1 + uVar3) {
    uVar3 = *(uint *)(param_1 + 0x23c) - iVar1;
  }
  puVar4 = (undefined4 *)(*(int *)(param_1 + 0x22c) + iVar1);
  puVar5 = param_2;
  for (uVar2 = uVar3 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  for (uVar2 = uVar3 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *(undefined1 *)puVar5 = *(undefined1 *)puVar4;
    puVar4 = (undefined4 *)((int)puVar4 + 1);
    puVar5 = (undefined4 *)((int)puVar5 + 1);
  }
  *(uint *)(param_1 + 0x230) = *(int *)(param_1 + 0x230) + uVar3;
  if ((*(uint *)(param_1 + 4) & 0x8000) != 0) {
    if (param_3 == 2) {
      FUN_1000afd0((undefined1 *)param_2,param_4);
      return (int)uVar3 / 2;
    }
    if (param_3 == 4) {
      FUN_1000aff0((int)param_2,param_4);
    }
  }
  return (int)uVar3 / param_3;
}



