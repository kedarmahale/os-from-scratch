/* advanced/fs/vfs/meow_vfs.h - Virtual File System Interface
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_VFS_H
#define MEOW_VFS_H

#include <stdint.h>
#include <stddef.h>
#include "../../kernel/meow_error_definitions.h"

/* ================================================================
 * VFS CONSTANTS AND TYPES
 * ================================================================ */

#define VFS_MAX_PATH        256
#define VFS_MAX_NAME        64

/* File types */
#define VFS_TYPE_REGULAR    1
#define VFS_TYPE_DIRECTORY  2
#define VFS_TYPE_DEVICE     3
#define VFS_TYPE_LINK       4

/* File flags */
#define VFS_O_RDONLY        0x00
#define VFS_O_WRONLY        0x01
#define VFS_O_RDWR          0x02
#define VFS_O_CREAT         0x04
#define VFS_O_APPEND        0x08
#define VFS_O_TRUNC         0x10

/* Seek types */
#define VFS_SEEK_SET        0
#define VFS_SEEK_CUR        1
#define VFS_SEEK_END        2

/* Mount flags */
#define VFS_MOUNT_RDONLY    0x01
#define VFS_MOUNT_NODEV     0x02

/* Types */
typedef int32_t ssize_t;
typedef uint32_t mode_t;
typedef uint32_t off_t;

/* File statistics */
typedef struct vfs_stat {
    uint32_t size;
    uint32_t type;
    mode_t mode;
    uint32_t created;
    uint32_t modified;
} vfs_stat_t;

/* Forward declarations */
struct vfs_filesystem;
struct vfs_file;
struct vfs_mount;

/* ================================================================
 * FILESYSTEM OPERATIONS STRUCTURE
 * ================================================================ */

typedef struct vfs_filesystem {
    char name[VFS_MAX_NAME];
    
    /* Filesystem management */
    meow_error_t (*mount)(const char* device, uint32_t flags);
    meow_error_t (*unmount)(void);
    
    /* File operations */
    meow_error_t (*open)(const char* path, int flags, void** private_data);
    meow_error_t (*close)(void* private_data);
    ssize_t (*read)(void* private_data, void* buffer, size_t count, off_t offset);
    ssize_t (*write)(void* private_data, const void* buffer, size_t count, off_t offset);
    off_t (*lseek)(void* private_data, off_t offset, int whence);
    
    /* Directory operations */
    meow_error_t (*mkdir)(const char* path, mode_t mode);
    meow_error_t (*rmdir)(const char* path);
    
    /* File management */
    meow_error_t (*unlink)(const char* path);
    meow_error_t (*stat)(const char* path, vfs_stat_t* stat_buf);
    
    /* Filesystem-specific data */
    void* private_data;
} vfs_filesystem_t;

/* ================================================================
 * VFS INTERNAL STRUCTURES
 * ================================================================ */

typedef struct vfs_file {
    int fd;
    vfs_filesystem_t* filesystem;
    void* private_data;
    off_t position;
    int flags;
} vfs_file_t;

typedef struct vfs_mount {
    char path[VFS_MAX_PATH];
    vfs_filesystem_t* filesystem;
    uint32_t flags;
} vfs_mount_t;

/* ================================================================
 * VFS CORE FUNCTIONS
 * ================================================================ */

/* VFS initialization */
meow_error_t vfs_init(void);

/* Filesystem registration */
meow_error_t vfs_register_filesystem(vfs_filesystem_t* fs);
vfs_filesystem_t* vfs_find_filesystem(const char* name);

/* Mount management */
meow_error_t vfs_mount(const char* device, const char* mountpoint, 
                       const char* fstype, uint32_t flags);
vfs_mount_t* vfs_find_mount(const char* path);

/* File operations */
int vfs_open(const char* path, int flags);
int vfs_close(int fd);
ssize_t vfs_read(int fd, void* buffer, size_t count);
ssize_t vfs_write(int fd, const void* buffer, size_t count);
off_t vfs_lseek(int fd, off_t offset, int whence);

/* Directory operations */
int vfs_mkdir(const char* path, mode_t mode);
int vfs_rmdir(const char* path);

/* File management */
int vfs_unlink(const char* path);
int vfs_stat(const char* path, vfs_stat_t* stat_buf);

/* Utility functions */
vfs_file_t* vfs_get_file(int fd);
void vfs_list_mounts(void);

#endif /* MEOW_VFS_H */