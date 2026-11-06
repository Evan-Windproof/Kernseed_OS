/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include "abi.h"
void _start(void){if(call(999,0,0)!=-38||call(0,0,8)!=-14||call(0,(long)0xffffffff80000000ULL,8)!=-14||call(0,(long)0xffffc00001100fffULL,8)!=-14||call(0,0,0)!=0)write_text("PROBE FAILED\n");else write_text("PROBE PASSED\n");finish();}
