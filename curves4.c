/* ===== FUN_0049d8d0 @ 0049d8d0  size=333  callers: FUN_0048a250  ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_0049d8d0(void)

{
  int iVar1;
  int local_10;
  undefined4 local_c;
  float local_8;
  undefined4 local_4;
  
  DAT_014972cc = DAT_014972cc + 1;
  if (DAT_014972cc < 2) {
    DAT_014972c8 = FUN_0040ef60(0x511,0,0);
    if (DAT_014972c8 == (int *)0x0) {
      return 0;
    }
    local_c = 0;
    local_4 = 0;
    local_10 = 0;
    do {
      local_8 = (float)local_10;
      iVar1 = FUN_0040f320(DAT_014972c8,0,&local_c);
      if (iVar1 == -1) {
        FUN_0040f010((undefined *)DAT_014972c8);
        return 0;
      }
      local_10 = local_10 + 1;
    } while (local_10 < 4);
    _DAT_01493404 = &LAB_0049e0a0;
    _DAT_01493414 = &LAB_0049e0a0;
    _DAT_014933f0 = 0xc;
    _DAT_014933f4 = FUN_0049dc00;
    _DAT_014933f8 = FUN_0049dea0;
    _DAT_014933fc = &LAB_0049e560;
    _DAT_01493400 = 4;
    _DAT_01493408 = &LAB_0049dca0;
    _DAT_0149340c = &LAB_0049e560;
    _DAT_01493410 = 4;
    _DAT_01493418 = &LAB_0049e3c0;
    _DAT_0149341c = &LAB_0049e560;
    _DAT_01493420 = 0x10;
    _DAT_01493424 = &LAB_0049e0f0;
    _DAT_01493428 = &LAB_0049e210;
    _DAT_0149342c = &LAB_0049e560;
    _DAT_01493430 = 0x10;
    _DAT_01493434 = &LAB_0049e180;
    _DAT_01493438 = &LAB_0049e650;
    _DAT_0149343c = &LAB_0049e560;
  }
  return 1;
}


/* ===== FUN_0042e2e0 @ 0042e2e0  size=902  callers:  ===== */

undefined4 __cdecl FUN_0042e2e0(int param_1,int param_2)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  uint uVar4;
  undefined4 uVar5;
  int iVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  ushort *puVar9;
  undefined4 uVar10;
  undefined4 local_7c;
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_54;
  undefined4 local_50;
  ushort local_4c [6];
  undefined4 local_40 [4];
  undefined4 local_30;
  undefined4 local_2c [4];
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  uVar4 = *(uint *)(param_1 + 0x10);
  if ((uVar4 & 1) == 0) {
    if ((uVar4 & 2) == 0) {
      if ((uVar4 & 4) == 0) {
        if ((uVar4 & 8) == 0) {
          if ((uVar4 & 0x10) == 0) {
            if ((uVar4 & 0x20) == 0) {
              if ((uVar4 & 0x40) == 0) {
                if ((uVar4 & 0x80) == 0) {
                  if ((uVar4 & 0x100) == 0) {
                    if ((uVar4 & 0x200) == 0) {
                      if ((uVar4 & 0x400) == 0) {
                        if ((uVar4 & 0x800) == 0) {
                          if ((uVar4 & 0x1000) == 0) {
                            if ((uVar4 & 0x100000) == 0) {
                              *(undefined4 *)(param_1 + 0x88) = 0;
                              tioWarning((byte *)s_Unknown_old_spline_states__remov_0055fea8);
                              uVar5 = *(undefined4 *)(param_2 + 4);
                              uVar10 = 0;
                              uVar4 = tioGetHandle();
                              tiohbSeek(uVar4,uVar5,uVar10);
                              return 1;
                            }
                            *(undefined4 *)(param_1 + 0x88) = 0x54524556;
                          }
                          else {
                            *(undefined4 *)(param_1 + 0x88) = 0x53434c54;
                          }
                        }
                        else {
                          *(undefined4 *)(param_1 + 0x88) = 0x524f5454;
                        }
                      }
                      else {
                        *(undefined4 *)(param_1 + 0x88) = 0x504f5354;
                      }
                    }
                    else {
                      *(undefined4 *)(param_1 + 0x88) = 0x4c54494d;
                    }
                  }
                  else {
                    *(undefined4 *)(param_1 + 0x88) = 0x53434c5a;
                  }
                }
                else {
                  *(undefined4 *)(param_1 + 0x88) = 0x53434c59;
                }
              }
              else {
                *(undefined4 *)(param_1 + 0x88) = 0x53434c58;
              }
            }
            else {
              *(undefined4 *)(param_1 + 0x88) = 0x524f545a;
            }
          }
          else {
            *(undefined4 *)(param_1 + 0x88) = 0x524f5459;
          }
        }
        else {
          *(undefined4 *)(param_1 + 0x88) = 0x524f5458;
        }
      }
      else {
        *(undefined4 *)(param_1 + 0x88) = 0x54524e5a;
      }
    }
    else {
      *(undefined4 *)(param_1 + 0x88) = 0x54524e59;
    }
  }
  else {
    *(undefined4 *)(param_1 + 0x88) = 0x54524e58;
  }
  *(undefined4 *)(param_1 + 0x10) = 0;
  iVar3 = *(int *)(*(int *)(param_1 + 0x8c) + 4);
  iVar6 = *(int *)(*(int *)(param_1 + 0x8c) + 8);
  iVar1 = modpSplGetType(param_1);
  if (iVar1 != 0) {
    FUN_0040f010(*(undefined **)(param_1 + 0x8c));
    piVar2 = FUN_0040ef60(iVar1,iVar3,iVar6);
    *(int **)(param_1 + 0x8c) = piVar2;
    if ((piVar2 != (int *)0x0) && (iVar3 = FUN_0040f040((int)piVar2,*(int *)(param_2 + 8)), iVar3 != 0)) {
      iVar3 = 0;
      if (*(int *)(param_2 + 8) < 1) {
        return 1;
      }
      do {
        puVar9 = local_4c;
        uVar4 = tioGetHandle();
        tiohbReadChunk(uVar4,puVar9);
        if (local_4c[0] != 0x805b) {
          tioError((byte *)s_Wrong_MODB_SPL_KEYENTRY_chunk_de_0055fe2c);
          return 0;
        }
        puVar7 = &local_7c;
        uVar4 = tioGetHandle();
        tiohbReadPoint3D(uVar4,puVar7);
        puVar7 = &local_70;
        uVar4 = tioGetHandle();
        tiohbReadPoint3D(uVar4,puVar7);
        puVar7 = &local_64;
        uVar4 = tioGetHandle();
        tiohbReadPoint3D(uVar4,puVar7);
        uVar4 = tioGetHandle();
        uVar5 = tiohbReadDWord(uVar4);
        piVar2 = *(int **)(param_1 + 0x8c);
        iVar6 = *piVar2;
        if (iVar6 == 0x111) {
          local_54 = local_7c;
          local_50 = local_78;
          puVar7 = (undefined4 *)(piVar2[4] * iVar3 + piVar2[5]);
          *puVar7 = uVar5;
          puVar7[1] = local_7c;
          puVar7[2] = local_78;
        }
        else if (iVar6 == 0x311) {
          local_40[0] = uVar5;
          local_40[1] = local_7c;
          local_40[3] = local_70;
          local_40[2] = local_78;
          local_30 = local_64;
          puVar7 = local_40;
          puVar8 = (undefined4 *)(piVar2[4] * iVar3 + piVar2[5]);
          for (iVar6 = 5; iVar6 != 0; iVar6 = iVar6 + -1) {
            *puVar8 = *puVar7;
            puVar7 = puVar7 + 1;
            puVar8 = puVar8 + 1;
          }
        }
        else if (iVar6 == 0x431) {
          local_2c[0] = uVar5;
          local_2c[3] = local_78;
          local_2c[2] = local_7c;
          local_18 = local_70;
          local_1c = local_74;
          local_10 = local_68;
          local_14 = local_6c;
          local_8 = local_60;
          local_c = local_64;
          local_4 = local_5c;
          puVar7 = local_2c;
          puVar8 = (undefined4 *)(piVar2[4] * iVar3 + piVar2[5]);
          for (iVar6 = 0xb; iVar6 != 0; iVar6 = iVar6 + -1) {
            *puVar8 = *puVar7;
            puVar7 = puVar7 + 1;
            puVar8 = puVar8 + 1;
          }
        }
        iVar3 = iVar3 + 1;
      } while (iVar3 < *(int *)(param_2 + 8));
      return 1;
    }
    tioError((byte *)s_Not_enough_memory_0055f06c);
    return 0;
  }
  tioError((byte *)s_Wrong_spline_type___i_0055fed4);
  return 0;
}


