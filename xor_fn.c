/* ===== FUN_10006bc0 @ 10006bc0  size=178 ===== */

undefined4 __cdecl FUN_10006bc0(int *param_1,int param_2,int param_3)

{
  byte bVar1;
  int iVar2;
  uint uVar3;
  byte *pbVar4;
  int iVar5;
  byte local_20 [32];
  
  if ((param_1 == (int *)0x0) || (*param_1 != 0x5458494f)) {
    return 0;
  }
  if ((char)param_1[0x43] == '\0') {
    return 1;
  }
  local_20[0] = 0;
  _strncpy((char *)local_20,(char *)(param_1 + 0x43),0x1f);
  uVar3 = 0xffffffff;
  iVar2 = 0;
  pbVar4 = local_20;
  do {
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    bVar1 = *pbVar4;
    pbVar4 = pbVar4 + 1;
  } while (bVar1 != 0);
  iVar5 = 0;
  if (0 < param_3) {
    do {
      pbVar4 = local_20 + iVar2;
      bVar1 = local_20[iVar2 + 1];
      iVar2 = iVar2 + 1;
      *(byte *)(iVar5 + param_2) = *(byte *)(iVar5 + param_2) ^ *pbVar4;
      if (bVar1 == 0) {
        iVar2 = 0;
        if (0 < (int)(~uVar3 - 1)) {
          do {
            local_20[iVar2] = local_20[iVar2] * '\x03' + 1;
            iVar2 = iVar2 + 1;
          } while (iVar2 < (int)(~uVar3 - 1));
        }
        iVar2 = 0;
      }
      iVar5 = iVar5 + 1;
    } while (iVar5 < param_3);
  }
  return 1;
}



