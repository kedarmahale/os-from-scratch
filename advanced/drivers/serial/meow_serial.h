/* advanced/drivers/serial/meow_serial.h - Serial Port Driver Header
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_SERIAL_H
#define MEOW_SERIAL_H

#include <stdint.h>
#include <stdarg.h>
#include "../../kernel/meow_error_definitions.h"
#include "../../kernel/meow_util.h"

/* Serial port statistics */
typedef struct serial_stats {
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t send_errors;
    uint32_t receive_errors;
} serial_stats_t;

/* Serial port management */
meow_error_t serial_init(uint8_t port_num, uint32_t baud_rate);
meow_error_t serial_cleanup(uint8_t port_num);
meow_error_t serial_set_baud_rate(uint8_t port_num, uint32_t baud_rate);

/* Basic I/O operations */
meow_error_t serial_write_byte(uint8_t port_num, uint8_t data);
meow_error_t serial_read_byte(uint8_t port_num, uint8_t* data);
meow_error_t serial_write_string(uint8_t port_num, const char* str);
meow_error_t serial_read_string(uint8_t port_num, char* buffer, size_t buffer_size, size_t* bytes_read);

/* Status and statistics */
int serial_data_available(uint8_t port_num);
meow_error_t serial_get_stats(uint8_t port_num, serial_stats_t* stats);

/* Formatted output */
meow_error_t serial_printf(uint8_t port_num, const char* format, ...);

/* Debug output integration */
void serial_debug_init(void);
void serial_debug_write(const char* message);
void serial_debug_log(meow_log_level_t level, const char* message);

#endif /* MEOW_SERIAL_H */