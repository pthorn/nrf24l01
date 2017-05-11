/*
    http://maniacbug.github.com/RF24/
    https://github.com/maniacbug/RF24/
    http://iteadstudio.com/store/index.php?main_page=product_info&cPath=7&products_id=53
    http://iteadstudio.com/store/index.php?main_page=product_info&cPath=7&products_id=419

*/

#include "nrf24l01.h"
#include "nrf24l01_defs.h"
#include "print.h"


#define DELAY_CSN_SETUP     200  // 2ns
#define DELAY_CSN_HOLD      200  // 2ns
#define DELAY_BETWEEN_REQ  5000  // 50ns


NRF24L01::NRF24L01(
    SPI_TypeDef* nRF_SPI,
    uint8_t af,
    Pin nrf_sck,
    Pin nrf_miso,
    Pin nrf_mosi,
    Pin nrf_csn,
    Pin nrf_ce,
    Pin nrf_irq
):
    nRF_SPI(nRF_SPI),
    af(af),
    nrf_sck(nrf_sck),
    nrf_miso(nrf_miso),
    nrf_mosi(nrf_mosi),
    nrf_csn(nrf_csn),
    nrf_ce(nrf_ce),
    nrf_irq(nrf_irq)
{
    // TODO init pins that are not going to be referenced later here
    // and forget about them
}


void NRF24L01::init()
{
    print("NRF24L01::init()\n");

    init_gpio();
    init_spi();
}


void NRF24L01::setup_rf(uint8_t channel, DataRate data_rate, RFPower rf_power)
{
    cmd_write_register(nRF_REG_RF_CH, channel & 0x7F);

    for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

    cmd_write_register(nRF_REG_RF_SETUP,
        ((data_rate == DataRate::DR_250Kbps ? 1 : 0) << RF_SETUP_RF_DR_LOW) |
        ((data_rate == DataRate::DR_2Mbps ? 1 : 0) << RF_SETUP_RF_DR_HIGH) |
        (static_cast<uint8_t>(rf_power) << RF_SETUP_RF_PWR)
    );
}


void NRF24L01::setup_esb(
    AddrWidth addr_width,
    AutoRetransmitDelay ard,       // see table 18 in datasheet!
    uint8_t auto_retransmit_count, //
    bool enable_dynamic_payload,
    bool enable_payload_in_ack,
    bool enable_dynamic_ack
) {
    if (auto_retransmit_count > 15) {
        print("auto_retransmit_count > 15!\n");
        return;
    }

    cmd_write_register(nRF_REG_SETUP_AW, static_cast<uint8_t>(addr_width));

    for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

    cmd_write_register(nRF_REG_SETUP_RETR,
        (static_cast<uint8_t>(ard) << REG_SETUP_RETR_ARD) |
        ((auto_retransmit_count & 0xF) << REG_SETUP_RETR_ARC)
    );

    for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

    cmd_write_register(nRF_REG_FEATURE,
        ((enable_dynamic_payload || enable_payload_in_ack ? 1 : 0) << EN_DPL) |
        ((enable_payload_in_ack ? 1 : 0) << EN_ACK_PAY) |
        ((enable_dynamic_ack ? 1 : 0) << EN_DYN_ACK)
    );
    // TODO if enable_dynamic_payload || enable_payload_in_ack then enable DPL for pipe 0
}


void NRF24L01::setup_esb_pipe(
    uint8_t pipe,
    bool enable,
    bool enable_auto_ack,
    uint8_t payload_length
) {
    // TODO per-pipe settings
    // - TODO enable auto ack (default: all enabled)
    // - enable pipe for rx ("enable rx address") (default: 0 and 1 enabled)
    // - rx address. pipes 0 and 1 can have up to 5 bytes, pipes 2..5 only 1 byte,
    //   the rest is taken from pipe 1. pipe 1 rx addr should be = tx addr for auto-acks
    // - payload length

    if (pipe > 5) {
        print("!\n");
        return;
    }

    // TODO check for max payload length!

    uint8_t en_aa = cmd_read_register(nRF_REG_EN_AA);

    for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

    uint8_t en_rxaddr = cmd_read_register(nRF_REG_EN_RXADDR);

    for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

    cmd_write_register(nRF_REG_RX_PW_P(pipe), payload_length);
}


/**
    Set transmit address AND receive address for pipe 0
 */
void NRF24L01::set_tx_addr(uint8_t const* addr)
{
    cmd_write_register(nRF_REG_TX_ADDR, addr, 5);  // TODO 5

    for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

    set_rx_addr(0, addr);
}


/**
    Set receive address of pipe 0 or 1
 */
void NRF24L01::set_rx_addr(uint8_t pipe, uint8_t const* addr)
{
    if (pipe != 0 && pipe != 1) {
        print("!\n");
        return;
    }

    cmd_write_register(nRF_REG_RX_ADDR_P(pipe), addr, 5);  // TODO 5
}


/**
  Addresses of pipes 2...5 only differ from the address of pipe 1 in the last byte
 */
void NRF24L01::set_rx_addr(uint8_t pipe, uint8_t addr)
{
    if (pipe < 2 || pipe > 5) {
        print("!\n");
        return;
    }

    cmd_write_register(nRF_REG_RX_ADDR_P(pipe), addr);
}


void NRF24L01::enable_rx(bool enable)
{
    if (enable) {
        uint8_t config = cmd_read_register(nRF_REG_CONFIG);
        cmd_write_register(nRF_REG_CONFIG, config | (1 << REG_CONFIG_PRIM_RX));
        nrf_ce.set_high();
    } else {
        nrf_ce.set_low();
    }
}


void NRF24L01::transmit(unsigned char const* data, uint8_t n_bytes)
{
    uint8_t const config = cmd_read_register(nRF_REG_CONFIG);

    if (status & (1 << REG_STATUS_TX_FULL)) {
        print("transmit: TX_FULL\n");
        return;
    }
    if (status & (1 << REG_STATUS_MAX_RT)) {
        print("transmit: MAX_RT\n");
        return;
    }

    if (config & (1 << REG_CONFIG_PRIM_RX)) {
        // set PTX mode
        uint8_t config = cmd_read_register(nRF_REG_CONFIG);
        cmd_write_register(nRF_REG_CONFIG, config & ~(1 << REG_CONFIG_PRIM_RX));
    }

    cmd_write_payload(data, n_bytes);

    for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

    // 10us pulse
    // if CE is left HIGH, the chip will go into Standby-II after transmitting
    nrf_ce.set_high();
    for (volatile int i = 0; i < 2000; ++i) ;;  // TODO 10us
    nrf_ce.set_low();
}


void NRF24L01::retransmit()
{
    // TODO
}


//
// actions
//

void NRF24L01::power(bool power)
{
    uint8_t config = cmd_read_register(nRF_REG_CONFIG);

    if (power) {
        config |= (1 << REG_CONFIG_PWR_UP);
    } else {
        config &= ~(1 << REG_CONFIG_PWR_UP);
    }

    cmd_write_register(nRF_REG_CONFIG, config);
}


void NRF24L01::poll()
{
    cmd_nop();

    if (status & (1 << REG_STATUS_RX_DR)) {
        // data ready: read payload
        cmd_write_register(nRF_REG_STATUS, 1 << REG_STATUS_RX_DR);

        for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

        uint8_t pipe_n;
        uint8_t size;
        uint8_t buffer[32];
        get_payload(pipe_n, buffer, size);

        print("poll: data ready, pipe %s, size %s: ", pipe_n, size);
        for (uint8_t i = 0; i < size; ++i) {
            print("%02x ", buffer[i]);
        }
        print("\n");
    }

    if (status & (1 << REG_STATUS_TX_DS)) {
        // data sent
        cmd_write_register(nRF_REG_STATUS, 1 << REG_STATUS_TX_DS);
        print("poll: data sent\n");

        // TODO
        // if (more data) {
        //     write_payload();
        //     start_tx();
        // }
    }

    if (status & (1 << REG_STATUS_MAX_RT)) {
        // max retransmissions
        print("poll: max RT, flush TX FIFO\n");
        cmd_write_register(nRF_REG_STATUS, 1 << REG_STATUS_MAX_RT);

        // TODO perhaps some retransmit logic?
        for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

        cmd_flush_tx_fifo();
    }
}


void NRF24L01::get_payload(uint8_t& pipe, uint8_t* buffer, uint8_t& size)
{
    //cmd_nop();

    if ((status & (1 << REG_STATUS_RX_DR)) == 0) {
        print("get_payload(): RX_DR not set\n");
        size = 0;
        return;
    }

    // get pipe number
    pipe = (status >> 1) & 7;

    if (pipe > 5) {
        print("get_payload(): bad pipe %s\n", pipe);
        size = 0;
        return;
    }

    //print("get_payload() 3 pipe %s\n", pipe);

    // get payload size
    // TODO nRF_CMD_R_RX_PL_WID
    size = get_rx_length_for_pipe(pipe);

    for (volatile int i = 0; i < DELAY_BETWEEN_REQ; ++i) ;;

    //print("get_payload() 4 size %s\n", size);

//    if (size > 32) {
//        print("get_payload(): size %s > 32, pipe %s, flush\n", size, pipe);
//        flush_rx_fifo();
//        size = 0;
//        return;
//    }

    // get payload
    cmd_read_payload(buffer, size);
}


uint8_t NRF24L01::get_rx_length_for_pipe(uint8_t pipe)
{
    // TODO support dynamic length
    return cmd_read_register(nRF_REG_RX_PW_P(pipe));
}


//
// debug
//

void NRF24L01::dump_status()
{
    // TODO print state (Standby-II etc.)

    uint8_t const config = cmd_read_register(nRF_REG_CONFIG);
    uint8_t const rx_pipe = (status >> 1) & 7;
    uint8_t const fifo_status = cmd_read_register(nRF_REG_FIFO_STATUS);

    print("CE %s status:%s%s%s%s RXP#: %s%s",
        nrf_ce.value(),
        status & (1 << REG_STATUS_RX_DR) ? "RX_DR" : "",        // data ready, c1
        status & (1 << REG_STATUS_TX_DS) ? " TX_DS" : "",       // data sent, c1
        status & (1 << REG_STATUS_MAX_RT) ? " MAX_RT" : "",     // max # of retransmits, c1
        status & (1 << REG_STATUS_TX_FULL) ? " TX_FULL" : "",   // TX FIFO full
        rx_pipe,                                                // pipe # for which data arrived
        rx_pipe == 7 ? " (none)" : "");

    print("; config:%s%s%s%s%s%s%s",
        config & (1 << REG_CONFIG_MASK_RX_DR) ? " MASK_RX_DR" : "",
        config & (1 << REG_CONFIG_MASK_TX_DS) ? " MASK_TX_DS" : "",
        config & (1 << REG_CONFIG_MASK_MAX_RT) ? " MASK_MAX_RT" : "",
        config & (1 << REG_CONFIG_EN_CRC) ? " EN_CRC" : "",
        config & (1 << REG_CONFIG_CRCO) ? " CRC1" : " CRC2",
        config & (1 << REG_CONFIG_PWR_UP) ? " PWR_UP" : "",
        config & (1 << REG_CONFIG_PRIM_RX) ? " PRX" : " PTX");

    print("; fifo_status:%s%s%s%s%s",
        fifo_status & (1 << REG_FIFO_STATUS_TX_REUSE) ? " TX_REUSE" : "",
        fifo_status & (1 << REG_FIFO_STATUS_TX_FULL) ? " TX_FULL" : "",
        fifo_status & (1 << REG_FIFO_STATUS_TX_EMPTY) ? " TX_EMPTY" : "",
        fifo_status & (1 << REG_FIFO_STATUS_RX_FULL) ? " RX_FULL" : "",
        fifo_status & (1 << REG_FIFO_STATUS_RX_EMPTY) ? " RX_EMPTY" : "");

    if (status & (1 << REG_STATUS_RX_DR)) {
        if (rx_pipe <= 5) {
            uint8_t const payload_size = get_rx_length_for_pipe(rx_pipe);
            print("; payload size %s", payload_size);
        }
    }

    print("\n");
}


void NRF24L01::dump_addrs()
{
    uint8_t addr[5];

    cmd_read_register(nRF_REG_TX_ADDR, addr, 5);
    print("TX        addr %02x%02x%02x%02x%02x\n", addr[4], addr[3], addr[2], addr[1], addr[0]);

    cmd_read_register(nRF_REG_RX_ADDR_P(0), addr, 5);
    print("RX pipe 0 addr %02x%02x%02x%02x%02x\n", addr[4], addr[3], addr[2], addr[1], addr[0]);

    cmd_read_register(nRF_REG_RX_ADDR_P(1), addr, 5);
    print("RX pipe 1 addr %02x%02x%02x%02x%02x\n", addr[4], addr[3], addr[2], addr[1], addr[0]);

    uint8_t p2 = cmd_read_register(nRF_REG_RX_ADDR_P(2));
    print("RX pipe 2 addr %02x\n", p2);
}


//
// SPI commands
//

void NRF24L01::cmd_nop()
{
    nrf_csn.set_low();
    status = spi_transfer_byte(nRF_CMD_NOP);
    nrf_csn.set_high();
}


uint8_t NRF24L01::cmd_read_register(uint8_t reg_addr)
{
    nrf_csn.set_low();

    for (volatile int i = 0; i < DELAY_CSN_SETUP; ++i) ;;

    status = spi_transfer_byte(nRF_CMD_R_REGISTER | (reg_addr & 0x1F));
    uint8_t value = spi_transfer_byte(0xFF);

    for (volatile int i = 0; i < DELAY_CSN_HOLD; ++i) ;;

    nrf_csn.set_high();

    return value;
}


void NRF24L01::cmd_write_register(uint8_t reg_addr, uint8_t value)
{
    nrf_csn.set_low();

    for (volatile int i = 0; i < DELAY_CSN_SETUP; ++i) ;;

    status = spi_transfer_byte(nRF_CMD_W_REGISTER | (reg_addr & 0x1F));
    spi_transfer_byte(value);

    for (volatile int i = 0; i < DELAY_CSN_HOLD; ++i) ;;

    nrf_csn.set_high();
}


/**
    read up to 5 bytes
*/
void NRF24L01::cmd_read_register(uint8_t reg_addr, uint8_t* buffer, uint8_t n_bytes)
{
    nrf_csn.set_low();

    for (volatile int i = 0; i < DELAY_CSN_SETUP; ++i) ;;

    status = spi_transfer_byte(nRF_CMD_R_REGISTER | (reg_addr & 0x1F));

    for (int i = 0; i < n_bytes; ++i) {
        buffer[i] = spi_transfer_byte(0xFF);
    }

    for (volatile int i = 0; i < DELAY_CSN_HOLD; ++i) ;;

    nrf_csn.set_high();
}


/**
    write up to 5 bytes
*/
void NRF24L01::cmd_write_register(uint8_t reg_addr, uint8_t const* buffer, uint8_t n_bytes)
{
    nrf_csn.set_low();

    for (volatile int i = 0; i < DELAY_CSN_SETUP; ++i) ;;

    status = spi_transfer_byte(nRF_CMD_W_REGISTER | (reg_addr & 0x1F));

    for (int i = 0; i < n_bytes; ++i) {
        spi_transfer_byte(buffer[i]);
    }

    for (volatile int i = 0; i < DELAY_CSN_HOLD; ++i) ;;

    nrf_csn.set_high();
}


void NRF24L01::cmd_read_payload(uint8_t *buffer, uint8_t n_bytes)
{
    nrf_csn.set_low();

    for (volatile int i = 0; i < DELAY_CSN_SETUP; ++i) ;;

    status = spi_transfer_byte(nRF_CMD_R_RX_PAYLOAD);

    for (int i = 0; i < n_bytes; ++i) {
        buffer[i] = spi_transfer_byte(0xFF);
    }

    for (volatile int i = 0; i < DELAY_CSN_HOLD; ++i) ;;

    nrf_csn.set_high();
}


void NRF24L01::cmd_write_payload(uint8_t const* buffer, uint8_t n_bytes)
{
    nrf_csn.set_low();

    for (volatile int i = 0; i < DELAY_CSN_SETUP; ++i) ;;

    status = spi_transfer_byte(nRF_CMD_W_TX_PAYLOAD);

    for (uint8_t i = 0; i < n_bytes; ++i) {
        spi_transfer_byte(buffer[i]);
    }

    for (volatile int i = 0; i < DELAY_CSN_HOLD; ++i) ;;

    nrf_csn.set_high();
}


void NRF24L01::cmd_flush_tx_fifo()
{
    nrf_csn.set_low();

    for (volatile int i = 0; i < DELAY_CSN_SETUP; ++i) ;;

    status = spi_transfer_byte(nRF_CMD_FLUSH_TX);

    for (volatile int i = 0; i < DELAY_CSN_HOLD; ++i) ;;

    nrf_csn.set_high();
}


void NRF24L01::cmd_flush_rx_fifo()
{
    nrf_csn.set_low();

    for (volatile int i = 0; i < DELAY_CSN_SETUP; ++i) ;;

    status = spi_transfer_byte(nRF_CMD_FLUSH_RX);

    for (volatile int i = 0; i < DELAY_CSN_HOLD; ++i) ;;

    nrf_csn.set_high();
}


void NRF24L01::cmd_reuse_tx_payload()
{
    nrf_csn.set_low();

    for (volatile int i = 0; i < DELAY_CSN_SETUP; ++i) ;;

    status = spi_transfer_byte(nRF_CMD_REUSE_TX_PL);

    for (volatile int i = 0; i < DELAY_CSN_HOLD; ++i) ;;

    nrf_csn.set_high();
}


//
// low level
//

uint8_t NRF24L01::spi_transfer_byte(uint8_t byte)
{
    while ((nRF_SPI->SR & SPI_SR_TXE) == 0) ;;
    nRF_SPI->DR = byte;
    while ((nRF_SPI->SR & SPI_SR_RXNE) == 0) ;;
    return nRF_SPI->DR;
}


void NRF24L01::init_gpio()
{
    nrf_sck
        .set_mode(Pin::Mode::AF)
        .set_speed(Pin::Speed::Medium)
        .set_af(af);

    nrf_miso
        .set_mode(Pin::Mode::AF)
        .set_speed(Pin::Speed::Medium)
        .set_af(af);

    nrf_mosi
        .set_mode(Pin::Mode::AF)
        .set_speed(Pin::Speed::Medium)
        .set_af(af);

    nrf_csn
        .set_mode(Pin::Mode::Output)
        .set_speed(Pin::Speed::Low)
        .set_high();

    nrf_ce
        .set_mode(Pin::Mode::Output)
        .set_speed(Pin::Speed::Low)
        .set_low();

    nrf_irq
        .set_mode(Pin::Mode::Input)
        .set_speed(Pin::Speed::Low);
}


void NRF24L01::init_spi()
{
//    nRF_SPI->CR2 =
//        (0xF << SPI_CR2_DS_Pos);  // 16 bits TODO 8

    // TODO baud rate, 8 bits, MSB first
    nRF_SPI->CR1 =
        SPI_CR1_MSTR |
        SPI_CR1_SSM |
        SPI_CR1_SSI |            // required in master mode with SSM on
        (5 << SPI_CR1_BR_Pos) |  // speed (see RM0360 rev 3 page 667)
        SPI_CR1_SPE;
}


#if 0
enum class State {
    POWER_DOWN,
    STANDBY_I,
    STANDBY_II,
    RX,
    TX
};
#endif
