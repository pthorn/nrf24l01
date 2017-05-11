#ifndef NRF24L01_H
#define NRF24L01_H

#include <cstdint>

#include "gpio.h"


class NRF24L01
{
public:
    enum class DataRate {
        DR_250Kbps,
        DR_1Mbps,
        DR_2Mbps
    };

    enum class RFPower {
        PWR_minus18dBm = 0,
        PWR_minus12dBm = 1,
        PWR_minus6dBm = 2,
        PWR_0dBm = 3
    };

    enum class AddrWidth {
        AW_3bytes = 1,
        AW_4bytes = 2,
        AW_5bytes = 3
    };

    enum class AutoRetransmitDelay {
        ARD_250uS,
        ARD_500uS,
        ARD_750uS,
        ARD_1000uS,
        ARD_1250uS,
        ARD_1500uS,
        ARD_1750uS,
        ARD_2000uS,
        ARD_2250uS,
        ARD_2500uS,
        ARD_2750uS,
        ARD_3000uS,
        ARD_3250uS,
        ARD_3500uS,
        ARD_3750uS,
        ARD_4000uS
    };

    NRF24L01(
        SPI_TypeDef* nRF_SPI,
        uint8_t af,
        Pin nrf_sck,
        Pin nrf_miso,
        Pin nrf_mosi,
        Pin nrf_csn,
        Pin nrf_ce,
        Pin nrf_irq
    );

    void poll();  // call from EXTI handler

    // initialization and configuration

    void init();
    void setup_rf(uint8_t channel, DataRate data_rate, RFPower rf_power);
    void setup_esb(
        AddrWidth addr_width,
        AutoRetransmitDelay ard,
        uint8_t auto_retransmit_count,
        bool enable_dynamic_payload,
        bool enable_payload_in_ack,
        bool enable_dynamic_ack
    );
    void setup_esb_pipe(
        uint8_t pipe,
        bool enable,
        bool enable_auto_ack,
        uint8_t payload_length
    );
    void set_tx_addr(const uint8_t *addr);
    void set_rx_addr(uint8_t pipe, uint8_t const* addr);
    void set_rx_addr(uint8_t pipe, uint8_t addr);

    // receive and transmit

    void enable_rx(bool enable);
    void transmit(unsigned char const* data, uint8_t n_bytes);
    void retransmit();

//    void test_rx();
//    void test_tx();
    void power(bool power);  // public?

    // debug

    void dump_status();
    void dump_addrs();

private:
    void init_gpio();
    void init_spi();

    uint8_t get_rx_length_for_pipe(uint8_t pipe);
    void get_payload(uint8_t &pipe, uint8_t *buffer, uint8_t &size);

    // SPI commands

    void cmd_nop();
    uint8_t cmd_read_register(uint8_t reg_addr);
    void cmd_read_register(uint8_t reg_addr, uint8_t* buffer, uint8_t n_bytes);
    void cmd_write_register(uint8_t reg_addr, uint8_t value);
    void cmd_write_register(uint8_t reg_addr, uint8_t const* buffer, uint8_t n_bytes);
    void cmd_read_payload(uint8_t *buffer, uint8_t n_bytes);
    void cmd_write_payload(uint8_t const* buffer, uint8_t n_bytes);
    void cmd_flush_rx_fifo();
    void cmd_flush_tx_fifo();
    void cmd_reuse_tx_payload();

    uint8_t spi_transfer_byte(uint8_t byte);

private:
    SPI_TypeDef* nRF_SPI;
    uint8_t af;
    Pin nrf_sck;   // AF
    Pin nrf_miso;  // AF
    Pin nrf_mosi;  // AF
    Pin nrf_csn;   // output: SPI ~CS
    Pin nrf_ce;    // output: activates the transciever (rx or tx mode)
    Pin nrf_irq;   // input: interrupt (active low, 3 sources)

    uint8_t status;
};

//extern NRF24L01 nrf24l01;

#endif // NRF24L01_H
