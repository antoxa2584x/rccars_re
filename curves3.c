/* ===== FUN_004881d0 @ 004881d0  size=266  callers: FUN_00488080  ===== */

undefined4 __cdecl FUN_004881d0(char *param_1,char *param_2)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  undefined4 *puVar7;
  char *pcVar8;
  char local_104 [260];
  
  if (DAT_01486520 < 500) {
    iVar6 = 0;
    if (0 < DAT_01486520) {
      puVar7 = &DAT_0148652c;
      do {
        pcVar2 = tfsGetName((char *)*puVar7);
        iVar3 = __strcmpi(param_2,pcVar2);
        if (iVar3 == 0) {
          return 1;
        }
        iVar6 = iVar6 + 1;
        puVar7 = puVar7 + 0x12;
      } while (iVar6 < DAT_01486520);
    }
    pcVar2 = tfsAddPath(param_1,param_2);
    uVar4 = 0xffffffff;
    do {
      pcVar8 = pcVar2;
      if (uVar4 == 0) break;
      uVar4 = uVar4 - 1;
      pcVar8 = pcVar2 + 1;
      cVar1 = *pcVar2;
      pcVar2 = pcVar8;
    } while (cVar1 != '\0');
    uVar4 = ~uVar4;
    pcVar2 = pcVar8 + -uVar4;
    pcVar8 = local_104;
    for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
      *(undefined4 *)pcVar8 = *(undefined4 *)pcVar2;
      pcVar2 = pcVar2 + 4;
      pcVar8 = pcVar8 + 4;
    }
    for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *pcVar8 = *pcVar2;
      pcVar2 = pcVar2 + 1;
      pcVar8 = pcVar8 + 1;
    }
    pcVar2 = FUN_00402b00(local_104);
    iVar6 = DAT_01486520;
    (&DAT_0148652c)[DAT_01486520 * 0x12] = pcVar2;
    if (pcVar2 == (char *)0x0) {
      return 0;
    }
    (&DAT_01486528)[iVar6 * 0x12] = 0;
    pcVar2 = tfsAddExt(param_2,(char *)0x0);
    uVar4 = 0xffffffff;
    do {
      pcVar8 = pcVar2;
      if (uVar4 == 0) break;
      uVar4 = uVar4 - 1;
      pcVar8 = pcVar2 + 1;
      cVar1 = *pcVar2;
      pcVar2 = pcVar8;
    } while (cVar1 != '\0');
    uVar4 = ~uVar4;
    pcVar2 = pcVar8 + -uVar4;
    pcVar8 = &DAT_01486530 + DAT_01486520 * 0x48;
    for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
      *(undefined4 *)pcVar8 = *(undefined4 *)pcVar2;
      pcVar2 = pcVar2 + 4;
      pcVar8 = pcVar8 + 4;
    }
    DAT_01486520 = DAT_01486520 + 1;
    for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *pcVar8 = *pcVar2;
      pcVar2 = pcVar2 + 1;
      pcVar8 = pcVar8 + 1;
    }
  }
  return 1;
}


/* ===== FUN_0040f090 @ 0040f090  size=134  callers: FUN_00436600  ===== */

int * __cdecl FUN_0040f090(int *param_1)

{
  int iVar1;
  int *piVar2;
  undefined4 *puVar3;
  int iVar4;
  uint uVar5;
  int *piVar6;
  undefined4 *puVar7;
  int *piVar8;
  
  if (param_1 == (int *)0x0) {
    return (int *)0x0;
  }
  piVar2 = FUN_0040ef60(*param_1,param_1[1],param_1[2]);
  if (piVar2 == (int *)0x0) {
    return (int *)0x0;
  }
  piVar6 = param_1;
  piVar8 = piVar2;
  for (iVar4 = 7; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar8 = *piVar6;
    piVar6 = piVar6 + 1;
    piVar8 = piVar8 + 1;
  }
  puVar3 = (undefined4 *)FUN_00402ab0(1,param_1[3] * param_1[4]);
  piVar2[5] = (int)puVar3;
  if (puVar3 == (undefined4 *)0x0) {
    FUN_00402ad0((undefined *)piVar2);
    return (int *)0x0;
  }
  iVar4 = param_1[3];
  iVar1 = param_1[4];
  puVar7 = (undefined4 *)param_1[5];
  for (uVar5 = (uint)(iVar4 * iVar1) >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *puVar3 = *puVar7;
    puVar7 = puVar7 + 1;
    puVar3 = puVar3 + 1;
  }
  for (uVar5 = iVar4 * iVar1 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined1 *)puVar3 = *(undefined1 *)puVar7;
    puVar7 = (undefined4 *)((int)puVar7 + 1);
    puVar3 = (undefined4 *)((int)puVar3 + 1);
  }
  return piVar2;
}


