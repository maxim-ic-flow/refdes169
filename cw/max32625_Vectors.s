/*****************************************************************************
 * Copyright (c) 2018 Rowley Associates Limited.                             *
 *                                                                           *
 * This file may be distributed under the terms of the License Agreement     *
 * provided with this software.                                              *
 *                                                                           *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE   *
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. *
 *                                                                           *
 *****************************************************************************/

/*****************************************************************************
 *                         Preprocessor Definitions                          *
 *                         ------------------------                          *
 * STARTUP_FROM_RESET                                                        *
 *                                                                           *
 *   If defined, the program will startup from power-on/reset. If not        *
 *   defined the program will just loop endlessly from power-on/reset.       *
 *                                                                           *
 * VECTORS_IN_RAM                                                            *
 *                                                                           *
 *   If defined, an area of RAM will large enough to store the vector table  *
 *   will be reserved.                                                       *
 *                                                                           *
 *****************************************************************************/

  .syntax unified
  .code 16

  .section .init, "ax"
  .align 2

/*****************************************************************************
 * Default Exception Handlers                                                *
 *****************************************************************************/

#ifndef STARTUP_FROM_RESET

  .thumb_func
  .weak Reset_Wait
Reset_Wait:
  b .

#endif

  .thumb_func
  .weak NMI_Handler
NMI_Handler:
  b .

  .thumb_func
  .weak HardFault_Handler
HardFault_Handler:
  b .

  .thumb_func
  .weak SVC_Handler
SVC_Handler:
  b .

  .thumb_func
  .weak PendSV_Handler
PendSV_Handler:
  b .

  .thumb_func
  .weak SysTick_Handler
SysTick_Handler:
  b .

  .thumb_func
Dummy_Handler:
  b .

#if defined(__OPTIMIZATION_SMALL)

  .weak CLKMAN_IRQHandler
  .thumb_set CLKMAN_IRQHandler,Dummy_Handler

  .weak PWRMAN_IRQHandler
  .thumb_set PWRMAN_IRQHandler,Dummy_Handler

  .weak FLC_IRQHandler
  .thumb_set FLC_IRQHandler,Dummy_Handler

  .weak RTC0_IRQHandler
  .thumb_set RTC0_IRQHandler,Dummy_Handler

  .weak RTC1_IRQHandler
  .thumb_set RTC1_IRQHandler,Dummy_Handler

  .weak RTC2_IRQHandler
  .thumb_set RTC2_IRQHandler,Dummy_Handler

  .weak RTC3_IRQHandler
  .thumb_set RTC3_IRQHandler,Dummy_Handler

  .weak PMU_IRQHandler
  .thumb_set PMU_IRQHandler,Dummy_Handler

  .weak USB_IRQHandler
  .thumb_set USB_IRQHandler,Dummy_Handler

  .weak AES_IRQHandler
  .thumb_set AES_IRQHandler,Dummy_Handler

  .weak MAA_IRQHandler
  .thumb_set MAA_IRQHandler,Dummy_Handler

  .weak WDT0_IRQHandler
  .thumb_set WDT0_IRQHandler,Dummy_Handler

  .weak WDT0_P_IRQHandler
  .thumb_set WDT0_P_IRQHandler,Dummy_Handler

  .weak WDT1_IRQHandler
  .thumb_set WDT1_IRQHandler,Dummy_Handler

  .weak WDT1_P_IRQHandler
  .thumb_set WDT1_P_IRQHandler,Dummy_Handler

  .weak GPIO_P0_IRQHandler
  .thumb_set GPIO_P0_IRQHandler,Dummy_Handler

  .weak GPIO_P1_IRQHandler
  .thumb_set GPIO_P1_IRQHandler,Dummy_Handler

  .weak GPIO_P2_IRQHandler
  .thumb_set GPIO_P2_IRQHandler,Dummy_Handler

  .weak GPIO_P3_IRQHandler
  .thumb_set GPIO_P3_IRQHandler,Dummy_Handler

  .weak GPIO_P4_IRQHandler
  .thumb_set GPIO_P4_IRQHandler,Dummy_Handler

  .weak GPIO_P5_IRQHandler
  .thumb_set GPIO_P5_IRQHandler,Dummy_Handler

  .weak GPIO_P6_IRQHandler
  .thumb_set GPIO_P6_IRQHandler,Dummy_Handler

  .weak TMR0_IRQHandler
  .thumb_set TMR0_IRQHandler,Dummy_Handler

  .weak TMR16_0_IRQHandler
  .thumb_set TMR16_0_IRQHandler,Dummy_Handler

  .weak TMR1_IRQHandler
  .thumb_set TMR1_IRQHandler,Dummy_Handler

  .weak TMR16_1_IRQHandler
  .thumb_set TMR16_1_IRQHandler,Dummy_Handler

  .weak TMR2_IRQHandler
  .thumb_set TMR2_IRQHandler,Dummy_Handler

  .weak TMR16_2_IRQHandler
  .thumb_set TMR16_2_IRQHandler,Dummy_Handler

  .weak TMR3_IRQHandler
  .thumb_set TMR3_IRQHandler,Dummy_Handler

  .weak TMR16_3_IRQHandler
  .thumb_set TMR16_3_IRQHandler,Dummy_Handler

  .weak TMR4_IRQHandler
  .thumb_set TMR4_IRQHandler,Dummy_Handler

  .weak TMR16_4_IRQHandler
  .thumb_set TMR16_4_IRQHandler,Dummy_Handler

  .weak TMR5_IRQHandler
  .thumb_set TMR5_IRQHandler,Dummy_Handler

  .weak TMR16_5_IRQHandler
  .thumb_set TMR16_5_IRQHandler,Dummy_Handler

  .weak UART0_IRQHandler
  .thumb_set UART0_IRQHandler,Dummy_Handler

  .weak UART1_IRQHandler
  .thumb_set UART1_IRQHandler,Dummy_Handler

  .weak UART2_IRQHandler
  .thumb_set UART2_IRQHandler,Dummy_Handler

  .weak UART3_IRQHandler
  .thumb_set UART3_IRQHandler,Dummy_Handler

  .weak PT_IRQHandler
  .thumb_set PT_IRQHandler,Dummy_Handler

  .weak I2CM0_IRQHandler
  .thumb_set I2CM0_IRQHandler,Dummy_Handler

  .weak I2CM1_IRQHandler
  .thumb_set I2CM1_IRQHandler,Dummy_Handler

  .weak I2CM2_IRQHandler
  .thumb_set I2CM2_IRQHandler,Dummy_Handler

  .weak I2CS_IRQHandler
  .thumb_set I2CS_IRQHandler,Dummy_Handler

  .weak SPIM0_IRQHandler
  .thumb_set SPIM0_IRQHandler,Dummy_Handler

  .weak SPIM1_IRQHandler
  .thumb_set SPIM1_IRQHandler,Dummy_Handler

  .weak SPIM2_IRQHandler
  .thumb_set SPIM2_IRQHandler,Dummy_Handler

  .weak SPIB_IRQHandler
  .thumb_set SPIB_IRQHandler,Dummy_Handler

  .weak OWM_IRQHandler
  .thumb_set OWM_IRQHandler,Dummy_Handler

  .weak AFE_IRQHandler
  .thumb_set AFE_IRQHandler,Dummy_Handler

  .weak SPIS_IRQHandler
  .thumb_set SPIS_IRQHandler,Dummy_Handler

  .weak GPIO_P7_IRQHandler
  .thumb_set GPIO_P7_IRQHandler,Dummy_Handler

  .weak GPIO_P8_IRQHandler
  .thumb_set GPIO_P8_IRQHandler,Dummy_Handler

#else

  .thumb_func
  .weak CLKMAN_IRQHandler
CLKMAN_IRQHandler:
  b .

  .thumb_func
  .weak PWRMAN_IRQHandler
PWRMAN_IRQHandler:
  b .

  .thumb_func
  .weak FLC_IRQHandler
FLC_IRQHandler:
  b .

  .thumb_func
  .weak RTC0_IRQHandler
RTC0_IRQHandler:
  b .

  .thumb_func
  .weak RTC1_IRQHandler
RTC1_IRQHandler:
  b .

  .thumb_func
  .weak RTC2_IRQHandler
RTC2_IRQHandler:
  b .

  .thumb_func
  .weak RTC3_IRQHandler
RTC3_IRQHandler:
  b .

  .thumb_func
  .weak PMU_IRQHandler
PMU_IRQHandler:
  b .

  .thumb_func
  .weak USB_IRQHandler
USB_IRQHandler:
  b .

  .thumb_func
  .weak AES_IRQHandler
AES_IRQHandler:
  b .

  .thumb_func
  .weak MAA_IRQHandler
MAA_IRQHandler:
  b .

  .thumb_func
  .weak WDT0_IRQHandler
WDT0_IRQHandler:
  b .

  .thumb_func
  .weak WDT0_P_IRQHandler
WDT0_P_IRQHandler:
  b .

  .thumb_func
  .weak WDT1_IRQHandler
WDT1_IRQHandler:
  b .

  .thumb_func
  .weak WDT1_P_IRQHandler
WDT1_P_IRQHandler:
  b .

  .thumb_func
  .weak GPIO_P0_IRQHandler
GPIO_P0_IRQHandler:
  b .

  .thumb_func
  .weak GPIO_P1_IRQHandler
GPIO_P1_IRQHandler:
  b .

  .thumb_func
  .weak GPIO_P2_IRQHandler
GPIO_P2_IRQHandler:
  b .

  .thumb_func
  .weak GPIO_P3_IRQHandler
GPIO_P3_IRQHandler:
  b .

  .thumb_func
  .weak GPIO_P4_IRQHandler
GPIO_P4_IRQHandler:
  b .

  .thumb_func
  .weak GPIO_P5_IRQHandler
GPIO_P5_IRQHandler:
  b .

  .thumb_func
  .weak GPIO_P6_IRQHandler
GPIO_P6_IRQHandler:
  b .

  .thumb_func
  .weak TMR0_IRQHandler
TMR0_IRQHandler:
  b .

  .thumb_func
  .weak TMR16_0_IRQHandler
TMR16_0_IRQHandler:
  b .

  .thumb_func
  .weak TMR1_IRQHandler
TMR1_IRQHandler:
  b .

  .thumb_func
  .weak TMR16_1_IRQHandler
TMR16_1_IRQHandler:
  b .

  .thumb_func
  .weak TMR2_IRQHandler
TMR2_IRQHandler:
  b .

  .thumb_func
  .weak TMR16_2_IRQHandler
TMR16_2_IRQHandler:
  b .

  .thumb_func
  .weak TMR3_IRQHandler
TMR3_IRQHandler:
  b .

  .thumb_func
  .weak TMR16_3_IRQHandler
TMR16_3_IRQHandler:
  b .

  .thumb_func
  .weak TMR4_IRQHandler
TMR4_IRQHandler:
  b .

  .thumb_func
  .weak TMR16_4_IRQHandler
TMR16_4_IRQHandler:
  b .

  .thumb_func
  .weak TMR5_IRQHandler
TMR5_IRQHandler:
  b .

  .thumb_func
  .weak TMR16_5_IRQHandler
TMR16_5_IRQHandler:
  b .

  .thumb_func
  .weak UART0_IRQHandler
UART0_IRQHandler:
  b .

  .thumb_func
  .weak UART1_IRQHandler
UART1_IRQHandler:
  b .

  .thumb_func
  .weak UART2_IRQHandler
UART2_IRQHandler:
  b .

  .thumb_func
  .weak UART3_IRQHandler
UART3_IRQHandler:
  b .

  .thumb_func
  .weak PT_IRQHandler
PT_IRQHandler:
  b .

  .thumb_func
  .weak I2CM0_IRQHandler
I2CM0_IRQHandler:
  b .

  .thumb_func
  .weak I2CM1_IRQHandler
I2CM1_IRQHandler:
  b .

  .thumb_func
  .weak I2CM2_IRQHandler
I2CM2_IRQHandler:
  b .

  .thumb_func
  .weak I2CS_IRQHandler
I2CS_IRQHandler:
  b .

  .thumb_func
  .weak SPIM0_IRQHandler
SPIM0_IRQHandler:
  b .

  .thumb_func
  .weak SPIM1_IRQHandler
SPIM1_IRQHandler:
  b .

  .thumb_func
  .weak SPIM2_IRQHandler
SPIM2_IRQHandler:
  b .

  .thumb_func
  .weak SPIB_IRQHandler
SPIB_IRQHandler:
  b .

  .thumb_func
  .weak OWM_IRQHandler
OWM_IRQHandler:
  b .

  .thumb_func
  .weak AFE_IRQHandler
AFE_IRQHandler:
  b .

  .thumb_func
  .weak SPIS_IRQHandler
SPIS_IRQHandler:
  b .

  .thumb_func
  .weak GPIO_P7_IRQHandler
GPIO_P7_IRQHandler:
  b .

  .thumb_func
  .weak GPIO_P8_IRQHandler
GPIO_P8_IRQHandler:
  b .

#endif

/*****************************************************************************
 * Vector Table                                                              *
 *****************************************************************************/

  .section .vectors, "ax"
  .align 2
  .global _vectors
  .extern __stack_end__
#ifdef STARTUP_FROM_RESET
  .extern Reset_Handler
#endif

_vectors:
  .word __stack_end__
#ifdef STARTUP_FROM_RESET
  .word Reset_Handler
#else
  .word Reset_Wait
#endif
  .word NMI_Handler
  .word HardFault_Handler
  .word 0 /* Reserved */
  .word 0 /* Reserved */
  .word 0 /* Reserved */
  .word 0 /* Reserved */
  .word 0 /* Reserved */
  .word 0 /* Reserved */
  .word 0 /* Reserved */
  .word SVC_Handler
  .word 0 /* Reserved */
  .word 0 /* Reserved */
  .word PendSV_Handler
  .word SysTick_Handler
  .word CLKMAN_IRQHandler
  .word PWRMAN_IRQHandler
  .word FLC_IRQHandler
  .word RTC0_IRQHandler
  .word RTC1_IRQHandler
  .word RTC2_IRQHandler
  .word RTC3_IRQHandler
  .word PMU_IRQHandler
  .word USB_IRQHandler
  .word AES_IRQHandler
  .word MAA_IRQHandler
  .word WDT0_IRQHandler
  .word WDT0_P_IRQHandler
  .word WDT1_IRQHandler
  .word WDT1_P_IRQHandler
  .word GPIO_P0_IRQHandler
  .word GPIO_P1_IRQHandler
  .word GPIO_P2_IRQHandler
  .word GPIO_P3_IRQHandler
  .word GPIO_P4_IRQHandler
  .word GPIO_P5_IRQHandler
  .word GPIO_P6_IRQHandler
  .word TMR0_IRQHandler
  .word TMR16_0_IRQHandler
  .word TMR1_IRQHandler
  .word TMR16_1_IRQHandler
  .word TMR2_IRQHandler
  .word TMR16_2_IRQHandler
  .word TMR3_IRQHandler
  .word TMR16_3_IRQHandler
  .word TMR4_IRQHandler
  .word TMR16_4_IRQHandler
  .word TMR5_IRQHandler
  .word TMR16_5_IRQHandler
  .word UART0_IRQHandler
  .word UART1_IRQHandler
  .word UART2_IRQHandler
  .word UART3_IRQHandler
  .word PT_IRQHandler
  .word I2CM0_IRQHandler
  .word I2CM1_IRQHandler
  .word I2CM2_IRQHandler
  .word I2CS_IRQHandler
  .word SPIM0_IRQHandler
  .word SPIM1_IRQHandler
  .word SPIM2_IRQHandler
  .word SPIB_IRQHandler
  .word OWM_IRQHandler
  .word AFE_IRQHandler
  .word SPIS_IRQHandler
  .word GPIO_P7_IRQHandler
  .word GPIO_P8_IRQHandler
_vectors_end:

#ifdef VECTORS_IN_RAM
  .section .vectors_ram, "ax"
  .align 2
  .global _vectors_ram

_vectors_ram:
  .space _vectors_end - _vectors, 0
#endif
