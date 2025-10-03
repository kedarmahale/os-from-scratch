/* advanced/fs/vfs/meow_vfs.c - Virtual File System Layer
 *
 * Basic VFS implementation for MeowKernel
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_vfs.h"
#include "../../kernel/meow_util.h"
#include "../../drivers/keyboard/meow_keyboard.h"

/* ================================================================
 * VFS GLOBAL STATE
 * ================================================================ */

#define MAX_FILESYSTEMS     8
#define MAX_OPEN_FILES      64
#define MAX_MOUNT_POINTS    16

/* Registered filesystems */
static vfs_filesystem_t* registered_filesystems[MAX_FILESYSTEMS];
static uint32_t filesystem_count = 0;

/* Open file table */
static vfs_file_t open_files[MAX_OPEN_FILES];
static uint32_t next_fd = 3; /* Start after stdin/stdout/stderr */

/* Mount points */
static vfs_mount_t mount_points[MAX_MOUNT_POINTS];
static uint32_t mount_count = 0;

/* Root filesystem */
static vfs_filesystem_t* root_fs = NULL;

/* ================================================================
 * VFS INITIALIZATION
 * ================================================================ */

meow_error_t vfs_init(void)
{
    meow_log(MEOW_LOG_MEOW, "📁 Initializing Virtual File System...");
    
    /* Clear registered filesystems */
    for (uint32_t i = 0; i < MAX_FILESYSTEMS; i++) {
        registered_filesystems[i] = NULL;
    }
    filesystem_count = 0;
    
    /* Clear open file table */
    for (uint32_t i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].fd = -1;
        open_files[i].filesystem = NULL;
        open_files[i].private_data = NULL;
        open_files[i].position = 0;
        open_files[i].flags = 0;
    }
    next_fd = 3;
    
    /* Clear mount points */
    for (uint32_t i = 0; i < MAX_MOUNT_POINTS; i++) {
        mount_points[i].path[0] = '\0';
        mount_points[i].filesystem = NULL;
        mount_points[i].flags = 0;
    }
    mount_count = 0;
    root_fs = NULL;
    
    /* Setup standard file descriptors */
    open_files[0].fd = 0; /* stdin */
    open_files[1].fd = 1; /* stdout */  
    open_files[2].fd = 2; /* stderr */
    
    meow_log(MEOW_LOG_CHIRP, "😺 VFS initialized - ready to handle files!");
    return MEOW_SUCCESS;
}

/* ================================================================
 * FILESYSTEM REGISTRATION
 * ================================================================ */

meow_error_t vfs_register_filesystem(vfs_filesystem_t* fs)
{
    MEOW_RETURN_IF_NULL(fs);
    MEOW_RETURN_IF_NULL(fs->name);
    
    if (filesystem_count >= MAX_FILESYSTEMS) {
        return MEOW_ERROR_OUT_OF_MEMORY;
    }
    
    registered_filesystems[filesystem_count++] = fs;
    
    meow_log(MEOW_LOG_CHIRP, "📁 Registered filesystem: %s", fs->name);
    return MEOW_SUCCESS;
}

vfs_filesystem_t* vfs_find_filesystem(const char* name)
{
    MEOW_RETURN_VALUE_IF_NULL(name, NULL);
    
    for (uint32_t i = 0; i < filesystem_count; i++) {
        if (meow_strcmp(registered_filesystems[i]->name, name) == 0) {
            return registered_filesystems[i];
        }
    }
    
    return NULL;
}

/* ================================================================
 * MOUNT POINT MANAGEMENT
 * ================================================================ */

meow_error_t vfs_mount(const char* device, const char* mountpoint, 
                       const char* fstype, uint32_t flags)
{
    MEOW_RETURN_IF_NULL(mountpoint);
    MEOW_RETURN_IF_NULL(fstype);
    
    if (mount_count >= MAX_MOUNT_POINTS) {
        return MEOW_ERROR_OUT_OF_MEMORY;
    }
    
    /* Find filesystem type */
    vfs_filesystem_t* fs = vfs_find_filesystem(fstype);
    if (!fs) {
        meow_log(MEOW_LOG_HISS, "😾 Unknown filesystem type: %s", fstype);
        return MEOW_ERROR_IO_FAILURE;
    }
    
    /* Initialize filesystem if needed */
    if (fs->mount && fs->mount(device, flags) != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to mount %s filesystem", fstype);
        return MEOW_ERROR_IO_FAILURE;
    }
    
    /* Add to mount table */
    vfs_mount_t* mount = &mount_points[mount_count++];
    meow_strncpy(mount->path, mountpoint, VFS_MAX_PATH, VFS_MAX_PATH - 1);
    mount->path[VFS_MAX_PATH - 1] = '\0';
    mount->filesystem = fs;
    mount->flags = flags;
    
    /* Set as root if mounting at "/" */
    if (meow_strcmp(mountpoint, "/") == 0) {
        root_fs = fs;
    }
    
    meow_log(MEOW_LOG_CHIRP, "📁 Mounted %s at %s", fstype, mountpoint);
    return MEOW_SUCCESS;
}

vfs_mount_t* vfs_find_mount(const char* path)
{
    MEOW_RETURN_VALUE_IF_NULL(path, NULL);
    
    size_t best_match_len = 0;
    vfs_mount_t* best_match = NULL;
    
    for (uint32_t i = 0; i < mount_count; i++) {
        size_t mount_len = meow_strlen(mount_points[i].path);
        
        if (meow_strncmp(path, mount_points[i].path, mount_len) == 0) {
            if (mount_len > best_match_len) {
                best_match_len = mount_len;
                best_match = &mount_points[i];
            }
        }
    }
    
    return best_match;
}

/* ================================================================
 * FILE OPERATIONS
 * ================================================================ */

int vfs_open(const char* path, int flags)
{
    MEOW_RETURN_VALUE_IF_NULL(path, -1);
    
    /* Find available file descriptor */
    int fd = -1;
    for (uint32_t i = 3; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].fd == -1) {
            fd = next_fd++;
            open_files[i].fd = fd;
            break;
        }
    }
    
    if (fd == -1) {
        meow_log(MEOW_LOG_HISS, "😾 No available file descriptors");
        return -1;
    }
    
    /* Find mount point */
    vfs_mount_t* mount = vfs_find_mount(path);
    if (!mount) {
        meow_log(MEOW_LOG_HISS, "😾 No filesystem mounted for path: %s", path);
        return -1;
    }
    
    /* Call filesystem open operation */
    if (mount->filesystem->open) {
        void* private_data = NULL;
        meow_error_t result = mount->filesystem->open(path, flags, &private_data);
        
        if (result != MEOW_SUCCESS) {
            meow_log(MEOW_LOG_HISS, "😾 Failed to open file: %s", path);
            return -1;
        }
        
        /* Setup file descriptor */
        for (uint32_t i = 3; i < MAX_OPEN_FILES; i++) {
            if (open_files[i].fd == fd) {
                open_files[i].filesystem = mount->filesystem;
                open_files[i].private_data = private_data;
                open_files[i].position = 0;
                open_files[i].flags = flags;
                break;
            }
        }
    }
    
    meow_log(MEOW_LOG_PURR, "📄 Opened file: %s (fd=%d)", path, fd);
    return fd;
}

int vfs_close(int fd)
{
    vfs_file_t* file = vfs_get_file(fd);
    if (!file) {
        return -1;
    }
    
    /* Call filesystem close operation */
    if (file->filesystem && file->filesystem->close) {
        file->filesystem->close(file->private_data);
    }
    
    /* Clear file descriptor */
    file->fd = -1;
    file->filesystem = NULL;
    file->private_data = NULL;
    file->position = 0;
    file->flags = 0;
    
    meow_log(MEOW_LOG_PURR, "📄 Closed file descriptor %d", fd);
    return 0;
}

ssize_t vfs_read(int fd, void* buffer, size_t count)
{
    MEOW_RETURN_VALUE_IF_NULL(buffer, -1);
    
    vfs_file_t* file = vfs_get_file(fd);
    if (!file) {
        return -1;
    }
    
    /* Handle special file descriptors */
    if (fd == 0) { /* stdin - read from keyboard */
        return keyboard_gets((char*)buffer, count);
    }
    
    /* Call filesystem read operation */
    if (file->filesystem && file->filesystem->read) {
        ssize_t bytes_read = file->filesystem->read(file->private_data, buffer, count, file->position);
        if (bytes_read > 0) {
            file->position += bytes_read;
        }
        return bytes_read;
    }
    
    return -1;
}

ssize_t vfs_write(int fd, const void* buffer, size_t count)
{
    MEOW_RETURN_VALUE_IF_NULL(buffer, -1);
    
    vfs_file_t* file = vfs_get_file(fd);
    
    /* Handle special file descriptors */
    if (fd == 1 || fd == 2) { /* stdout/stderr - write to VGA */
        const char* str = (const char*)buffer;
        for (size_t i = 0; i < count && str[i] != '\0'; i++) {
            meow_putc(str[i]);
        }
        return count;
    }
    
    if (!file) {
        return -1;
    }
    
    /* Call filesystem write operation */
    if (file->filesystem && file->filesystem->write) {
        ssize_t bytes_written = file->filesystem->write(file->private_data, buffer, count, file->position);
        if (bytes_written > 0) {
            file->position += bytes_written;
        }
        return bytes_written;
    }
    
    return -1;
}

off_t vfs_lseek(int fd, off_t offset, int whence)
{
    vfs_file_t* file = vfs_get_file(fd);
    if (!file) {
        return -1;
    }
    
    /* Call filesystem seek operation */
    if (file->filesystem && file->filesystem->lseek) {
        off_t new_pos = file->filesystem->lseek(file->private_data, offset, whence);
        if (new_pos >= 0) {
            file->position = new_pos;
        }
        return new_pos;
    }
    
    /* Default seek behavior */
    switch (whence) {
        case VFS_SEEK_SET:
            file->position = offset;
            break;
        case VFS_SEEK_CUR:
            file->position += offset;
            break;
        case VFS_SEEK_END:
            /* Would need file size - not implemented */
            return -1;
        default:
            return -1;
    }
    
    return file->position;
}

/* ================================================================
 * DIRECTORY OPERATIONS
 * ================================================================ */

int vfs_mkdir(const char* path, mode_t mode)
{
    MEOW_RETURN_VALUE_IF_NULL(path, -1);
    
    vfs_mount_t* mount = vfs_find_mount(path);
    if (!mount) {
        return -1;
    }
    
    if (mount->filesystem && mount->filesystem->mkdir) {
        return mount->filesystem->mkdir(path, mode) == MEOW_SUCCESS ? 0 : -1;
    }
    
    return -1;
}

int vfs_rmdir(const char* path)
{
    MEOW_RETURN_VALUE_IF_NULL(path, -1);
    
    vfs_mount_t* mount = vfs_find_mount(path);
    if (!mount) {
        return -1;
    }
    
    if (mount->filesystem && mount->filesystem->rmdir) {
        return mount->filesystem->rmdir(path) == MEOW_SUCCESS ? 0 : -1;
    }
    
    return -1;
}

/* ================================================================
 * FILE MANAGEMENT
 * ================================================================ */

int vfs_unlink(const char* path)
{
    MEOW_RETURN_VALUE_IF_NULL(path, -1);
    
    vfs_mount_t* mount = vfs_find_mount(path);
    if (!mount) {
        return -1;
    }
    
    if (mount->filesystem && mount->filesystem->unlink) {
        return mount->filesystem->unlink(path) == MEOW_SUCCESS ? 0 : -1;
    }
    
    return -1;
}

int vfs_stat(const char* path, vfs_stat_t* stat_buf)
{
    MEOW_RETURN_VALUE_IF_NULL(path, -1);
    MEOW_RETURN_VALUE_IF_NULL(stat_buf, -1);
    
    vfs_mount_t* mount = vfs_find_mount(path);
    if (!mount) {
        return -1;
    }
    
    if (mount->filesystem && mount->filesystem->stat) {
        return mount->filesystem->stat(path, stat_buf) == MEOW_SUCCESS ? 0 : -1;
    }
    
    return -1;
}

/* ================================================================
 * UTILITY FUNCTIONS
 * ================================================================ */

vfs_file_t* vfs_get_file(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].fd == fd) {
            return &open_files[i];
        }
    }
    
    return NULL;
}

void vfs_list_mounts(void)
{
    meow_log(MEOW_LOG_CHIRP, "📁 Mounted filesystems:");
    
    if (mount_count == 0) {
        meow_log(MEOW_LOG_PURR, "  (no filesystems mounted)");
        return;
    }
    
    for (uint32_t i = 0; i < mount_count; i++) {
        meow_log(MEOW_LOG_PURR, "  %s -> %s", 
                 mount_points[i].path, 
                 mount_points[i].filesystem->name);
    }
}