/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2021. All rights reserved.
/// @author mn-sc
///
/// @brief tiny xmodem loader

#include "xmodem.h"
#include "uart.h"
#include "rtc.h"
#include "crc.h"

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_1S 1000
#define MAXRETRANS 25

static inline int _inbyte(unsigned timeout_msec)
{
    int c;
    unsigned delay = timeout_msec << 7;
    do {
        c = sc1f_uart_getch_nowait();
        if (c < 0) {
            rtc_delay_us(10);
            if (timeout_msec && !delay) {
                c = -2;
                break;
            }
            --delay;
        }
    } while (c < 0);

    return c;
}

static inline void _outbyte(int c)
{
    sc1f_uart_put(c);
}

static void flushinput(void)
{
    while (_inbyte(DLY_1S) >= 0);
}

static void xmodem_send_can(void)
{
    _outbyte(CAN);
    _outbyte(CAN);
    _outbyte(CAN);
    flushinput();
}

int xmodem_receive(uint8_t *dest, unsigned destsz)
{
    int bufsz/*, crc = 0*/;
    unsigned char trychar = 'C';
    int c;

    uint8_t *buf = dest;
    uint16_t partial_crc;
    uint8_t seq[2];
    uint8_t seqnum = 1;
    uint16_t buf_crc = 0;
    int pktsize;
    int rxsize;

    int totalbytes = 0;

    int retry = MAXRETRANS;

    for (;;) {
        // wait for control byte
        while (1) {
            if (trychar) _outbyte(trychar);
            if ((c = _inbyte((DLY_1S)*3/2)) >= 0) {
                switch (c) {
                case SOH:
                    pktsize = bufsz = 128;
                    partial_crc = 0;
                    rxsize = -2;
                    goto start_recv;
                case STX:
                    pktsize = bufsz = 1024;
                    partial_crc = 0;
                    rxsize = -2;
                    goto start_recv;
                /* case 0x3: */
                /*     return -2; // ^C pressed */
                case EOT:
                    if (!totalbytes) // ^D pressed
                        return -2;
                    // transmition done
                    // send ACK and finish
                    _outbyte(ACK);
                    flushinput();
                    return totalbytes;
                case CAN:
                    // transmition cancelled by remote host
                    if ((c = _inbyte(DLY_1S)) == CAN) {
                        _outbyte(ACK);
                        flushinput();
                        return -1;
                    }
                    break;
                default:
                    break;
                }
            }
        }

    start_recv:
        /* if (trychar == 'C') crc = 1; */
        trychar = 0;
        // receive packet
        while (1) {
            c = _inbyte(DLY_1S);
            if (c < 0) {
                _outbyte(NAK);
                flushinput();
                break;
            }
            if (rxsize < 0) {
                seq[2 + rxsize] = c;
            } else if (rxsize < pktsize) {
                if (totalbytes + pktsize <= destsz)
                    buf[rxsize] = c;
                partial_crc = crc16_ccitt_update(partial_crc, c);
            } else if (rxsize == pktsize) {
                buf_crc = (c & 0xff) << 8;
            } else {
                // last crc byte
                buf_crc |= c & 0xff;

                if (seq[0] != (uint8_t)~seq[1]) {
                    // integrity error
                    _outbyte(NAK);
                    flushinput();
                    break;
                } else if (buf_crc != partial_crc) {
                    // CRC error
                    _outbyte(NAK);
                    flushinput();
                    break;
                } else if (seqnum == seq[0] + 1) {
                    // retransmission of the last packet
                    // ignore it
                    if (--retry <= 0) {
                        // too many retry error
                        xmodem_send_can();
                        return -3;
                    }
                    _outbyte(ACK);
                    break;
                } else if (seqnum == seq[0]) {
                    ++seqnum;
                    buf += pktsize;
                    totalbytes += pktsize;
                    retry = MAXRETRANS; // reset retries
                    _outbyte(ACK);
                    break;
                } else {
                    // out of sync
                    xmodem_send_can();
                    return -2;
                }
            }
            ++rxsize;
        }
    }
}
