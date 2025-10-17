// ======================================================================
// \title Os/Directory.cpp
// \brief Zephyr implementation for Os::Directory
// ======================================================================
#include <sys/stat.h>
#include <cerrno>
#include <cstring>

#include <Fw/Types/Assert.hpp>
#include <Fw/Types/StringUtils.hpp>
#include "Directory.hpp"

namespace Os {
namespace Zephyr {
namespace Directory {

// Helper function to convert errno to Directory Status
ZephyrDirectory::Status errno_to_directory_status(int errno_value) {
    switch (errno_value) {
        case 0:
            return ZephyrDirectory::Status::OP_OK;
        case ENOENT:
            return ZephyrDirectory::Status::DOESNT_EXIST;
        case EEXIST:
            return ZephyrDirectory::Status::ALREADY_EXISTS;
        case EACCES:
            return ZephyrDirectory::Status::NO_PERMISSION;
        case EIO:
            return ZephyrDirectory::Status::OTHER_ERROR;
        case EBADF:
            return ZephyrDirectory::Status::BAD_DESCRIPTOR;
        default:
            return ZephyrDirectory::Status::OTHER_ERROR;
    }
}

ZephyrDirectory::ZephyrDirectory() : Os::DirectoryInterface(), m_handle() {}

DirectoryHandle* ZephyrDirectory::getHandle() {
    return &this->m_handle;
}

ZephyrDirectory::Status ZephyrDirectory::open(const char* path, OpenMode mode) {
    ZephyrDirectory::Status status = ZephyrDirectory::Status::OP_OK;

    return status;
}

bool ZephyrDirectory::isOpen() {
    return this->m_handle.m_dir_descriptor != nullptr;
}

ZephyrDirectory::Status ZephyrDirectory::rewind() {
    ZephyrDirectory::Status status = ZephyrDirectory::Status::OP_OK;
    return status;
}

ZephyrDirectory::Status ZephyrDirectory::read(char* fileNameBuffer, FwSizeType bufSize) {
    ZephyrDirectory::Status status = ZephyrDirectory::Status::OP_OK;
    return status;
}

void ZephyrDirectory::close() {
    if (this->m_handle.m_dir_descriptor != nullptr) {
        //close it
    }
    this->m_handle.m_dir_descriptor = nullptr;
}

}  // namespace Directory
}  // namespace Zephyr
}  // namespace Os
