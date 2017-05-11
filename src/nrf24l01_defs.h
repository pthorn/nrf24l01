#ifndef NRF24L01_DEFS_H
#define NRF24L01_DEFS_H

//
// register addresses. all registers are 8 bits long
//

#define nRF_REG_CONFIG         0x00   // configuration
#define REG_CONFIG_MASK_RX_DR  6      // mask RX_DR  interrupt (1=masked, 0=unmasked)
#define REG_CONFIG_MASK_TX_DS  5      // mask TX_DS  interrupt (1=masked, 0=unmasked)
#define REG_CONFIG_MASK_MAX_RT 4      // mask MAX_RT interrupt (1=masked, 0=unmasked)
#define REG_CONFIG_EN_CRC      3      // enable CRC
#define REG_CONFIG_CRCO        2      // 0 = 1 byte, 1 = 2 bytes
#define REG_CONFIG_PWR_UP      1      // power up (required to exit standby-I mode)
#define REG_CONFIG_PRIM_RX     0      // 1 = RX, 0 = TX

#define nRF_REG_EN_AA       0x01   // enable auto ack for pipes 0...5 (enables ESB)
#define nRF_REG_EN_RXADDR   0x02   // enabled rx addresses

#define nRF_REG_SETUP_AW    0x03   // setup of address widths
#define REG_SETUP_AW_AW 0          // 01->3 bytes, 10->4, 11->5 bytes

#define nRF_REG_SETUP_RETR  0x04   // setup of automatic retransmission
#define REG_SETUP_RETR_ARD  4
#define REG_SETUP_RETR_ARC  0

#define nRF_REG_RF_CH       0x05   // rf channel

#define nRF_REG_RF_SETUP    0x06   // rf setup
#define RF_SETUP_CONT_WAVE  7      // tx carrier continuously
#define RF_SETUP_RF_DR_LOW  5
#define RF_SETUP_PLL_LOCK   4      // force PLL lock (do not use)
#define RF_SETUP_RF_DR_HIGH 3      // [RF_DR_LOW:RF_DR_HIGH]: 00->1Mbps, 01->2Mbps, 10->250Kbps
#define RF_SETUP_RF_PWR     1      // 00->-18dBm, 01->-12dBm, 10->-6dBm, 11->0dBm

#define nRF_REG_STATUS      0x07   // status (read automatically with every command)
#define REG_STATUS_RX_DR    6      // data ready interrupt, write 1 to clear
#define REG_STATUS_TX_DS    5      // data sent interrupt, write 1 to clear
#define REG_STATUS_MAX_RT   4      // max retransmissions interrupt, write 1 to clear
#define REG_STATUS_RX_P_NO  1      // [3:1] data pipe number for received payload
                                   // 000...101, 110 - not used, 111 - rx fifo empty
#define REG_STATUS_TX_FULL  0      // TX FIFO full

#define nRF_REG_OBSERVE_TX      0x08   // transmit observe
#define REG_OBSERVE_TX_PLOS_CNT 4      // [7:4] count of lost packets, maxes out @ 15,
                                       // reset by writing to RF_CH
#define REG_OBSERVE_TX_ARC_CNT  0      // [3:0] count of retransmitted packets,
                                       // reset at new packet transmission

#define nRF_REG_RPD         0x09   // received power detector
#define REG_RPD_RPD         0      // [0] received power detector

// addresses are 3...5 bytes long (set by SETUP_AW), lsb first
// rx addrs for pipes 1...5 are the same as that of P0 except the LSB, so registers are 1 byte long
#define nRF_REG_RX_ADDR_P(n) (0x0A + (n))  // receive address for data pipe n (0...5)
#define nRF_REG_TX_ADDR     0x10           // transmit address (ptx only)

#define nRF_REG_RX_PW_P(n) (0x11 + (n))  // length of received payload for pipe #n

#define nRF_REG_FIFO_STATUS       0x17   // fifo status
#define REG_FIFO_STATUS_TX_REUSE  6
#define REG_FIFO_STATUS_TX_FULL   5
#define REG_FIFO_STATUS_TX_EMPTY  4
#define REG_FIFO_STATUS_RX_FULL   1
#define REG_FIFO_STATUS_RX_EMPTY  0

#define nRF_REG_FEATURE     0x1D   // features
#define EN_DPL 2
#define EN_ACK_PAY 1
#define EN_DYN_ACK 0


//
// commands, see datasheet page 51
//

#define nRF_CMD_R_REGISTER          0x00  // read registers, OR with a 5-bit start address. up to 5 bytes
#define nRF_CMD_W_REGISTER          0x20  // write registers, OR with a 5-bit start address. up to 5 bytes
#define nRF_CMD_R_RX_PAYLOAD        0x61  // read RX payload, up to 32 bytes
#define nRF_CMD_W_TX_PAYLOAD        0xa0  // write TX payload, up to 32 bytes
#define nRF_CMD_FLUSH_TX            0xe1
#define nRF_CMD_FLUSH_RX            0xe0
#define nRF_CMD_REUSE_TX_PL         0xe3  // active until FLUSH_TX or W_TX_PAYLOAD, see STATUS.TX_REUSE
#define nRF_CMD_R_RX_PL_WID         0x60  // read payload width (when DPL is on (FEATURE.EN_DPL))
#define nRF_CMD_W_ACK_PAYLOAD       0x
#define nRF_CMD_W_TX_PAYLOAD_NO_ACK 0x
#define nRF_CMD_NOP                 0xFF

#endif // NRF24L01_DEFS_H
