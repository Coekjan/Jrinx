#ifndef _KERN_DRIVERS_SERIAL_UART16550A_H_
#define _KERN_DRIVERS_SERIAL_UART16550A_H_

#define COM_RBR 0
#define COM_THR 0
#define COM_DLL 0
#define COM_IER 1
#define COM_DLM 1
#define COM_IIR 2
#define COM_FCR 2
#define COM_LCR 3
#define COM_MCR 4
#define COM_LSR 5
#define COM_MSR 6
#define COM_SCR 7

#define COM_IER_RCV_DATA_AVAIL 0b00000001
#define COM_IER_TRANS_HOLD_REG_EMPTY 0b00000010
#define COM_IER_RCV_LSR_CHG 0b00000100
#define COM_IER_MODEM_SR_CHG 0b00001000
#define COM_IER_SLEEP_MODE 0b00010000
#define COM_IER_LOW_PWR_MODE 0b00100000

#define COM_IIR_IP 0b00000001
#define COM_IIR_NO_IP 0b00000010
#define COM_IIR_CAUSE 0b00001110
#define COM_IIR_FIFO64_EN 0b00100000
#define COM_IIR_FIFO_SR 0b11000000

#define COM_FCR_FIFO_EN 0b00000001
#define COM_FCR_CLR_RCV_FIFO 0b00000010
#define COM_FCR_CLR_TRANS_FIFO 0b00000100
#define COM_FCR_DMA_MODE 0b00001000
#define COM_FCR_FIFO64_EN 0b00100000
#define COM_FCR_BYTES_NUM 0b11000000

#define COM_LCR_BITWIDTH 0b00000011
#define COM_LCR_STOPBIT_NUM 0b00000100
#define COM_LCR_PARITY_MODE 0b00111000
#define COM_LCR_BRK_SIG_EN 0b01000000
#define COM_LCR_DLAB_EN 0b10000000

#define COM_MCR_DATA_TREM_RDY 0b00000001
#define COM_MCR_DATA_REQ_SEND 0b00000010
#define COM_MCR_AUX_OUT1 0b00000100
#define COM_MCR_AUX_OUT2 0b00001000
#define COM_LOOPBACK_MODE 0b00010000
#define COM_AUTOFLOW_CTRL 0b00100000

#define COM_LSR_DATA_AVAIL 0b00000001
#define COM_LSR_OVERRUN_ERR 0b00000010
#define COM_LSR_PARITY_ERR 0b00000100
#define COM_LSR_FRAMING_ERR 0b00001000
#define COM_LSR_BRK_SIG_RCV 0b00010000
#define COM_LSR_THR_EMPTY 0b00100000
#define COM_LSR_THR_EMPTY_LINE_IDLE 0b01000000
#define COM_LSR_FIFO_ERR_DATA 0b10000000

#define COM_MSR_CHG_CLR_TO_SND 0b00000001
#define COM_MSR_CHG_DATASET_RDY 0b00000010
#define COM_MSR_CHG_TR_EDGE_RING_INDIC 0b00000100
#define COM_MSR_CHG_CHANGE_CARRIER_DETECT 0b00001000
#define COM_MSR_CLR_TO_SND 0b00010000
#define COM_MSR_DATASET_RDY 0b00100000
#define COM_MSR_RING_INDIC 0b01000000
#define COM_MSR_CARRIAR_DETECT 0b10000000

#endif
