/* ===== FUN_004452c0 @ 004452c0  size=96 ===== */

undefined4 FUN_004452c0(int *param_1)

{
  undefined4 uVar1;
  
  uVar1 = tioGetHandle(param_1,0xa0,1);
  tiohbRead(uVar1);
  if (((((*param_1 == 0x4353494d) && (param_1[2] == 0xa0)) && (0x9f < (uint)param_1[3])) &&
      ((param_1[4] != 0 && (param_1[5] != 0)))) &&
     ((param_1[6] != 0 && ((param_1[7] != 0 && ((uint)param_1[7] < 0xb)))))) {
    return 1;
  }
  return 0;
}



