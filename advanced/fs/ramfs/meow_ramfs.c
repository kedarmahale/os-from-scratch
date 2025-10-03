/* advanced/fs/ramfs/meow_ramfs.c - RAM Filesystem Implementation
 *
 * Simple in-memory filesystem for MeowKernel
 * Copyright (c) 2025 MeowKernel Project
 */

#include "../vfs/meow_vfs.h"
#include "../../kernel/meow_util.h"
#include "../../hal/meow_hal_interface.h"
#include "../../mm/meow_heap_allocator.h"

/* ================================================================
 * RAMFS STRUCTURES
 * ================================================================ */

#define RAMFS_MAX_FILES     64
#define RAMFS_MAX_FILE_SIZE (64 * 1024)  /* 64KB per file */

typedef struct ramfs_file {
    char name[VFS_MAX_NAME];
    uint32_t size;
    uint32_t type;
    uint8_t* data;
    uint32_t created;
    uint32_t modified;
    uint8_t in_use;
} ramfs_file_t;

typedef struct ramfs_data {
    ramfs_file_t files[RAMFS_MAX_FILES];
    uint32_t file_count;
} ramfs_data_t;

/* Global RAMFS instance */
static ramfs_data_t ramfs_instance = {0};
static uint8_t ramfs_mounted = 0;

/* ================================================================
 * RAMFS INTERNAL FUNCTIONS
 * ================================================================ */

static ramfs_file_t* ramfs_find_file(const char* path)
{
    /* Skip leading slash */
    if (path[0] == '/') path++;
    
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_instance.files[i].in_use && 
            meow_strcmp(ramfs_instance.files[i].name, path) == 0) {
            return &ramfs_instance.files[i];
        }
    }
    
    return NULL;
}

static ramfs_file_t* ramfs_create_file(const char* path, uint32_t type)
{
    /* Skip leading slash */
    if (path[0] == '/') path++;
    
    /* Find free slot */
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!ramfs_instance.files[i].in_use) {
            ramfs_file_t* file = &ramfs_instance.files[i];
            
            meow_strncpy(file->name, path, VFS_MAX_NAME, VFS_MAX_NAME - 1);
            file->name[VFS_MAX_NAME - 1] = '\0';
            file->size = 0;
            file->type = type;
            file->data = NULL;
            file->created = HAL_TIMER_OP(get_ticks);
            file->modified = file->created;
            file->in_use = 1;
            
            ramfs_instance.file_count++;
            
            meow_log(MEOW_LOG_PURR, "📄 Created RAMFS file: %s", path);
            return file;
        }
    }
    
    return NULL;
}

/* ================================================================
 * RAMFS FILESYSTEM OPERATIONS
 * ================================================================ */

static meow_error_t ramfs_mount(const char* device, uint32_t flags)
{
    if (ramfs_mounted) {
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }
    
    /* Initialize RAMFS */
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        ramfs_instance.files[i].in_use = 0;
        ramfs_instance.files[i].data = NULL;
    }
    ramfs_instance.file_count = 0;
    
    /* Create root directory */
    ramfs_file_t* root = ramfs_create_file("", VFS_TYPE_DIRECTORY);
    if (!root) {
        return MEOW_ERROR_OUT_OF_MEMORY;
    }
    
    ramfs_mounted = 1;
    
    meow_log(MEOW_LOG_CHIRP, "📁 RAMFS mounted successfully");
    return MEOW_SUCCESS;
}

static meow_error_t ramfs_unmount(void)
{
    if (!ramfs_mounted) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    /* Free all file data */
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_instance.files[i].in_use && ramfs_instance.files[i].data) {
            meow_heap_free(ramfs_instance.files[i].data);
        }
    }
    
    ramfs_mounted = 0;
    
    meow_log(MEOW_LOG_PURR, "📁 RAMFS unmounted");
    return MEOW_SUCCESS;
}

/* ================================================================
 * RAMFS FILE OPERATIONS
 * ================================================================ */

static meow_error_t ramfs_open(const char* path, int flags, void** private_data)
{
    ramfs_file_t* file = ramfs_find_file(path);
    
    /* Create file if it doesn't exist and O_CREAT is set */
    if (!file && (flags & VFS_O_CREAT)) {
        file = ramfs_create_file(path, VFS_TYPE_REGULAR);
        if (!file) {
            return MEOW_ERROR_OUT_OF_MEMORY;
        }
    }
    
    if (!file) {
        return MEOW_ERROR_IO_FAILURE;
    }
    
    /* Allocate data buffer if needed */
    if (!file->data && file->type == VFS_TYPE_REGULAR) {
        file->data = meow_heap_alloc(RAMFS_MAX_FILE_SIZE);
        if (!file->data) {
            return MEOW_ERROR_OUT_OF_MEMORY;
        }
    }
    
    /* Truncate file if O_TRUNC is set */
    if (flags & VFS_O_TRUNC) {
        file->size = 0;
    }
    
    *private_data = file;
    return MEOW_SUCCESS;
}

static meow_error_t ramfs_close(void* private_data)
{
    /* Nothing special needed for RAMFS close */
    return MEOW_SUCCESS;
}

static ssize_t ramfs_read(void* private_data, void* buffer, size_t count, off_t offset)
{
    ramfs_file_t* file = (ramfs_file_t*)private_data;
    
    if (!file || !file->data || file->type != VFS_TYPE_REGULAR) {
        return -1;
    }
    
    /* Check bounds */
    if (offset >= file->size) {
        return 0; /* EOF */
    }
    
    /* Adjust count to not read past end of file */
    if (offset + count > file->size) {
        count = file->size - offset;
    }
    
    /* Copy data */
    meow_memcpy(buffer, file->data + offset, count);
    
    return count;
}

static ssize_t ramfs_write(void* private_data, const void* buffer, size_t count, off_t offset)
{
    ramfs_file_t* file = (ramfs_file_t*)private_data;
    
    if (!file || !file->data || file->type != VFS_TYPE_REGULAR) {
        return -1;
    }
    
    /* Check bounds */
    if (offset + count > RAMFS_MAX_FILE_SIZE) {
        count = RAMFS_MAX_FILE_SIZE - offset;
        if (count <= 0) {
            return -1; /* File too large */
        }
    }
    
    /* Copy data */
    meow_memcpy(file->data + offset, buffer, count);
    
    /* Update file size */
    if (offset + count > file->size) {
        file->size = offset + count;
    }
    
    /* Update modified time */
    file->modified = HAL_TIMER_OP(get_ticks);
    
    return count;
}

static off_t ramfs_lseek(void* private_data, off_t offset, int whence)
{
    ramfs_file_t* file = (ramfs_file_t*)private_data;
    
    if (!file) {
        return -1;
    }
    
    off_t new_pos;
    
    switch (whence) {
        case VFS_SEEK_SET:
            new_pos = offset;
            break;
        case VFS_SEEK_CUR:
            /* This would need current position from VFS layer */
            return -1;
        case VFS_SEEK_END:
            new_pos = file->size + offset;
            break;
        default:
            return -1;
    }
    
    if (new_pos < 0 || new_pos > RAMFS_MAX_FILE_SIZE) {
        return -1;
    }
    
    return new_pos;
}

/* ================================================================
 * RAMFS DIRECTORY OPERATIONS
 * ================================================================ */

static meow_error_t ramfs_mkdir(const char* path, mode_t mode)
{
    if (ramfs_find_file(path)) {
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }
    
    ramfs_file_t* dir = ramfs_create_file(path, VFS_TYPE_DIRECTORY);
    return dir ? MEOW_SUCCESS : MEOW_ERROR_OUT_OF_MEMORY;
}

static meow_error_t ramfs_rmdir(const char* path)
{
    ramfs_file_t* file = ramfs_find_file(path);
    
    if (!file) {
        return MEOW_ERROR_IO_FAILURE;
    }
    
    if (file->type != VFS_TYPE_DIRECTORY) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    /* Free file entry */
    if (file->data) {
        meow_heap_free(file->data);
    }
    
    file->in_use = 0;
    ramfs_instance.file_count--;
    
    return MEOW_SUCCESS;
}

/* ================================================================
 * RAMFS FILE MANAGEMENT
 * ================================================================ */

static meow_error_t ramfs_unlink(const char* path)
{
    ramfs_file_t* file = ramfs_find_file(path);
    
    if (!file) {
        return MEOW_ERROR_IO_FAILURE;
    }
    
    if (file->type == VFS_TYPE_DIRECTORY) {
        return MEOW_ERROR_INVALID_PARAMETER; /* Use rmdir for directories */
    }
    
    /* Free file data */
    if (file->data) {
        meow_heap_free(file->data);
    }
    
    file->in_use = 0;
    ramfs_instance.file_count--;
    
    meow_log(MEOW_LOG_PURR, "🗑️ Deleted RAMFS file: %s", path);
    return MEOW_SUCCESS;
}

static meow_error_t ramfs_stat(const char* path, vfs_stat_t* stat_buf)
{
    ramfs_file_t* file = ramfs_find_file(path);
    
    if (!file) {
        return MEOW_ERROR_IO_FAILURE;
    }
    
    stat_buf->size = file->size;
    stat_buf->type = file->type;
    stat_buf->mode = 0644; /* Default permissions */
    stat_buf->created = file->created;
    stat_buf->modified = file->modified;
    
    return MEOW_SUCCESS;
}

/* ================================================================
 * RAMFS FILESYSTEM DEFINITION
 * ================================================================ */

static vfs_filesystem_t ramfs_filesystem = {
    .name = "ramfs",
    .mount = ramfs_mount,
    .unmount = ramfs_unmount,
    .open = ramfs_open,
    .close = ramfs_close,
    .read = ramfs_read,
    .write = ramfs_write,
    .lseek = ramfs_lseek,
    .mkdir = ramfs_mkdir,
    .rmdir = ramfs_rmdir,
    .unlink = ramfs_unlink,
    .stat = ramfs_stat,
    .private_data = NULL
};

/* ================================================================
 * RAMFS INITIALIZATION
 * ================================================================ */

meow_error_t ramfs_init(void)
{
    meow_log(MEOW_LOG_MEOW, "💾 Initializing RAMFS...");
    
    /* Register with VFS */
    meow_error_t result = vfs_register_filesystem(&ramfs_filesystem);
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to register RAMFS");
        return result;
    }
    
    meow_log(MEOW_LOG_CHIRP, "😺 RAMFS initialized successfully");
    return MEOW_SUCCESS;
}

/* ================================================================
 * RAMFS UTILITY FUNCTIONS
 * ================================================================ */

void ramfs_list_files(void)
{
    meow_log(MEOW_LOG_CHIRP, "📁 RAMFS file listing:");
    
    if (ramfs_instance.file_count == 0) {
        meow_log(MEOW_LOG_PURR, "  (no files)");
        return;
    }
    
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        ramfs_file_t* file = &ramfs_instance.files[i];
        if (file->in_use) {
            const char* type_str = (file->type == VFS_TYPE_DIRECTORY) ? "DIR" : "FILE";
            meow_log(MEOW_LOG_PURR, "  %s  %8u  %s", 
                     type_str, file->size, file->name);
        }
    }
}