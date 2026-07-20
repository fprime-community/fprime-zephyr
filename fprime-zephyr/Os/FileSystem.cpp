// ======================================================================
// \title fprime-zephyr/Os/FileSystem.cpp
// \brief Posix implementation for Os::FileSystem
// ======================================================================
#include "fprime-zephyr/Os/FileSystem.hpp"
#include "Os/Posix/error.hpp" // Zephyr uses Posix error codes (but negated)


#include <zephyr/fs/fs.h>

namespace Os {
namespace Zephyr {
namespace FileSystem {


ZephyrFileSystem::Status remove_helper(const char* path) {
    ZephyrFileSystem::Status status = ZephyrFileSystem::Status::OP_OK;
    int errno_status = fs_unlink(path);
    if (errno_status != 0) {
        status = Os::Posix::errno_to_filesystem_status(-1 * errno_status);
    }
    return status;
}

ZephyrFileSystem::Status ZephyrFileSystem::_removeDirectory(const char* path) {
    PathType pathType;
    ZephyrFileSystem::Status status = ZephyrFileSystem::_getPathType(path, pathType);
    if (status == OP_OK && pathType != PathType::DIRECTORY) {
        status = Status::NOT_DIR;
    } else if (status == OP_OK) {
        status = remove_helper(path);
    }
    return status;
}

ZephyrFileSystem::Status ZephyrFileSystem::_removeFile(const char* path) {
    return remove_helper(path);
}

ZephyrFileSystem::Status ZephyrFileSystem::_rename(const char* originPath, const char* destPath) {
    Status status = OP_OK;
    int errno_status = fs_rename(originPath, destPath);
    if (errno_status != 0) {
        status = Os::Posix::errno_to_filesystem_status(-1 * errno_status);
    }
    return status;
}

ZephyrFileSystem::Status ZephyrFileSystem::_getWorkingDirectory(char* path, FwSizeType bufferSize) {
    // Working directories not supported in Zephyr
    Status status = NOT_SUPPORTED;
    return status;
}

ZephyrFileSystem::Status ZephyrFileSystem::_changeWorkingDirectory(const char* path) {
    Status status = NOT_SUPPORTED;
    return status;
}

ZephyrFileSystem::Status ZephyrFileSystem::_getFreeSpace(const char* path,
                                                       FwSizeType& totalBytes,
                                                       FwSizeType& freeBytes) {

    Status stat = OP_OK;
    static_assert(std::numeric_limits<FwSizeType>::max() >= std::numeric_limits<fsblkcnt_t>::max(),
                  "FwSizeType must be able to hold fsblkcnt_t");
    static_assert(std::numeric_limits<FwSizeType>::min() <= std::numeric_limits<fsblkcnt_t>::min(),
                  "FwSizeType must be able to hold fsblkcnt_t");
    static_assert(std::numeric_limits<FwSizeType>::max() >= std::numeric_limits<unsigned long>::max(),
                  "FwSizeType must be able to hold unsigned long");


    struct fs_statvfs fsStat;
    int errno_status = fs_statvfs(path, &fsStat);
    if (errno_status != 0) {
        return Os::Posix::errno_to_filesystem_status(-1 * errno_status);
    }

    const FwSizeType block_size = static_cast<FwSizeType>(fsStat.f_frsize);
    const FwSizeType free_blocks = static_cast<FwSizeType>(fsStat.f_bfree);
    const FwSizeType total_blocks = static_cast<FwSizeType>(fsStat.f_blocks);

    // Check for overflow in multiplication
    if (free_blocks > (std::numeric_limits<FwSizeType>::max() / block_size) ||
        total_blocks > (std::numeric_limits<FwSizeType>::max() / block_size)) {
        return OVERFLOW_ERROR;
    }
    freeBytes = free_blocks * block_size;
    totalBytes = total_blocks * block_size;
    return stat;
}

FileSystemHandle* ZephyrFileSystem::getHandle() {
    return &this->m_handle;
}

ZephyrFileSystem::Status ZephyrFileSystem::_getPathType(const char* path, PathType& pathType) {
    FW_ASSERT(path != nullptr);
    struct fs_dirent entry;
    Status status = OP_OK;
    int errno_status = fs_stat(path, &entry);
    if (errno_status != 0) {
        status = Os::Posix::errno_to_filesystem_status(-1 * errno_status);
    } else if (entry.type == FS_DIR_ENTRY_DIR) {
        pathType = PathType::DIRECTORY;
    } else if (entry.type == FS_DIR_ENTRY_FILE) {
        pathType = PathType::FILE;
    } else {
        pathType = PathType::OTHER;
    }
    return status;
}

}  // namespace FileSystem
}  // namespace Zephyr
}  // namespace Os
