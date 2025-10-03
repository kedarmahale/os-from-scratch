/* advanced/fs/devfs/meow_devfs.c - Device Filesystem Implementation
 *
 * Virtual device filesystem for MeowKernel
 * Copyright (c) 2025 MeowKernel Project
 */

#include "../vfs/meow_vfs.h"
#include "../../kernel/meow_util.h"
#include "../../drivers/keyboard/meow_keyboard.h"
#include "../../drivers/serial/meow_serial.h"

/* ================================================================
 * DEVFS DEVICE TYPES
 * ================================================================ */

typedef enum devfs_device_type {
    DEVFS_DEV_NULL = 0,
    DEVFS_DEV_ZERO,
    DEVFS_DEV_RANDOM,
    DEVFS_DEV_CONSOLE,
    DEVFS_DEV_KEYBOARD,
    DEVFS_DEV_SERIAL,
    DEVFS_DEV_MEMORY
} devfs_device_type_t;

typedef struct devfs_device {
    char name[VFS_MAX_NAME];
    devfs_device_type_t type;
    uint32_t minor;
    uint8_t readable;
    uint8_t writable;
} devfs_device_t;

/* Device table */
static devfs_device_t devfs_devices[] = {
    {"null",     DEVFS_DEV_NULL,     0, 1, 1},
    {"zero",     DEVFS_DEV_ZERO,     0, 1, 1},
    {"random",   DEVFS_DEV_RANDOM,   0, 1, 0},
    {"console",  DEVFS_DEV_CONSOLE,  0, 1, 1},
    {"keyboard", DEVFS_DEV_KEYBOARD, 0, 1, 0},
    {"ttyS0",    DEVFS_DEV_SERIAL,   0, 1, 1},
    {"ttyS1",    DEVFS_DEV_SERIAL,   1, 1, 1},
    {"mem",      DEVFS_DEV_MEMORY,   0, 1, 1},
};

#define DEVFS_DEVICE_COUNT (sizeof(devfs_devices) / sizeof(devfs_devices[0]))

static uint8_t devfs_mounted = 0;
static uint32_t random_seed = 12345;

/* ================================================================
 * DEVFS INTERNAL FUNCTIONS
 * ================================================================ */

static devfs_device_t* devfs_find_device(const char* path)
{
    /* Skip leading slash and "dev/" */
    if (path[0] == '/') path++;
    if (meow_strncmp(path, "dev/", 4) == 0) path += 4;
    
    for (uint32_t i = 0; i < DEVFS_DEVICE_COUNT; i++) {
        if (meow_strcmp(devfs_devices[i].name, path) == 0) {
            return &devfs_devices[i];
        }
    }
    
    return NULL;
}

static uint32_t devfs_simple_random(void)
{
    random_seed = random_seed * 1103515245 + 12345;
    return random_seed;
}

/* ================================================================
 * DEVFS FILESYSTEM OPERATIONS
 * ================================================================ */

static meow_error_t devfs_mount(const char* device, uint32_t flags)
{
    if (devfs_mounted) {
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }
    
    devfs_mounted = 1;
    random_seed = HAL_TIMER_OP(get_ticks);
    
    meow_log(MEOW_LOG_CHIRP, "🔌 DevFS mounted with %u devices", DEVFS_DEVICE_COUNT);
    return MEOW_SUCCESS;
}

static meow_error_t devfs_unmount(void)
{
    devfs_mounted = 0;
    return MEOW_SUCCESS;
}

/* ================================================================
 * DEVFS FILE OPERATIONS
 * ================================================================ */

static meow_error_t devfs_open(const char* path, int flags, void** private_data)
{
    devfs_device_t* dev = devfs_find_device(path);
    
    if (!dev) {
        return MEOW_ERROR_DEVICE_NOT_FOUND;
    }
    
    /* Check permissions */
    if ((flags & VFS_O_WRONLY) && !dev->writable) {
        return MEOW_ERROR_ACCESS_DENIED;
    }
    
    if ((flags & VFS_O_RDONLY) && !dev->readable) {
        return MEOW_ERROR_ACCESS_DENIED;
    }
    
    *private_data = dev;
    
    meow_log(MEOW_LOG_PURR, "🔌 Opened device: /dev/%s", dev->name);
    return MEOW_SUCCESS;
}

static meow_error_t devfs_close(void* private_data)
{
    /* Nothing special needed for device close */
    return MEOW_SUCCESS;
}

static ssize_t devfs_read(void* private_data, void* buffer, size_t count, off_t offset)
{
    devfs_device_t* dev = (devfs_device_t*)private_data;
    
    if (!dev || !dev->readable) {
        return -1;
    }
    
    switch (dev->type) {
        case DEVFS_DEV_NULL:
            return 0; /* Always EOF */
            
        case DEVFS_DEV_ZERO:
            meow_memset(buffer, 0, count);
            return count;
            
        case DEVFS_DEV_RANDOM: {
            uint8_t* buf = (uint8_t*)buffer;
            for (size_t i = 0; i < count; i++) {
                buf[i] = devfs_simple_random() & 0xFF;
            }
            return count;
        }
        
        case DEVFS_DEV_CONSOLE:
        case DEVFS_DEV_KEYBOARD: {
            /* Read from keyboard */
            char* str_buf = (char*)buffer;
            if (count > 0) {
                str_buf[0] = keyboard_getchar();
                return 1;
            }
            return 0;
        }
        
        case DEVFS_DEV_SERIAL: {
            /* Read from serial port */
            uint8_t* buf = (uint8_t*)buffer;
            size_t bytes_read = 0;
            
            while (bytes_read < count && serial_data_available(dev->minor)) {
                if (serial_read_byte(dev->minor, &buf[bytes_read]) == MEOW_SUCCESS) {
                    bytes_read++;
                } else {
                    break;
                }
            }
            
            return bytes_read;
        }
        
        case DEVFS_DEV_MEMORY:
            /* Direct memory access - dangerous! */
            if (offset >= 0x100000 && offset < 0x200000) { /* Limit to safe range */
                meow_memcpy(buffer, (void*)(uintptr_t)offset, count);
                return count;
            }
            return -1;
            
        default:
            return -1;
    }
}

static ssize_t devfs_write(void* private_data, const void* buffer, size_t count, off_t offset)
{
    devfs_device_t* dev = (devfs_device_t*)private_data;
    
    if (!dev || !dev->writable) {
        return -1;
    }
    
    switch (dev->type) {
        case DEVFS_DEV_NULL:
            return count; /* Discard all data */
            
        case DEVFS_DEV_ZERO:
            return count; /* Ignore writes */
            
        case DEVFS_DEV_CONSOLE: {
            /* Write to VGA console */
            const char* str = (const char*)buffer;
            for (size_t i = 0; i < count && str[i] != '\0'; i++) {
                meow_putc(str[i]);
            }
            return count;
        }
        
        case DEVFS_DEV_SERIAL: {
            /* Write to serial port */
            const uint8_t* buf = (const uint8_t*)buffer;
            size_t bytes_written = 0;
            
            for (size_t i = 0; i < count; i++) {
                if (serial_write_byte(dev->minor, buf[i]) == MEOW_SUCCESS) {
                    bytes_written++;
                } else {
                    break;
                }
            }
            
            return bytes_written;
        }
        
        case DEVFS_DEV_MEMORY:
            /* Direct memory access - dangerous! */
            if (offset >= 0x100000 && offset < 0x200000) { /* Limit to safe range */
                meow_memcpy((void*)(uintptr_t)offset, buffer, count);
                return count;
            }
            return -1;
            
        default:
            return -1;
    }
}

static off_t devfs_lseek(void* private_data, off_t offset, int whence)
{
    devfs_device_t* dev = (devfs_device_t*)private_data;
    
    /* Most devices don't support seeking */
    switch (dev->type) {
        case DEVFS_DEV_MEMORY:
            /* Memory device supports seeking */
            switch (whence) {
                case VFS_SEEK_SET:
                    return offset;
                case VFS_SEEK_CUR:
                case VFS_SEEK_END:
                    /* Would need current position from VFS */
                    return -1;
                default:
                    return -1;
            }
            
        default:
            return -1; /* Seeking not supported */
    }
}

/* ================================================================
 * DEVFS DIRECTORY AND FILE MANAGEMENT
 * ================================================================ */

static meow_error_t devfs_mkdir(const char* path, mode_t mode)
{
    return MEOW_ERROR_NOT_SUPPORTED; /* DevFS is read-only structure */
}

static meow_error_t devfs_rmdir(const char* path)
{
    return MEOW_ERROR_NOT_SUPPORTED; /* DevFS is read-only structure */
}

static meow_error_t devfs_unlink(const char* path)
{
    return MEOW_ERROR_NOT_SUPPORTED; /* DevFS devices can't be deleted */
}

static meow_error_t devfs_stat(const char* path, vfs_stat_t* stat_buf)
{
    devfs_device_t* dev = devfs_find_device(path);
    
    if (!dev) {
        return MEOW_ERROR_DEVICE_NOT_FOUND;
    }
    
    stat_buf->size = 0; /* Devices have no fixed size */
    stat_buf->type = VFS_TYPE_DEVICE;
    stat_buf->mode = 0644; /* Default device permissions */
    stat_buf->created = 0;
    stat_buf->modified = 0;
    
    return MEOW_SUCCESS;
}

/* ================================================================
 * DEVFS FILESYSTEM DEFINITION
 * ================================================================ */

static vfs_filesystem_t devfs_filesystem = {
    .name = "devfs",
    .mount = devfs_mount,
    .unmount = devfs_unmount,
    .open = devfs_open,
    .close = devfs_close,
    .read = devfs_read,
    .write = devfs_write,
    .lseek = devfs_lseek,
    .mkdir = devfs_mkdir,
    .rmdir = devfs_rmdir,
    .unlink = devfs_unlink,
    .stat = devfs_stat,
    .private_data = NULL
};

/* ================================================================
 * DEVFS INITIALIZATION
 * ================================================================ */

meow_error_t devfs_init(void)
{
    meow_log(MEOW_LOG_MEOW, "🔌 Initializing DevFS...");
    
    /* Register with VFS */
    meow_error_t result = vfs_register_filesystem(&devfs_filesystem);
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to register DevFS");
        return result;
    }
    
    meow_log(MEOW_LOG_CHIRP, "😺 DevFS initialized with %u devices", DEVFS_DEVICE_COUNT);
    return MEOW_SUCCESS;
}

/* ================================================================
 * DEVFS UTILITY FUNCTIONS
 * ================================================================ */

void devfs_list_devices(void)
{
    meow_log(MEOW_LOG_CHIRP, "🔌 Available devices:");
    
    for (uint32_t i = 0; i < DEVFS_DEVICE_COUNT; i++) {
        devfs_device_t* dev = &devfs_devices[i];
        char perms[4] = "---";
        if (dev->readable) perms[0] = 'r';
        if (dev->writable) perms[1] = 'w';
        
        meow_log(MEOW_LOG_PURR, "  /dev/%-12s  %s", dev->name, perms);
    }
}