/* ===== csiLoadHeader @ 00445140  size=224 ===== */

/* int __stdcall csiLoadHeader(char *,struct csiHEADER *) */

int csiLoadHeader(char *param_1,csiHEADER *param_2)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 uVar3;
  
                    /* 0x45140  24  ?csiLoadHeader@@YGHPADPAUcsiHEADER@@@Z */
  uVar1 = tioGetHandle();
  if (param_1 == (char *)0x0) {
    DAT_0057d980 = 3;
    return 0;
  }
  if (param_2 == (csiHEADER *)0x0) {
    DAT_0057d980 = 6;
    return 0;
  }
  iVar2 = tioInitFile(param_1,5);
  if (iVar2 != 0) {
    iVar2 = FUN_004452c0(param_2);
    if (iVar2 == 0) {
      uVar3 = tioGetHandle();
      tiohTermHandle(uVar3);
      tioSetHandle(uVar1);
      DAT_0057d980 = 4;
      return 0;
    }
    uVar3 = tioGetHandle();
    tiohTermHandle(uVar3);
    tioSetHandle(uVar1);
    DAT_0057d980 = 0;
    return 1;
  }
  if (DAT_0057d980 != 1) {
    if (DAT_0057d980 != 2) {
      DAT_0057d980 = 6;
      return 0;
    }
    DAT_0057d980 = 3;
    return 0;
  }
  DAT_0057d980 = 2;
  return 0;
}



/* ===== csiLoadPicture @ 00444c50  size=826 ===== */

/* struct csiPICTURE * __stdcall csiLoadPicture(char *) */

csiPICTURE * csiLoadPicture(char *param_1)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  csiPICTURE *pcVar6;
  csiPICTURE *pcVar7;
  int *piVar8;
  uint uVar9;
  uint uVar10;
  int local_a0 [4];
  int local_90;
  int local_8c;
  undefined4 local_88;
  uint local_84;
  
                    /* 0x44c50  26  ?csiLoadPicture@@YGPAUcsiPICTURE@@PAD@Z */
  uVar1 = tioGetHandle();
  if ((param_1 != (char *)0x0) && (iVar2 = tioInitFile(param_1,5), iVar2 == 0)) {
    if (DAT_0057d980 == 1) {
      DAT_0057d980 = 2;
      return (csiPICTURE *)0x0;
    }
    if (DAT_0057d980 != 2) {
      DAT_0057d980 = 6;
      return (csiPICTURE *)0x0;
    }
    DAT_0057d980 = 3;
    return (csiPICTURE *)0x0;
  }
  uVar3 = tioGetHandle();
  uVar3 = tiohGetFileSize(uVar3);
  iVar2 = FUN_00402ab0(1,uVar3);
  if (iVar2 == 0) {
    uVar1 = tioGetHandle();
    tiohTermHandle(uVar1);
    return (csiPICTURE *)0x0;
  }
  uVar4 = tioGetHandle(iVar2,1,uVar3);
  tiohbRead(uVar4);
  uVar4 = tioGetHandle();
  iVar5 = tiohGetErrorState(uVar4);
  uVar4 = tioGetHandle();
  tiohTermHandle(uVar4);
  if ((iVar5 == 0) && (iVar5 = tioInitMemory(iVar2,uVar3,5), iVar5 != 0)) {
    iVar5 = FUN_004452c0(local_a0);
    if (iVar5 == 0) {
      if (param_1 != (char *)0x0) {
        uVar3 = tioGetHandle();
        tiohTermHandle(uVar3);
        tioSetHandle(uVar1);
      }
      FUN_00402ad0(iVar2);
      DAT_0057d980 = 4;
      return (csiPICTURE *)0x0;
    }
    pcVar6 = (csiPICTURE *)FUN_00402ab0(1,0xd0);
    if (pcVar6 == (csiPICTURE *)0x0) {
      if (param_1 != (char *)0x0) {
        uVar3 = tioGetHandle();
        tiohTermHandle(uVar3);
        tioSetHandle(uVar1);
      }
      FUN_00402ad0(iVar2);
      DAT_0057d980 = 2;
      return (csiPICTURE *)0x0;
    }
    piVar8 = local_a0;
    pcVar7 = pcVar6;
    for (iVar5 = 0x28; iVar5 != 0; iVar5 = iVar5 + -1) {
      *(int *)pcVar7 = *piVar8;
      piVar8 = piVar8 + 1;
      pcVar7 = pcVar7 + 4;
    }
    local_a0[3] = local_a0[3] - local_a0[2];
    *(int *)(pcVar6 + 0xa0) = local_a0[3];
    if (local_a0[3] != 0) {
      iVar5 = FUN_00402ab0(1,local_a0[3]);
      *(int *)(pcVar6 + 0xa4) = iVar5;
      if (iVar5 == 0) {
        FUN_00402ad0(pcVar6);
        if (param_1 != (char *)0x0) {
          uVar3 = tioGetHandle();
          tiohTermHandle(uVar3);
          tioSetHandle(uVar1);
        }
        DAT_0057d980 = 2;
        goto LAB_00444e56;
      }
      uVar3 = tioGetHandle(iVar5,*(undefined4 *)(pcVar6 + 0xa0),1);
      tiohbRead(uVar3);
    }
    uVar9 = local_8c * local_90;
    uVar10 = 0;
    if (local_84 != 0) {
      pcVar7 = pcVar6 + 0xa8;
      do {
        iVar5 = FUN_00402ab0(local_88,uVar9);
        *(int *)pcVar7 = iVar5;
        if (iVar5 == 0) {
          FUN_00402ad0(*(undefined4 *)(pcVar6 + 0xa4));
          pcVar6 = pcVar6 + uVar10 * 4 + 0xa4;
          do {
            FUN_00402ad0(*(undefined4 *)pcVar6);
            pcVar6 = pcVar6 + -4;
          } while( true );
        }
        uVar3 = tioGetHandle(iVar5,local_88,uVar9);
        tiohbRead(uVar3);
        uVar9 = uVar9 >> 2;
        uVar10 = uVar10 + 1;
        pcVar7 = pcVar7 + 4;
      } while (uVar10 < local_84);
    }
    uVar3 = tioGetHandle();
    iVar5 = tiohGetErrorState(uVar3);
    if (iVar5 == 0) {
      if (param_1 != (char *)0x0) {
        uVar3 = tioGetHandle();
        tiohTermHandle(uVar3);
        tioSetHandle(uVar1);
      }
      DAT_0057d980 = 0;
      FUN_00402ad0(iVar2);
      return pcVar6;
    }
    if (param_1 != (char *)0x0) {
      uVar3 = tioGetHandle();
      tiohTermHandle(uVar3);
      tioSetHandle(uVar1);
    }
    csiDestroyImage(pcVar6);
    DAT_0057d980 = 1;
  }
LAB_00444e56:
  FUN_00402ad0(iVar2);
  return (csiPICTURE *)0x0;
}



/* ===== csiCSI2Image @ 00442660  size=177 ===== */

/* struct imIMAGE * __stdcall csiCSI2Image(struct csiPICTURE *) */

imIMAGE * csiCSI2Image(csiPICTURE *param_1)

{
  csiPICTURE *pcVar1;
  imIMAGE *piVar2;
  imIMAGE *piVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  bool bVar7;
  
                    /* 0x42660  4  ?csiCSI2Image@@YGPAUimIMAGE@@PAUcsiPICTURE@@@Z */
  pcVar1 = param_1;
  piVar2 = imCreateImage(*(int *)(param_1 + 0x10),*(int *)(param_1 + 0x14));
  if (piVar2 == (imIMAGE *)0x0) {
    return (imIMAGE *)0x0;
  }
  *(undefined4 *)(piVar2 + 0x1c) = 0;
  if (*(int *)(param_1 + 0x1c) != 1) {
    param_1 = (csiPICTURE *)(piVar2 + 0x20);
    uVar5 = 1;
    do {
      uVar6 = 1 << ((byte)uVar5 & 0x1f);
      piVar3 = imCreateImage(*(uint *)(pcVar1 + 0x10) / uVar6,*(uint *)(pcVar1 + 0x14) / uVar6);
      *(imIMAGE **)param_1 = piVar3;
      if (piVar3 == (imIMAGE *)0x0) {
        imDestroyImage(piVar2);
        return (imIMAGE *)0x0;
      }
      param_1 = param_1 + 4;
      *(int *)(piVar2 + 0x1c) = *(int *)(piVar2 + 0x1c) + 1;
      bVar7 = uVar5 < *(int *)(pcVar1 + 0x1c) - 1U;
      uVar5 = uVar5 + 1;
    } while (bVar7);
  }
  iVar4 = csiCSICopyToImage(pcVar1,piVar2);
  if (iVar4 == 0) {
    return (imIMAGE *)0x0;
  }
  csiFormat2Image((csiHEADER *)pcVar1,(imFORMAT *)(piVar2 + 0xc));
  return piVar2;
}



/* ===== csiGetARGBFormat @ 00441a80  size=358 ===== */

/* int __stdcall csiGetARGBFormat(struct csiHEADER *,unsigned long *,unsigned long *,unsigned long
   *,unsigned long *,unsigned long *) */

int csiGetARGBFormat(csiHEADER *param_1,ulong *param_2,ulong *param_3,ulong *param_4,ulong *param_5,
                    ulong *param_6)

{
  ushort uVar1;
  ulong uVar2;
  csiHEADER *pcVar3;
  uint uVar4;
  
                    /* 0x41a80  15  ?csiGetARGBFormat@@YGHPAUcsiHEADER@@PAK1111@Z */
  *param_6 = 0;
  *param_5 = 0;
  *param_4 = 0;
  *param_3 = 0;
  *param_2 = 0;
  uVar4 = 0;
  if (*(int *)(param_1 + 0x20) != 0) {
    pcVar3 = param_1 + 0x2c;
    do {
      switch(param_1[uVar4 + 0x24]) {
      case (csiHEADER)0x41:
        uVar1 = *(ushort *)pcVar3;
        if (uVar1 == 0xffff) {
          *param_2 = 0;
        }
        else {
          *param_2 = ((uint)(uVar1 >> 8) - (uVar1 & 0xff)) + 1;
        }
        break;
      case (csiHEADER)0x42:
        uVar1 = *(ushort *)pcVar3;
        if (uVar1 == 0xffff) {
          *param_5 = 0;
        }
        else {
          *param_5 = ((uint)(uVar1 >> 8) - (uVar1 & 0xff)) + 1;
        }
        break;
      default:
        return 0;
      case (csiHEADER)0x47:
        uVar1 = *(ushort *)pcVar3;
        if (uVar1 == 0xffff) {
          *param_4 = 0;
        }
        else {
          *param_4 = ((uint)(uVar1 >> 8) - (uVar1 & 0xff)) + 1;
        }
        break;
      case (csiHEADER)0x49:
        uVar1 = *(ushort *)pcVar3;
        if (uVar1 == 0xffff) {
          uVar2 = 0;
        }
        else {
          uVar2 = ((uint)(uVar1 >> 8) - (uVar1 & 0xff)) + 1;
        }
        *param_6 = uVar2;
        break;
      case (csiHEADER)0x52:
        uVar1 = *(ushort *)pcVar3;
        if (uVar1 == 0xffff) {
          *param_3 = 0;
        }
        else {
          *param_3 = ((uint)(uVar1 >> 8) - (uVar1 & 0xff)) + 1;
        }
      }
      uVar4 = uVar4 + 1;
      pcVar3 = pcVar3 + 2;
    } while (uVar4 < *(uint *)(param_1 + 0x20));
  }
  return 1;
}



/* ===== csiHeaderApplyFormat @ 00441cb0  size=139 ===== */

/* void __stdcall csiHeaderApplyFormat(struct csiHEADER *,int) */

void csiHeaderApplyFormat(csiHEADER *param_1,int param_2)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  
                    /* 0x41cb0  19  ?csiHeaderApplyFormat@@YGXPAUcsiHEADER@@H@Z */
  if (((param_1 != (csiHEADER *)0x0) && (-1 < param_2)) && (param_2 < 0xb)) {
    iVar3 = 0;
    piVar1 = &DAT_00561188;
    do {
      if (*piVar1 == param_2) break;
      piVar1 = piVar1 + 9;
      iVar3 = iVar3 + 1;
    } while ((int)piVar1 < 0x561314);
    if (iVar3 != 0xb) {
      *(undefined4 *)param_1 = 0x4353494d;
      iVar2 = iVar3 * 0x24;
      *(undefined4 *)(param_1 + 0x18) = (&DAT_0056118c)[iVar3 * 9];
      *(undefined4 *)(param_1 + 0x20) = (&DAT_00561190)[iVar3 * 9];
      *(undefined4 *)(param_1 + 8) = 0xa0;
      *(undefined4 *)(param_1 + 0x24) = *(undefined4 *)(&DAT_00561194 + iVar2);
      *(undefined4 *)(param_1 + 0x28) =
           *(undefined4 *)((int)&PTR_LAB_00424751_1_00561195 + iVar2 + 3);
      *(undefined4 *)(param_1 + 0x2c) = *(undefined4 *)(&DAT_0056119c + iVar3 * 0x12);
      *(undefined4 *)(param_1 + 0x30) = *(undefined4 *)(iVar2 + 0x5611a0);
      *(undefined4 *)(param_1 + 0x34) = *(undefined4 *)(iVar2 + 0x5611a4);
      *(undefined4 *)(param_1 + 0x38) = *(undefined4 *)(iVar2 + 0x5611a8);
    }
  }
  return;
}



/* ===== csiGetRequestMem @ 00441c20  size=108 ===== */

/* int __stdcall csiGetRequestMem(struct csiHEADER *,int) */

int csiGetRequestMem(csiHEADER *param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
                    /* 0x41c20  18  ?csiGetRequestMem@@YGHPAUcsiHEADER@@H@Z */
  if ((param_1 == (csiHEADER *)0x0) || (*(int *)param_1 != 0x4353494d)) {
switchD_00441c53_caseD_3:
  }
  else {
    iVar1 = 0;
    uVar2 = *(int *)(param_1 + 0x18) * *(int *)(param_1 + 0x14) * *(int *)(param_1 + 0x10);
    uVar4 = 0;
    if (*(uint *)(param_1 + 0x1c) != 0) {
      do {
        switch(param_2) {
        case 1:
          uVar3 = uVar2;
          break;
        case 2:
          uVar3 = uVar2 + 1 & 0xfffffffe;
          break;
        default:
          goto switchD_00441c53_caseD_3;
        case 4:
          uVar3 = uVar2 + 3 & 0xfffffffc;
          break;
        case 8:
          uVar3 = uVar2 + 7 & 0xfffffff8;
        }
        iVar1 = iVar1 + uVar3;
        uVar2 = uVar2 >> 2;
        uVar4 = uVar4 + 1;
        if (*(uint *)(param_1 + 0x1c) <= uVar4) {
          return iVar1;
        }
      } while( true );
    }
  }
  return 0;
}



