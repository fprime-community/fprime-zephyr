// ======================================================================
// \title fprime-zephyr/Os/Directory.cpp
// \brief Zephyr implementation for Os::Directory
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/StringUtils.hpp>
#include <fprime-zephyr/Os/Directory.hpp>
#include <Os/Posix/error.hpp>
#include <zephyr/fs/fs.h>

namespace Os {
namespace Zephyr {
namespace Directory {

ZephyrDirectory::ZephyrDirectory() : Os::DirectoryInterface(), m_handle() {}

DirectoryHandle* ZephyrDirectory::getHandle() {
    return &this->m_handle;
}

ZephyrDirectory::Status ZephyrDirectory::open(const char* path, OpenMode mode) {
    int errno_status = 0;
    Status status = Status::OP_OK;

    // If one of the CREATE mode, attempt to create the directory
    if (mode == OpenMode::CREATE_EXCLUSIVE || mode == OpenMode::CREATE_IF_MISSING) {
        if ((errno_status = ::fs_mkdir(path)) != 0) {
            status = Os::Posix::errno_to_directory_status(-1 * errno_status);
            // If error is not ALREADY_EXISTS, return the error
            // If any error and mode CREATE_EXCLUSIVE, return the error
            // Else, we keep going with OP_OK
            if (status != Status::ALREADY_EXISTS || mode == OpenMode::CREATE_EXCLUSIVE) {
                return status;
            } else {
                status = Status::OP_OK;
            }
        }
    }
    // Now attempt to open the directory
    errno_status = ::fs_opendir(&this->m_handle.m_directory_descriptor, path);
    if (errno_status != 0) {
        status = Os::Posix::errno_to_directory_status(-1 * errno_status);
    }
    return status;
}

ZephyrDirectory::Status ZephyrDirectory::rewind() {
    return ZephyrDirectory::Status::NOT_SUPPORTED;
}

ZephyrDirectory::Status ZephyrDirectory::read(char* fileNameBuffer, FwSizeType bufSize) {
    FW_ASSERT(fileNameBuffer);
    int errno_status = 0;
    Status status = Status::OP_OK;


    struct fs_dirent direntData;
    errno_status = fs_readdir(&this->m_handle.m_directory_descriptor, &direntData);
    if (errno_status != 0) {
        status = Os::Posix::errno_to_directory_status(-1 * errno_status);
    } else if (direntData.name[0] == 0) {
        status = Status::NO_MORE_FILES;
    } else {
        (void)Fw::StringUtils::string_copy(fileNameBuffer, direntData.name, bufSize);
    }
    return status;
}

void ZephyrDirectory::close() {
    (void) fs_closedir(&this->m_handle.m_directory_descriptor);
}

}  // namespace Directory
}  // namespace Zephyr
}  // namespace Os
