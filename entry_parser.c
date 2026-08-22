/* ===== FUN_10006c80 @ 10006c80  size=389 ===== */

void __cdecl FUN_10006c80(int param_1,int param_2,int *param_3)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  char *pcVar4;
  uint uVar5;
  char *pcVar6;
  uint local_110;
  undefined4 local_10c;
  char local_104 [260];
  
  uVar2 = tiohGetErrorState(*(int *)(param_2 + 0x140));
  do {
    if ((uVar2 != 0) ||
       (uVar2 = tiohbTell(*(int *)(param_2 + 0x140)), *(uint *)(param_1 + 4) <= uVar2)) {
      return;
    }
    tiohbReadChunk(*(int *)(param_2 + 0x140),(ushort *)&local_110);
    uVar2 = local_110 & 0xffff;
    if (uVar2 < 0x3025) {
      if (uVar2 == 0x3024) {
        iVar3 = tiohbReadDWord(*(int *)(param_2 + 0x140));
        param_3[3] = iVar3;
      }
      else if (uVar2 == 0x3022) {
        iVar3 = tiohbReadDWord(*(int *)(param_2 + 0x140));
        param_3[1] = iVar3;
      }
      else {
        if (uVar2 != 0x3023) goto LAB_10006d4d;
        iVar3 = tiohbReadDWord(*(int *)(param_2 + 0x140));
        param_3[2] = iVar3;
      }
    }
    else if (uVar2 == 0x4021) {
      tiohbReadString(*(int *)(param_2 + 0x140),local_104,0x104);
      *param_3 = *(int *)(param_2 + 0x148) + *(int *)(param_2 + 0x150);
      pcVar4 = tfsReplaceDelim(local_104,'\\');
      uVar2 = 0xffffffff;
      do {
        pcVar6 = pcVar4;
        if (uVar2 == 0) break;
        uVar2 = uVar2 - 1;
        pcVar6 = pcVar4 + 1;
        cVar1 = *pcVar4;
        pcVar4 = pcVar6;
      } while (cVar1 != '\0');
      uVar2 = ~uVar2;
      pcVar4 = pcVar6 + -uVar2;
      pcVar6 = (char *)*param_3;
      for (uVar5 = uVar2 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined4 *)pcVar6 = *(undefined4 *)pcVar4;
        pcVar4 = pcVar4 + 4;
        pcVar6 = pcVar6 + 4;
      }
      for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
        *pcVar6 = *pcVar4;
        pcVar4 = pcVar4 + 1;
        pcVar6 = pcVar6 + 1;
      }
      uVar2 = 0xffffffff;
      pcVar4 = (char *)*param_3;
      do {
        if (uVar2 == 0) break;
        uVar2 = uVar2 - 1;
        cVar1 = *pcVar4;
        pcVar4 = pcVar4 + 1;
      } while (cVar1 != '\0');
      *(uint *)(param_2 + 0x150) = *(int *)(param_2 + 0x150) + ~uVar2;
    }
    else {
LAB_10006d4d:
      tioWarning((byte *)s_Wrong_chunk_ID_0x_08X__0x_08X_ex_1001f890);
      tiohbSeek(*(int *)(param_2 + 0x140),local_10c,0);
    }
    uVar2 = tiohGetErrorState(*(int *)(param_2 + 0x140));
  } while( true );
}



/* ===== tioPackLoadRoot @ 10005ca0  size=1218 ===== */

undefined4 __cdecl tioPackLoadRoot(char *param_1,char *param_2)

{
  char cVar1;
  int iVar2;
  int *piVar3;
  undefined4 *puVar4;
  int *piVar5;
  undefined4 uVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  char *pcVar11;
  int iVar12;
  char *pcVar13;
  uint local_24;
  uint local_20;
  int local_1c;
  ushort local_18 [2];
  uint local_14;
  ushort local_c [6];
  
                    /* 0x5ca0  77  tioPackLoadRoot */
  piVar3 = &DAT_10024c8c;
  iVar9 = 0;
  do {
    if (*piVar3 == 0) break;
    piVar3 = piVar3 + 1;
    iVar9 = iVar9 + 1;
  } while ((int)piVar3 < 0x10024cdc);
  if (iVar9 == 0x14) {
    return 0;
  }
  puVar4 = (undefined4 *)FUN_100021c0(1,0x154);
  (&DAT_10024c8c)[iVar9] = puVar4;
  if (puVar4 == (undefined4 *)0x0) {
    return 0;
  }
  *puVar4 = 0x5458494f;
  uVar7 = 0xffffffff;
  *(undefined4 *)((&DAT_10024c8c)[iVar9] + 4) = 0xffffff9c;
  pcVar11 = param_1;
  do {
    pcVar13 = pcVar11;
    if (uVar7 == 0) break;
    uVar7 = uVar7 - 1;
    pcVar13 = pcVar11 + 1;
    cVar1 = *pcVar11;
    pcVar11 = pcVar13;
  } while (cVar1 != '\0');
  uVar7 = ~uVar7;
  pcVar11 = pcVar13 + -uVar7;
  pcVar13 = (char *)((&DAT_10024c8c)[iVar9] + 8);
  for (uVar8 = uVar7 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
    *(undefined4 *)pcVar13 = *(undefined4 *)pcVar11;
    pcVar11 = pcVar11 + 4;
    pcVar13 = pcVar13 + 4;
  }
  for (uVar7 = uVar7 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
    *pcVar13 = *pcVar11;
    pcVar11 = pcVar11 + 1;
    pcVar13 = pcVar13 + 1;
  }
  if (param_2 == (char *)0x0) {
    param_2 = &DAT_10023758;
  }
  uVar7 = 0xffffffff;
  do {
    pcVar11 = param_2;
    if (uVar7 == 0) break;
    uVar7 = uVar7 - 1;
    pcVar11 = param_2 + 1;
    cVar1 = *param_2;
    param_2 = pcVar11;
  } while (cVar1 != '\0');
  uVar7 = ~uVar7;
  pcVar11 = pcVar11 + -uVar7;
  pcVar13 = (char *)((&DAT_10024c8c)[iVar9] + 0x10c);
  for (uVar8 = uVar7 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
    *(undefined4 *)pcVar13 = *(undefined4 *)pcVar11;
    pcVar11 = pcVar11 + 4;
    pcVar13 = pcVar13 + 4;
  }
  for (uVar7 = uVar7 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
    *pcVar13 = *pcVar11;
    pcVar11 = pcVar11 + 1;
    pcVar13 = pcVar13 + 1;
  }
  piVar3 = (int *)tioGetHandle();
  piVar5 = tioInitFile(param_1,5);
  if (piVar5 == (int *)0x0) {
    FUN_10002200((&DAT_10024c8c)[iVar9]);
    (&DAT_10024c8c)[iVar9] = 0;
    return 0;
  }
  tiohbReadChunk((int)piVar5,local_18);
  if (local_18[0] != 0x5210) {
    tioError((byte *)s_Wrong_chunk_ID_0x_08X__0x_08X_ex_1001f890);
    tiohTermHandle(piVar5);
    tioSetHandle(piVar3);
    return 0;
  }
  tiohbReadFloat((int)piVar5);
  *(int **)((&DAT_10024c8c)[iVar9] + 0x140) = piVar5;
  uVar7 = tiohGetErrorState((int)piVar5);
  do {
    if ((uVar7 != 0) || (uVar7 = tiohbTell((int)piVar5), local_14 <= uVar7)) {
      uVar7 = tiohGetErrorState((int)piVar5);
      if (uVar7 != 0) {
        tiohTermHandle(piVar5);
        FUN_10002200(*(undefined4 *)((&DAT_10024c8c)[iVar9] + 0x130));
        FUN_10002200(*(undefined4 *)((&DAT_10024c8c)[iVar9] + 0x148));
        FUN_10002200((&DAT_10024c8c)[iVar9]);
        (&DAT_10024c8c)[iVar9] = 0;
        tioSetHandle(piVar3);
        return 0;
      }
      tioSetHandle(piVar3);
      return (&DAT_10024c8c)[iVar9];
    }
    tiohbReadChunk((int)piVar5,(ushort *)&local_24);
    uVar7 = local_24 & 0xffff;
    if (uVar7 < 0x3013) {
      if (uVar7 == 0x3012) {
        uVar6 = tiohbReadDWord((int)piVar5);
        *(undefined4 *)((&DAT_10024c8c)[iVar9] + 0x14c) = uVar6;
        uVar6 = FUN_100021c0(1,*(int *)((&DAT_10024c8c)[iVar9] + 0x14c) + 10);
        *(undefined4 *)((&DAT_10024c8c)[iVar9] + 0x148) = uVar6;
        if (*(int *)((&DAT_10024c8c)[iVar9] + 0x148) == 0) {
          tioError((byte *)0x0);
        }
        else {
          *(undefined4 *)((&DAT_10024c8c)[iVar9] + 0x150) = 0;
        }
      }
      else if (uVar7 == 0x613) {
        *(int *)((&DAT_10024c8c)[iVar9] + 300) = local_1c;
        uVar6 = FUN_100021c0(*(undefined4 *)((&DAT_10024c8c)[iVar9] + 300),0x10);
        *(undefined4 *)((&DAT_10024c8c)[iVar9] + 0x130) = uVar6;
        if (*(int *)((&DAT_10024c8c)[iVar9] + 0x130) == 0) {
          tioError((byte *)0x0);
        }
        else {
          iVar12 = 0;
          uVar7 = tiohGetErrorState((int)piVar5);
          if (uVar7 == 0) {
            iVar10 = 0;
            do {
              uVar7 = tiohbTell((int)piVar5);
              if (local_20 <= uVar7) break;
              tiohbReadChunk((int)piVar5,local_c);
              if (local_c[0] != 0x220) {
                tioWarning((byte *)s_Wrong_chunk_ID_0x_08X__0x_08X_ex_1001f890);
                tiohbSeek((int)piVar5,local_20,0);
                break;
              }
              iVar2 = (&DAT_10024c8c)[iVar9];
              if (*(int *)(iVar2 + 300) <= iVar12) {
                tioError((byte *)s_Unexpected_chunk_ID_0x_08X__too_m_1001f838);
                break;
              }
              FUN_10006c80((int)local_c,iVar2,(int *)(*(int *)(iVar2 + 0x130) + iVar10));
              iVar12 = iVar12 + 1;
              iVar10 = iVar10 + 0x10;
              uVar7 = tiohGetErrorState((int)piVar5);
            } while (uVar7 == 0);
          }
        }
      }
      else {
        if (uVar7 != 0x630) goto LAB_10006050;
        if (local_1c != *(int *)((&DAT_10024c8c)[iVar9] + 300)) {
          tioWarning((byte *)s_Wrong_chunk_0x_08X_length___i____1001f864);
        }
        tiohbSeek((int)piVar5,local_20,0);
        iVar12 = (&DAT_10024c8c)[iVar9];
        *(uint *)(iVar12 + 0x134) = local_24;
        *(uint *)(iVar12 + 0x138) = local_20;
        *(int *)(iVar12 + 0x13c) = local_1c;
      }
    }
    else if (uVar7 == 0x4011) {
      tiohbReadString((int)piVar5,&DAT_1002a500,0x400);
    }
    else if (uVar7 == 0x4014) {
      tiohbReadString((int)piVar5,&DAT_1002a500,0x400);
      uVar7 = 0xffffffff;
      pcVar11 = &DAT_1002a500;
      do {
        pcVar13 = pcVar11;
        if (uVar7 == 0) break;
        uVar7 = uVar7 - 1;
        pcVar13 = pcVar11 + 1;
        cVar1 = *pcVar11;
        pcVar11 = pcVar13;
      } while (cVar1 != '\0');
      uVar7 = ~uVar7;
      pcVar11 = pcVar13 + -uVar7;
      pcVar13 = (char *)((&DAT_10024c8c)[iVar9] + 0x10c);
      for (uVar8 = uVar7 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
        *(undefined4 *)pcVar13 = *(undefined4 *)pcVar11;
        pcVar11 = pcVar11 + 4;
        pcVar13 = pcVar13 + 4;
      }
      for (uVar7 = uVar7 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
        *pcVar13 = *pcVar11;
        pcVar11 = pcVar11 + 1;
        pcVar13 = pcVar13 + 1;
      }
    }
    else {
LAB_10006050:
      tioWarning((byte *)s_Wrong_chunk_ID_0x_08X_1001f820);
      tiohbSeek((int)piVar5,local_20,0);
    }
    uVar7 = tiohGetErrorState((int)piVar5);
  } while( true );
}



