/* ===== FUN_10009cd0 @ 10009cd0  size=275 ===== */

undefined4 * __cdecl FUN_10009cd0(byte param_1)

{
  undefined4 *puVar1;
  char *pcVar2;
  undefined4 uVar3;
  int iVar4;
  undefined4 *puVar5;
  
  puVar1 = (undefined4 *)FUN_100021c0(0x504,1);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = 0x5458494f;
    if ((param_1 & 4) == 0) {
      *(undefined1 *)(puVar1 + 7) = 10;
      pcVar2 = FUN_10002270(&DAT_1001fb8c);
      puVar1[9] = pcVar2;
      uVar3 = FUN_100021c0(1,0x1000);
      puVar1[0x8d] = uVar3;
      uVar3 = tioVListCreate();
      puVar1[5] = uVar3;
      uVar3 = tioVListCreate();
      puVar1[0x115] = uVar3;
      if (DAT_1002b200 != 0) {
        tioVListAddVar((int *)puVar1[5],s___TEXTIO___1001fb80,1,1);
      }
    }
    else {
      *(undefined1 *)(puVar1 + 7) = 0;
    }
    puVar5 = puVar1 + 10;
    for (iVar4 = 0x40; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0xffffffff;
      puVar5 = puVar5 + 1;
    }
    puVar1[0x8a] = 1;
    puVar1[0x8e] = &DAT_10023758;
    uVar3 = FUN_100021c0(0x400,1);
    puVar1[0x93] = uVar3;
    iVar4 = FUN_100021c0(0x400,1);
    puVar1[0x127] = iVar4;
    if ((puVar1[0x93] != 0) && (iVar4 != 0)) {
      puVar5 = puVar1 + 0x94;
      for (iVar4 = 0x40; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar5 = 0x20202020;
        puVar5 = puVar5 + 1;
      }
      return puVar1;
    }
    FUN_10002200(puVar1[0x93]);
    FUN_10002200(puVar1[0x127]);
    FUN_10002200(puVar1);
  }
  return (undefined4 *)0x0;
}



/* ===== FUN_1000a650 @ 1000a650  size=40 ===== */

void __cdecl FUN_1000a650(undefined4 param_1)

{
  int *piVar1;
  int iVar2;
  
  iVar2 = 0;
  piVar1 = &DAT_100254f4;
  do {
    if (*piVar1 == 0) break;
    piVar1 = piVar1 + 1;
    iVar2 = iVar2 + 1;
  } while ((int)piVar1 < 0x10025684);
  if (iVar2 != 100) {
    (&DAT_100254f4)[iVar2] = param_1;
  }
  return;
}



/* ===== FUN_1000a6b0 @ 1000a6b0  size=1285 ===== */

void __cdecl FUN_1000a6b0(int param_1)

{
  uint uVar1;
  
  switch(*(undefined4 *)(param_1 + 0x18)) {
  case 0:
    FUN_1000abd0(param_1);
    FUN_1000ac20(param_1);
    *(undefined1 **)(param_1 + 0x4f8) = &LAB_10007a80;
    *(undefined1 **)(param_1 + 0x4fc) = &LAB_10007a80;
    *(undefined1 **)(param_1 + 0x500) = &LAB_10007a80;
    break;
  case 1:
    *(code **)(param_1 + 0x4a0) = FUN_10010a50;
    uVar1 = *(uint *)(param_1 + 4) & 0x8000;
    if (uVar1 == 0) {
      *(undefined1 **)(param_1 + 0x4a4) = &LAB_10010a80;
      *(undefined1 **)(param_1 + 0x4a8) = &LAB_10010b00;
      *(code **)(param_1 + 0x4ac) = FUN_10010ac0;
      *(undefined1 **)(param_1 + 0x4b0) = &LAB_10010b30;
      *(undefined1 **)(param_1 + 0x4c0) = &LAB_10010cd0;
      *(code **)(param_1 + 0x4c4) = FUN_10010d00;
    }
    else {
      *(code **)(param_1 + 0x4a4) = FUN_10010ac0;
      *(undefined1 **)(param_1 + 0x4a8) = &LAB_10010b30;
      *(undefined1 **)(param_1 + 0x4ac) = &LAB_10010a80;
      *(undefined1 **)(param_1 + 0x4b0) = &LAB_10010b00;
      *(code **)(param_1 + 0x4c0) = FUN_10010d00;
      *(undefined1 **)(param_1 + 0x4c4) = &LAB_10010cd0;
    }
    *(code **)(param_1 + 0x4b4) = FUN_10010b60;
    *(code **)(param_1 + 0x4b8) = FUN_10010c00;
    *(code **)(param_1 + 0x4bc) = FUN_10010c60;
    *(code **)(param_1 + 0x4c8) = FUN_10010d40;
    *(code **)(param_1 + 0x4cc) = FUN_10010dc0;
    if (uVar1 == 0) {
      *(undefined1 **)(param_1 + 0x4d0) = &LAB_10010e00;
      *(undefined1 **)(param_1 + 0x4d4) = &LAB_10010e50;
      *(code **)(param_1 + 0x4d8) = FUN_10010ea0;
      *(undefined1 **)(param_1 + 0x4dc) = &LAB_10010f00;
      *(undefined1 **)(param_1 + 0x4ec) = &LAB_10010e50;
      *(code **)(param_1 + 0x4f0) = FUN_100110d0;
    }
    else {
      *(code **)(param_1 + 0x4d0) = FUN_10010ea0;
      *(undefined1 **)(param_1 + 0x4d4) = &LAB_10010f00;
      *(undefined1 **)(param_1 + 0x4d8) = &LAB_10010e00;
      *(undefined1 **)(param_1 + 0x4dc) = &LAB_10010e50;
      *(code **)(param_1 + 0x4ec) = FUN_100110d0;
      *(undefined1 **)(param_1 + 0x4f0) = &LAB_10010e50;
    }
    *(undefined1 **)(param_1 + 0x4e0) = &LAB_10010f30;
    *(code **)(param_1 + 0x4e4) = FUN_10010fc0;
    *(code **)(param_1 + 0x4e8) = FUN_10011040;
    *(code **)(param_1 + 0x4f4) = FUN_10011110;
    *(undefined1 **)(param_1 + 0x4f8) = &LAB_100111e0;
    *(code **)(param_1 + 0x4fc) = FUN_10011210;
    *(undefined1 **)(param_1 + 0x500) = &LAB_10011270;
    break;
  case 2:
    *(code **)(param_1 + 0x4a0) = FUN_100102a0;
    uVar1 = *(uint *)(param_1 + 4) & 0x8000;
    if (uVar1 == 0) {
      *(code **)(param_1 + 0x4a4) = FUN_100102e0;
      *(code **)(param_1 + 0x4a8) = FUN_10010320;
      *(code **)(param_1 + 0x4ac) = FUN_10010350;
      *(undefined1 **)(param_1 + 0x4b0) = &LAB_10010390;
      *(code **)(param_1 + 0x4c0) = FUN_10010490;
      *(code **)(param_1 + 0x4c4) = FUN_100104e0;
    }
    else {
      *(code **)(param_1 + 0x4a4) = FUN_10010350;
      *(undefined1 **)(param_1 + 0x4a8) = &LAB_10010390;
      *(code **)(param_1 + 0x4ac) = FUN_100102e0;
      *(code **)(param_1 + 0x4b0) = FUN_10010320;
      *(code **)(param_1 + 0x4c0) = FUN_100104e0;
      *(code **)(param_1 + 0x4c4) = FUN_10010490;
    }
    *(undefined1 **)(param_1 + 0x4b4) = &LAB_100103b0;
    *(undefined1 **)(param_1 + 0x4b8) = &LAB_10010410;
    *(undefined1 **)(param_1 + 0x4bc) = &LAB_10010450;
    *(code **)(param_1 + 0x4c8) = FUN_10010520;
    *(code **)(param_1 + 0x4cc) = FUN_10010570;
    if (uVar1 == 0) {
      *(code **)(param_1 + 0x4d0) = FUN_100105b0;
      *(code **)(param_1 + 0x4d4) = FUN_100105f0;
      *(code **)(param_1 + 0x4d8) = FUN_10010630;
      *(code **)(param_1 + 0x4dc) = FUN_10010690;
      *(undefined1 **)(param_1 + 0x4ec) = &LAB_100107b0;
      *(undefined1 **)(param_1 + 0x4f0) = &LAB_100107f0;
    }
    else {
      *(code **)(param_1 + 0x4d0) = FUN_10010630;
      *(code **)(param_1 + 0x4d4) = FUN_10010690;
      *(code **)(param_1 + 0x4d8) = FUN_100105b0;
      *(code **)(param_1 + 0x4dc) = FUN_100105f0;
      *(undefined1 **)(param_1 + 0x4ec) = &LAB_100107f0;
      *(undefined1 **)(param_1 + 0x4f0) = &LAB_100107b0;
    }
    *(code **)(param_1 + 0x4e0) = FUN_100106c0;
    *(undefined1 **)(param_1 + 0x4e4) = &LAB_10010730;
    *(undefined1 **)(param_1 + 0x4e8) = &LAB_10010770;
    *(undefined1 **)(param_1 + 0x4f4) = &LAB_10010830;
    *(undefined1 **)(param_1 + 0x4f8) = &LAB_10010880;
    *(undefined1 **)(param_1 + 0x4fc) = &LAB_100108b0;
    *(undefined1 **)(param_1 + 0x500) = &LAB_10010900;
    break;
  case 3:
    *(code **)(param_1 + 0x4a0) = FUN_1000f7e0;
    uVar1 = *(uint *)(param_1 + 4) & 0x8000;
    if (uVar1 == 0) {
      *(undefined1 **)(param_1 + 0x4a4) = &LAB_1000f810;
      *(undefined1 **)(param_1 + 0x4a8) = &LAB_1000f850;
      *(code **)(param_1 + 0x4ac) = FUN_1000f890;
      *(code **)(param_1 + 0x4b0) = FUN_1000f8e0;
      *(undefined1 **)(param_1 + 0x4c0) = &LAB_1000fac0;
      *(code **)(param_1 + 0x4c4) = FUN_1000fb00;
    }
    else {
      *(code **)(param_1 + 0x4a4) = FUN_1000f890;
      *(code **)(param_1 + 0x4a8) = FUN_1000f8e0;
      *(undefined1 **)(param_1 + 0x4ac) = &LAB_1000f810;
      *(undefined1 **)(param_1 + 0x4b0) = &LAB_1000f850;
      *(code **)(param_1 + 0x4c0) = FUN_1000fb00;
      *(undefined1 **)(param_1 + 0x4c4) = &LAB_1000fac0;
    }
    *(undefined1 **)(param_1 + 0x4b4) = &LAB_1000f910;
    *(code **)(param_1 + 0x4b8) = FUN_1000f980;
    *(code **)(param_1 + 0x4bc) = FUN_1000fa10;
    *(code **)(param_1 + 0x4c8) = FUN_1000fb40;
    *(code **)(param_1 + 0x4cc) = FUN_1000fbf0;
    if (uVar1 == 0) {
      *(code **)(param_1 + 0x4d0) = FUN_1000fc30;
      *(code **)(param_1 + 0x4d4) = FUN_1000fc80;
      *(code **)(param_1 + 0x4d8) = FUN_1000fcc0;
      *(code **)(param_1 + 0x4dc) = FUN_1000fd20;
      *(code **)(param_1 + 0x4ec) = FUN_1000ff50;
      *(code **)(param_1 + 0x4f0) = FUN_1000ff90;
    }
    else {
      *(code **)(param_1 + 0x4d0) = FUN_1000fcc0;
      *(code **)(param_1 + 0x4d4) = FUN_1000fd20;
      *(code **)(param_1 + 0x4d8) = FUN_1000fc30;
      *(code **)(param_1 + 0x4dc) = FUN_1000fc80;
      *(code **)(param_1 + 0x4ec) = FUN_1000ff90;
      *(code **)(param_1 + 0x4f0) = FUN_1000ff50;
    }
    *(undefined1 **)(param_1 + 0x4e0) = &LAB_1000fd70;
    *(code **)(param_1 + 0x4e4) = FUN_1000fe10;
    *(code **)(param_1 + 0x4e8) = FUN_1000fea0;
    *(undefined1 **)(param_1 + 0x4f4) = &LAB_1000ffd0;
    *(undefined1 **)(param_1 + 0x4f8) = &LAB_10010080;
    *(undefined1 **)(param_1 + 0x4fc) = &LAB_10010090;
    *(undefined1 **)(param_1 + 0x500) = &LAB_100100e0;
    break;
  default:
    goto switchD_1000a6c1_default;
  }
  if ((*(uint *)(param_1 + 4) & 0x40) == 0) {
    if (((byte)*(uint *)(param_1 + 4) & 1) == 1) {
      FUN_1000ac20(param_1);
    }
    if (((byte)*(undefined4 *)(param_1 + 4) & 2) == 2) {
      FUN_1000abd0(param_1);
    }
  }
switchD_1000a6c1_default:
  return;
}



