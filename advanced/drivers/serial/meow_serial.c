/* advanced/drivers/serial/meow_serial.c - Serial Port Driver
 *
 * Simple serial driver for debugging and communication
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_serial.h"
#include "../../kernel/meow_util.h"
#include "../../hal/meow_hal_interface.h"

/* ================================================================
 * SERIAL PORT CONSTANTS
 * ================================================================ */

/* COM port base addresses */
#define COM1_BASE   0x3F8
#define COM2_BASE   0x2F8
#define COM3_BASE   0x3E8
#define COM4_BASE   0x2E8

/* Register offsets from base address */
#define SERIAL_DATA_REG         0   /* Data register (read/write) */
#define SERIAL_IER_REG          1   /* Interrupt Enable Register */
#define SERIAL_IIR_REG          2   /* Interrupt Identification Register */
#define SERIAL_LCR_REG          3   /* Line Control Register */
#define SERIAL_MCR_REG          4   /* Modem Control Register */
#define SERIAL_LSR_REG          5   /* Line Status Register */
#define SERIAL_MSR_REG          6   /* Modem Status Register */
#define SERIAL_SCRATCH_REG      7   /* Scratch Register */

/* Divisor latch registers (when DLAB bit is set) */
#define SERIAL_DLL_REG          0   /* Divisor Latch Low */
#define SERIAL_DLH_REG          1   /* Divisor Latch High */

/* Line Status Register bits */
#define LSR_DATA_READY          0x01
#define LSR_OVERRUN_ERROR       0x02
#define LSR_PARITY_ERROR        0x04
#define LSR_FRAMING_ERROR       0x08
#define LSR_BREAK_INTERRUPT     0x10
#define LSR_THR_EMPTY           0x20
#define LSR_TRANSMITTER_EMPTY   0x40
#define LSR_FIFO_ERROR          0x80

/* Line Control Register bits */
#define LCR_WORD_LENGTH_5       0x00
#define LCR_WORD_LENGTH_6       0x01
#define LCR_WORD_LENGTH_7       0x02
#define LCR_WORD_LENGTH_8       0x03
#define LCR_STOP_BITS_1         0x00
#define LCR_STOP_BITS_2         0x04
#define LCR_PARITY_NONE         0x00
#define LCR_PARITY_ODD          0x08
#define LCR_PARITY_EVEN         0x18
#define LCR_PARITY_MARK         0x28
#define LCR_PARITY_SPACE        0x38
#define LCR_BREAK_ENABLE        0x40
#define LCR_DLAB                0x80

/* ================================================================
 * SERIAL DRIVER STATE
 * ================================================================ */

typedef struct serial_port {
    uint16_t base_addr;
    uint8_t initialized;
    uint32_t baud_rate;
    serial_stats_t stats;
} serial_port_t;

static serial_port_t serial_ports[4] = {
    { COM1_BASE, 0, 0, {0} },
    { COM2_BASE, 0, 0, {0} },
    { COM3_BASE, 0, 0, {0} },
    { COM4_BASE, 0, 0, {0} }
};

/* ================================================================
 * INTERNAL HELPER FUNCTIONS
 * ================================================================ */

static uint8_t serial_read_reg(serial_port_t* port, uint8_t reg)
{
    return HAL_IO_OP(inb, port->base_addr + reg);
}

static void serial_write_reg(serial_port_t* port, uint8_t reg, uint8_t value)
{
    HAL_IO_OP(outb, port->base_addr + reg, value);
}

static int serial_is_transmit_ready(serial_port_t* port)
{
    return (serial_read_reg(port, SERIAL_LSR_REG) & LSR_THR_EMPTY) != 0;
}

static int serial_is_data_available(serial_port_t* port)
{
    return (serial_read_reg(port, SERIAL_LSR_REG) & LSR_DATA_READY) != 0;
}

static serial_port_t* get_serial_port(uint8_t port_num)
{
    if (port_num >= 4) return NULL;
    return &serial_ports[port_num];
}

/* ================================================================
 * PUBLIC API IMPLEMENTATION
 * ================================================================ */

meow_error_t serial_init(uint8_t port_num, uint32_t baud_rate)
{
    serial_port_t* port = get_serial_port(port_num);
    if (!port) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    meow_log(MEOW_LOG_MEOW, "📡 Initializing serial port COM%u at %u baud...", 
             port_num + 1, baud_rate);
    
    /* Calculate divisor for baud rate */
    uint16_t divisor = 115200 / baud_rate;
    if (divisor == 0) divisor = 1;
    
    /* Disable interrupts */
    serial_write_reg(port, SERIAL_IER_REG, 0x00);
    
    /* Set DLAB to access divisor registers */
    serial_write_reg(port, SERIAL_LCR_REG, LCR_DLAB);
    
    /* Set baud rate divisor */
    serial_write_reg(port, SERIAL_DLL_REG, divisor & 0xFF);
    serial_write_reg(port, SERIAL_DLH_REG, (divisor >> 8) & 0xFF);
    
    /* Configure: 8 bits, no parity, 1 stop bit */
    serial_write_reg(port, SERIAL_LCR_REG, LCR_WORD_LENGTH_8 | LCR_STOP_BITS_1 | LCR_PARITY_NONE);
    
    /* Enable FIFO, clear them, with 14-byte threshold */
    serial_write_reg(port, SERIAL_IIR_REG, 0xC7);
    
    /* Enable IRQs, set RTS/DSR */
    serial_write_reg(port, SERIAL_MCR_REG, 0x0B);
    
    /* Test serial port by writing to scratch register */
    serial_write_reg(port, SERIAL_SCRATCH_REG, 0xAE);
    if (serial_read_reg(port, SERIAL_SCRATCH_REG) != 0xAE) {
        meow_log(MEOW_LOG_HISS, "😾 Serial port COM%u test failed - may not be present", 
                 port_num + 1);
        return MEOW_ERROR_HARDWARE_FAILURE;
    }
    
    /* Mark as initialized */
    port->initialized = 1;
    port->baud_rate = baud_rate;
    
    /* Clear statistics */
    port->stats.bytes_sent = 0;
    port->stats.bytes_received = 0;
    port->stats.send_errors = 0;
    port->stats.receive_errors = 0;
    
    meow_log(MEOW_LOG_CHIRP, "😺 Serial port COM%u initialized successfully", port_num + 1);
    return MEOW_SUCCESS;
}

meow_error_t serial_cleanup(uint8_t port_num)
{
    serial_port_t* port = get_serial_port(port_num);
    if (!port || !port->initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    /* Disable interrupts */
    serial_write_reg(port, SERIAL_IER_REG, 0x00);
    
    /* Reset modem control */
    serial_write_reg(port, SERIAL_MCR_REG, 0x00);
    
    port->initialized = 0;
    
    meow_log(MEOW_LOG_PURR, "😴 Serial port COM%u cleaned up", port_num + 1);
    return MEOW_SUCCESS;
}

meow_error_t serial_write_byte(uint8_t port_num, uint8_t data)
{
    serial_port_t* port = get_serial_port(port_num);
    if (!port || !port->initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    /* Wait for transmit buffer to be empty */
    uint32_t timeout = 10000;
    while (!serial_is_transmit_ready(port) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        port->stats.send_errors++;
        return MEOW_ERROR_TIMEOUT;
    }
    
    /* Send the byte */
    serial_write_reg(port, SERIAL_DATA_REG, data);
    port->stats.bytes_sent++;
    
    return MEOW_SUCCESS;
}

meow_error_t serial_read_byte(uint8_t port_num, uint8_t* data)
{
    serial_port_t* port = get_serial_port(port_num);
    if (!port || !port->initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    MEOW_RETURN_IF_NULL(data);
    
    /* Check if data is available */
    if (!serial_is_data_available(port)) {
        return MEOW_ERROR_IO_FAILURE;
    }
    
    /* Read the byte */
    *data = serial_read_reg(port, SERIAL_DATA_REG);
    port->stats.bytes_received++;
    
    /* Check for errors */
    uint8_t lsr = serial_read_reg(port, SERIAL_LSR_REG);
    if (lsr & (LSR_OVERRUN_ERROR | LSR_PARITY_ERROR | LSR_FRAMING_ERROR)) {
        port->stats.receive_errors++;
        meow_log(MEOW_LOG_HISS, "😾 Serial receive error on COM%u: LSR=0x%02X", 
                 port_num + 1, lsr);
    }
    
    return MEOW_SUCCESS;
}

meow_error_t serial_write_string(uint8_t port_num, const char* str)
{
    MEOW_RETURN_IF_NULL(str);
    
    meow_error_t result = MEOW_SUCCESS;
    while (*str && result == MEOW_SUCCESS) {
        result = serial_write_byte(port_num, *str);
        str++;
    }
    
    return result;
}

meow_error_t serial_read_string(uint8_t port_num, char* buffer, size_t buffer_size, size_t* bytes_read)
{
    MEOW_RETURN_IF_NULL(buffer);
    MEOW_RETURN_IF_NULL(bytes_read);
    
    if (buffer_size == 0) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    *bytes_read = 0;
    size_t pos = 0;
    
    while (pos < buffer_size - 1) {
        uint8_t byte;
        meow_error_t result = serial_read_byte(port_num, &byte);
        
        if (result == MEOW_ERROR_IO_FAILURE) {
            break; /* No more data available */
        } else if (result != MEOW_SUCCESS) {
            return result;
        }
        
        buffer[pos++] = byte;
        
        /* Stop at newline */
        if (byte == '\n' || byte == '\r') {
            break;
        }
    }
    
    buffer[pos] = '\0';
    *bytes_read = pos;
    
    return MEOW_SUCCESS;
}

int serial_data_available(uint8_t port_num)
{
    serial_port_t* port = get_serial_port(port_num);
    if (!port || !port->initialized) {
        return 0;
    }
    
    return serial_is_data_available(port) ? 1 : 0;
}

meow_error_t serial_get_stats(uint8_t port_num, serial_stats_t* stats)
{
    serial_port_t* port = get_serial_port(port_num);
    if (!port || !port->initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    MEOW_RETURN_IF_NULL(stats);
    
    *stats = port->stats;
    return MEOW_SUCCESS;
}

meow_error_t serial_set_baud_rate(uint8_t port_num, uint32_t baud_rate)
{
    serial_port_t* port = get_serial_port(port_num);
    if (!port || !port->initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    /* Calculate divisor */
    uint16_t divisor = 115200 / baud_rate;
    if (divisor == 0) divisor = 1;
    
    /* Save current LCR */
    uint8_t lcr = serial_read_reg(port, SERIAL_LCR_REG);
    
    /* Set DLAB to access divisor registers */
    serial_write_reg(port, SERIAL_LCR_REG, lcr | LCR_DLAB);
    
    /* Set new baud rate divisor */
    serial_write_reg(port, SERIAL_DLL_REG, divisor & 0xFF);
    serial_write_reg(port, SERIAL_DLH_REG, (divisor >> 8) & 0xFF);
    
    /* Restore LCR */
    serial_write_reg(port, SERIAL_LCR_REG, lcr);
    
    port->baud_rate = baud_rate;
    
    meow_log(MEOW_LOG_PURR, "📡 Serial COM%u baud rate changed to %u", 
             port_num + 1, baud_rate);
    
    return MEOW_SUCCESS;
}

/* ================================================================
 * CONVENIENCE FUNCTIONS
 * ================================================================ */
meow_error_t serial_printf(uint8_t port_num, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    // Use meow_printf directly with variadic args
    meow_printf(format, args);

    va_end(args);

    // Send the formatted string through serial
    return serial_write_string(port_num, NULL); // Assuming serial_write_string will send data previously prepared
}

/* ================================================================
 * DEBUG OUTPUT INTEGRATION
 * ================================================================ */

void serial_debug_init(void)
{
    /* Initialize COM1 for debug output */
    if (serial_init(0, 9600) == MEOW_SUCCESS) {
        meow_log(MEOW_LOG_CHIRP, "📡 Serial debug output enabled on COM1");
        
        /* Send initial message */
        serial_write_string(0, "\r\n=== MeowKernel Phase 2 Debug Output ===\r\n");
        serial_write_string(0, "Serial debug interface initialized\r\n");
    }
}

void serial_debug_write(const char* message)
{
    if (serial_ports[0].initialized) {
        serial_write_string(0, message);
    }
}

void serial_debug_log(meow_log_level_t level, const char* message)
{
    if (!serial_ports[0].initialized) return;
    
    const char* level_str;
    switch (level) {
        case MEOW_LOG_PURR: level_str = "PURR"; break;
        case MEOW_LOG_MEOW: level_str = "MEOW"; break;
        case MEOW_LOG_CHIRP: level_str = "CHIRP"; break;
        case MEOW_LOG_HISS: level_str = "HISS"; break;
        case MEOW_LOG_YOWL: level_str = "YOWL"; break;
        case MEOW_LOG_SCREECH: level_str = "SCREECH"; break;
        default: level_str = "UNKNOWN"; break;
    }
    
    serial_printf(0, "[%s] %s\r\n", level_str, message);
}