#ifndef _KERN_DRIVERS_SERIAL_SIFIVEUART0_H_
#define _KERN_DRIVERS_SERIAL_SIFIVEUART0_H_

#define COM_TXDATA 0x00
#define COM_RXDATA 0x04
#define COM_TXCTRL 0x08
#define COM_RXCTRL 0x0c
#define COM_IE 0x10
#define COM_IP 0x14
#define COM_DIV 0x18

#define COM_TXDATA_FULL 0x80000000U
#define COM_RXDATA_EMPTY 0x80000000U

#define COM_TXCTRL_TXEN 0x1U
#define COM_TXCTRL_NSTOP 0x2U
#define COM_TXCTRL_TXCNT_SHIFT 16

#define COM_RXCTRL_RXEN 0x1U
#define COM_RXCTRL_RXCNT_SHIFT 16

#define COM_IPE_TXWM 0x1U
#define COM_IPE_RXWM 0x2U

#endif
