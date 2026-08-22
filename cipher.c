/* ===== FUN_10001590 @ 10001590  size=1025 ===== */

undefined4 __cdecl FUN_10001590(uint *param_1,FILE *param_2)

{
  int *piVar1;
  byte *pbVar2;
  bool bVar3;
  uint *puVar4;
  FILE *pFVar5;
  FILE *pFVar6;
  uint uVar7;
  uint uVar8;
  undefined1 *puVar9;
  uint *puVar10;
  int iVar11;
  int iVar12;
  bool bVar13;
  uint local_1c;
  int local_18;
  int local_10;
  undefined4 local_c;
  uint local_8;
  int local_4;
  
  pFVar5 = param_2;
  puVar4 = param_1;
  local_1c = 0;
  bVar3 = false;
  bVar13 = false;
  while( true ) {
    local_4 = FUN_10011f05((char *)pFVar5);
    uVar7 = FUN_10011dee((char *)&local_c,4,1,(int *)pFVar5);
    uVar8 = FUN_10011dee((char *)&param_2,4,1,(int *)pFVar5);
    if ((uVar7 != 1) || (uVar8 != 1)) break;
    FUN_10001000(5,(byte *)s_Load_table__entry__08X___i_1001f1f8);
    pFVar6 = param_2;
    switch(local_c) {
    case 0x43525343:
      if (param_2 < (FILE *)0x10) {
LAB_10001693:
        bVar3 = true;
      }
      else {
        uVar7 = FUN_10011dee((char *)(puVar4 + 0x11),4,1,(int *)pFVar5);
        uVar8 = FUN_10011dee((char *)(puVar4 + 0x12),4,1,(int *)pFVar5);
        if ((uVar7 != 1) || (uVar8 != 1)) goto LAB_10001693;
      }
      local_1c = puVar4[0x11];
      break;
    case 0x43525344:
      if ((FILE *)0xc < param_2) {
        puVar10 = puVar4 + 0x13;
        uVar7 = FUN_10011dee((char *)puVar10,4,1,(int *)pFVar5);
        FUN_10001000(5,(byte *)s_Load_table__n_data__i_1001f1c0);
        if (uVar7 == 1) {
          iVar11 = 0;
          if (*puVar10 != 0) {
            puVar9 = (undefined1 *)(*puVar10 * ((-(uint)bVar13 & 4) + 6));
            if (puVar9 <= &param_2[-1]._charbuf) {
              uVar7 = FUN_100021c0(1,(int)&param_2[-1]._charbuf - (int)puVar9);
              puVar4[0x15] = uVar7;
              uVar7 = FUN_100021c0(*puVar10,0xc);
              puVar4[0x14] = uVar7;
              if ((puVar4[0x15] == 0) || (uVar7 == 0)) {
                bVar3 = true;
                FUN_10001000(5,(byte *)s_Table_items_malloc_failed_1001f184);
              }
              else {
                local_10 = 0;
                local_18 = 0;
                if (!bVar3) {
                  while (local_18 < (int)puVar4[0x13]) {
                    uVar7 = FUN_10011dee((char *)(iVar11 + puVar4[0x14]),4,1,(int *)pFVar5);
                    if (bVar13) {
                      local_8 = FUN_10011dee((char *)(iVar11 + 4 + puVar4[0x14]),4,1,(int *)pFVar5);
                    }
                    else {
                      local_8 = 1;
                    }
                    uVar8 = FUN_10011dee((char *)&param_1,2,1,(int *)pFVar5);
                    if (((uVar7 != 1) || (uVar8 != 1)) || (local_8 != 1)) break;
                    *(uint *)(iVar11 + 8 + puVar4[0x14]) = puVar4[0x15] + local_10;
                    uVar7 = FUN_10011dee(*(char **)(iVar11 + 8 + puVar4[0x14]),1,
                                         (uint)param_1 & 0xffff,(int *)pFVar5);
                    if (uVar7 < ((uint)param_1 & 0xffff)) {
                      bVar3 = true;
                      FUN_10001000(5,(byte *)s_Table_items_reader__i_failed_1001f1a0);
                    }
                    iVar12 = 0;
                    if ((short)param_1 != 0) {
                      do {
                        pbVar2 = (byte *)(*(int *)(iVar11 + 8 + puVar4[0x14]) + iVar12);
                        *pbVar2 = *pbVar2 ^ (byte)local_1c;
                        local_1c = local_1c + puVar4[0x12];
                        iVar12 = iVar12 + 1;
                      } while (iVar12 < (int)((uint)param_1 & 0xffff));
                    }
                    if (*(char *)(*(int *)(iVar11 + 8 + puVar4[0x14]) + -1 +
                                 ((uint)param_1 & 0xffff)) != '\0') {
                      bVar3 = true;
                    }
                    local_10 = local_10 + ((uint)param_1 & 0xffff);
                    local_18 = local_18 + 1;
                    iVar11 = iVar11 + 0xc;
                    if (bVar3) break;
                  }
                }
              }
              break;
            }
          }
        }
      }
      goto LAB_100016fd;
    case 0x43525348:
      if ((param_2 < (FILE *)0xc) ||
         (uVar7 = FUN_10011dee((char *)puVar4,4,1,(int *)pFVar5), uVar7 != 1)) {
        bVar3 = true;
      }
      bVar13 = (*puVar4 & 0xffff0000) != 0x10000;
      break;
    case 0x4352534c:
      if (((FILE *)0x8 < param_2) && (param_2 < (FILE *)0x49)) {
        piVar1 = &param_2[-1]._bufsiz;
        puVar10 = puVar4 + 1;
        puVar9 = (undefined1 *)FUN_10011dee((char *)puVar10,1,(uint)piVar1,(int *)pFVar5);
        if ((int *)puVar9 != piVar1) {
          bVar3 = true;
        }
        if (0 < (int)piVar1) {
          do {
            *(byte *)puVar10 = (byte)*puVar10 ^ (byte)local_1c;
            local_1c = local_1c + puVar4[0x12];
            puVar10 = (uint *)((int)puVar10 + 1);
          } while ((int)((-4 - (int)puVar4) + (int)puVar10) < (int)piVar1);
        }
        if (((undefined1 *)((int)&pFVar6[-1]._bufsiz + 3))[(int)puVar4] == '\0') break;
      }
LAB_100016fd:
      bVar3 = true;
    }
    FUN_10001000(5,(byte *)s_Load_table__entry_passed__i_1001f1d8);
    iVar11 = FUN_10011d35((int *)pFVar5,(int)&param_2->_ptr + local_4,0);
    if ((iVar11 != 0) || (bVar3)) break;
  }
  FUN_100119d1(pFVar5);
  if ((!bVar3) && (((*puVar4 != 0 && ((char)puVar4[1] != '\0')) && (puVar4[0x13] != 0)))) {
    return 1;
  }
  return 0;
}



/* ===== FUN_10001590 @ 10001590  size=1025 ===== */

undefined4 __cdecl FUN_10001590(uint *param_1,FILE *param_2)

{
  int *piVar1;
  byte *pbVar2;
  bool bVar3;
  uint *puVar4;
  FILE *pFVar5;
  FILE *pFVar6;
  uint uVar7;
  uint uVar8;
  undefined1 *puVar9;
  uint *puVar10;
  int iVar11;
  int iVar12;
  bool bVar13;
  uint local_1c;
  int local_18;
  int local_10;
  undefined4 local_c;
  uint local_8;
  int local_4;
  
  pFVar5 = param_2;
  puVar4 = param_1;
  local_1c = 0;
  bVar3 = false;
  bVar13 = false;
  while( true ) {
    local_4 = FUN_10011f05((char *)pFVar5);
    uVar7 = FUN_10011dee((char *)&local_c,4,1,(int *)pFVar5);
    uVar8 = FUN_10011dee((char *)&param_2,4,1,(int *)pFVar5);
    if ((uVar7 != 1) || (uVar8 != 1)) break;
    FUN_10001000(5,(byte *)s_Load_table__entry__08X___i_1001f1f8);
    pFVar6 = param_2;
    switch(local_c) {
    case 0x43525343:
      if (param_2 < (FILE *)0x10) {
LAB_10001693:
        bVar3 = true;
      }
      else {
        uVar7 = FUN_10011dee((char *)(puVar4 + 0x11),4,1,(int *)pFVar5);
        uVar8 = FUN_10011dee((char *)(puVar4 + 0x12),4,1,(int *)pFVar5);
        if ((uVar7 != 1) || (uVar8 != 1)) goto LAB_10001693;
      }
      local_1c = puVar4[0x11];
      break;
    case 0x43525344:
      if ((FILE *)0xc < param_2) {
        puVar10 = puVar4 + 0x13;
        uVar7 = FUN_10011dee((char *)puVar10,4,1,(int *)pFVar5);
        FUN_10001000(5,(byte *)s_Load_table__n_data__i_1001f1c0);
        if (uVar7 == 1) {
          iVar11 = 0;
          if (*puVar10 != 0) {
            puVar9 = (undefined1 *)(*puVar10 * ((-(uint)bVar13 & 4) + 6));
            if (puVar9 <= &param_2[-1]._charbuf) {
              uVar7 = FUN_100021c0(1,(int)&param_2[-1]._charbuf - (int)puVar9);
              puVar4[0x15] = uVar7;
              uVar7 = FUN_100021c0(*puVar10,0xc);
              puVar4[0x14] = uVar7;
              if ((puVar4[0x15] == 0) || (uVar7 == 0)) {
                bVar3 = true;
                FUN_10001000(5,(byte *)s_Table_items_malloc_failed_1001f184);
              }
              else {
                local_10 = 0;
                local_18 = 0;
                if (!bVar3) {
                  while (local_18 < (int)puVar4[0x13]) {
                    uVar7 = FUN_10011dee((char *)(iVar11 + puVar4[0x14]),4,1,(int *)pFVar5);
                    if (bVar13) {
                      local_8 = FUN_10011dee((char *)(iVar11 + 4 + puVar4[0x14]),4,1,(int *)pFVar5);
                    }
                    else {
                      local_8 = 1;
                    }
                    uVar8 = FUN_10011dee((char *)&param_1,2,1,(int *)pFVar5);
                    if (((uVar7 != 1) || (uVar8 != 1)) || (local_8 != 1)) break;
                    *(uint *)(iVar11 + 8 + puVar4[0x14]) = puVar4[0x15] + local_10;
                    uVar7 = FUN_10011dee(*(char **)(iVar11 + 8 + puVar4[0x14]),1,
                                         (uint)param_1 & 0xffff,(int *)pFVar5);
                    if (uVar7 < ((uint)param_1 & 0xffff)) {
                      bVar3 = true;
                      FUN_10001000(5,(byte *)s_Table_items_reader__i_failed_1001f1a0);
                    }
                    iVar12 = 0;
                    if ((short)param_1 != 0) {
                      do {
                        pbVar2 = (byte *)(*(int *)(iVar11 + 8 + puVar4[0x14]) + iVar12);
                        *pbVar2 = *pbVar2 ^ (byte)local_1c;
                        local_1c = local_1c + puVar4[0x12];
                        iVar12 = iVar12 + 1;
                      } while (iVar12 < (int)((uint)param_1 & 0xffff));
                    }
                    if (*(char *)(*(int *)(iVar11 + 8 + puVar4[0x14]) + -1 +
                                 ((uint)param_1 & 0xffff)) != '\0') {
                      bVar3 = true;
                    }
                    local_10 = local_10 + ((uint)param_1 & 0xffff);
                    local_18 = local_18 + 1;
                    iVar11 = iVar11 + 0xc;
                    if (bVar3) break;
                  }
                }
              }
              break;
            }
          }
        }
      }
      goto LAB_100016fd;
    case 0x43525348:
      if ((param_2 < (FILE *)0xc) ||
         (uVar7 = FUN_10011dee((char *)puVar4,4,1,(int *)pFVar5), uVar7 != 1)) {
        bVar3 = true;
      }
      bVar13 = (*puVar4 & 0xffff0000) != 0x10000;
      break;
    case 0x4352534c:
      if (((FILE *)0x8 < param_2) && (param_2 < (FILE *)0x49)) {
        piVar1 = &param_2[-1]._bufsiz;
        puVar10 = puVar4 + 1;
        puVar9 = (undefined1 *)FUN_10011dee((char *)puVar10,1,(uint)piVar1,(int *)pFVar5);
        if ((int *)puVar9 != piVar1) {
          bVar3 = true;
        }
        if (0 < (int)piVar1) {
          do {
            *(byte *)puVar10 = (byte)*puVar10 ^ (byte)local_1c;
            local_1c = local_1c + puVar4[0x12];
            puVar10 = (uint *)((int)puVar10 + 1);
          } while ((int)((-4 - (int)puVar4) + (int)puVar10) < (int)piVar1);
        }
        if (((undefined1 *)((int)&pFVar6[-1]._bufsiz + 3))[(int)puVar4] == '\0') break;
      }
LAB_100016fd:
      bVar3 = true;
    }
    FUN_10001000(5,(byte *)s_Load_table__entry_passed__i_1001f1d8);
    iVar11 = FUN_10011d35((int *)pFVar5,(int)&param_2->_ptr + local_4,0);
    if ((iVar11 != 0) || (bVar3)) break;
  }
  FUN_100119d1(pFVar5);
  if ((!bVar3) && (((*puVar4 != 0 && ((char)puVar4[1] != '\0')) && (puVar4[0x13] != 0)))) {
    return 1;
  }
  return 0;
}



