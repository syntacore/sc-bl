/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2021. All rights reserved.
/// @author mn-sc
///
/// @brief tiny xmodem loader

#ifndef SCR_INFRA_XMODEM_H
#define SCR_INFRA_XMODEM_H

#include <stdint.h>

int xmodem_receive(uint8_t *buf, unsigned buf_sz);

#endif // SCR_INFRA_XMODEM_H
