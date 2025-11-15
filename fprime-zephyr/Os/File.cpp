// ======================================================================
// \title fprime-zephyr/Os/File.cpp
// \brief Zephyr implementation for Os::File
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/File.hpp>
#include <fprime-zephyr/Os/File.hpp>
#include <type_traits>

namespace Os {
namespace Zephyr {
namespace File {


// Create constants for the max limits of the signed types
// These constants are used for comparisons with complementary unsigned types to avoid sign-compare warning
using UnsignedOffT = std::make_unsigned<off_t>::type;
static const UnsignedOffT OFF_T_MAX_LIMIT = static_cast<UnsignedOffT>(std::numeric_limits<off_t>::max());
using UnsignedSSizeT = std::make_unsigned<ssize_t>::type;
static const UnsignedSSizeT SSIZE_T_MAX_LIMIT = static_cast<UnsignedSSizeT>(std::numeric_limits<ssize_t>::max());

// Ensure size of FwSizeType is large enough to fit eh necessary range
static_assert(sizeof(FwSignedSizeType) >= sizeof(off_t),
              "FwSignedSizeType is not large enough to store values of type off_t");
static_assert(sizeof(FwSignedSizeType) >= sizeof(ssize_t),
              "FwSignedSizeType is not large enough to store values of type ssize_t");
static_assert(sizeof(FwSizeType) >= sizeof(size_t), "FwSizeType is not large enough to store values of type size_t");

// Now check ranges of FwSizeType
static_assert(std::numeric_limits<FwSignedSizeType>::max() >= std::numeric_limits<off_t>::max(),
              "Maximum value of FwSignedSizeType less than the maximum value of off_t. Configure a larger type.");
static_assert(std::numeric_limits<FwSizeType>::max() >= OFF_T_MAX_LIMIT,
              "Maximum value of FwSizeType less than the maximum value of off_t. Configure a larger type.");
static_assert(std::numeric_limits<FwSignedSizeType>::max() >= std::numeric_limits<ssize_t>::max(),
              "Maximum value of FwSignedSizeType less than the maximum value of ssize_t. Configure a larger type.");
static_assert(std::numeric_limits<FwSignedSizeType>::min() <= std::numeric_limits<off_t>::min(),
              "Minimum value of FwSignedSizeType larger than the minimum value of off_t. Configure a larger type.");
static_assert(std::numeric_limits<FwSignedSizeType>::min() <= std::numeric_limits<ssize_t>::min(),
              "Minimum value of FwSizeType larger than the minimum value of ssize_t. Configure a larger type.");
static_assert(std::numeric_limits<FwSizeType>::max() >= std::numeric_limits<size_t>::max(),
              "Maximum value of FwSizeType less than the maximum value of size_t. Configure a larger type.");

ZephyrFile::ZephyrFile() {
    fs_file_t_init(&this->m_handle.m_file);
}

ZephyrFile::ZephyrFile(const ZephyrFile& other) {
    fs_file_t_init(&this->m_handle.m_file);
    this->m_handle.m_file = other.m_handle.m_file;
}

ZephyrFile::Status ZephyrFile::open(const char* filepath,
                                  ZephyrFile::Mode requested_mode,
                                  ZephyrFile::OverwriteType overwrite) {
    int mode_flags = 0;
    Status status = OP_OK;
    switch (requested_mode) {
        case OPEN_READ:
            mode_flags = FS_O_READ;
            break;
        case OPEN_WRITE:
        case OPEN_SYNC_WRITE:
            mode_flags = FS_O_CREATE | FS_O_WRITE;
            break;
        case OPEN_CREATE:
            // TODO: handle overwrite
            mode_flags =
                FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC | ((overwrite == ZephyrFile::OverwriteType::OVERWRITE) ? 0 : 0);
            break;
        case OPEN_APPEND:
            mode_flags = FS_O_CREATE | FS_O_WRITE | FS_O_APPEND;
            break;
        default:
            FW_ASSERT(0, requested_mode);
            break;
    }
    int return_code = fs_open(&this->m_handle.m_file, filepath, mode_flags);
    if (return_code != 0) {
        //TODO: better status
        status = Os::File::Status::OTHER_ERROR;
    }
    return status;
}

void ZephyrFile::close() {
    (void) fs_close(&this->m_handle.m_file);
}

ZephyrFile::Status ZephyrFile::size(FwSizeType& size_result) {
    FwSizeType current_position = 0;
    Status status = this->position(current_position);
    size_result = 0;
    if (Os::File::Status::OP_OK == status) {
        off_t end_of_file;
        // Must be a coding error if current_position is larger than off_t max in Zephyr File
        FW_ASSERT(current_position <= OFF_T_MAX_LIMIT);
        // Seek to the end of the file to determine size
        int return_code = fs_seek(&this->m_handle.m_file, 0, FS_SEEK_END);
        if (return_code < 0) {
            //TODO:
            status = Os::File::Status::OTHER_ERROR;
        }
        end_of_file = fs_tell(&this->m_handle.m_file);
        // Return to original position
        (void)fs_seek(&this->m_handle.m_file, static_cast<off_t>(current_position), FS_SEEK_SET);
        size_result = static_cast<FwSizeType>(end_of_file);
    }
    return status;
}

ZephyrFile::Status ZephyrFile::position(FwSizeType& position_result) {
    Status status = OP_OK;
    position_result = 0;
    off_t actual = fs_tell(&this->m_handle.m_file);
    if (actual < 0) {
        // TODO: better errors
        status = Os::File::Status::OTHER_ERROR;
    }
    // Protected by static assertion (FwSizeType >= off_t)
    position_result = static_cast<FwSizeType>(actual);
    return status;
}

ZephyrFile::Status ZephyrFile::preallocate(FwSizeType offset, FwSizeType length) {
    ZephyrFile::Status status = Os::File::Status::NOT_SUPPORTED;
    // Check for larger size than Zephyr supports
    if ((length > OFF_T_MAX_LIMIT) || (offset > OFF_T_MAX_LIMIT) ||
        (std::numeric_limits<off_t>::max() - length) < offset) {
        status = Os::File::Status::BAD_SIZE;
    }
    // When the operation is not supported or Zephyr-API is not sufficient, fallback to a slower algorithm
    if (Os::File::Status::NOT_SUPPORTED == status) {
        // Calculate size
        FwSizeType file_size = 0;
        status = this->size(file_size);
        if (Os::File::Status::OP_OK == status) {
            // Calculate current position
            FwSizeType file_position = 0;
            status = this->position(file_position);
            // Check for overflow in seek calls
            if (file_position > static_cast<FwSizeType>(std::numeric_limits<FwSignedSizeType>::max()) ||
                file_size > static_cast<FwSizeType>(std::numeric_limits<FwSignedSizeType>::max())) {
                status = Os::File::Status::BAD_SIZE;
            }
            // Only allocate when the file is smaller than the allocation
            else if ((Os::File::Status::OP_OK == status) && (file_size < (offset + length))) {
                const FwSizeType write_length = (offset + length) - file_size;
                status = this->seek(static_cast<FwSignedSizeType>(file_size), ZephyrFile::SeekType::ABSOLUTE);
                if (Os::File::Status::OP_OK == status) {
                    // Fill in zeros past size of file to ensure compatibility with fallocate
                    for (FwSizeType i = 0; i < write_length; i++) {
                        FwSizeType write_size = 1;
                        status =
                            this->write(reinterpret_cast<const U8*>("\0"), write_size, ZephyrFile::WaitType::NO_WAIT);
                        if (Status::OP_OK != status || write_size != 1) {
                            break;
                        }
                    }
                    // Return to original position
                    if (Os::File::Status::OP_OK == status) {
                        status =
                            this->seek(static_cast<FwSignedSizeType>(file_position), ZephyrFile::SeekType::ABSOLUTE);
                    }
                }
            }
        }
    }
    return status;
}

ZephyrFile::Status ZephyrFile::seek(FwSignedSizeType offset, ZephyrFile::SeekType seekType) {
    Status status = OP_OK;
    if (offset > std::numeric_limits<off_t>::max()) {
        status = BAD_SIZE;
    } else {
        int return_code = fs_seek(&this->m_handle.m_file, static_cast<off_t>(offset),
                (seekType == SeekType::ABSOLUTE) ? FS_SEEK_SET : FS_SEEK_CUR);
        off_t actual = fs_tell(&this->m_handle.m_file);
        if (return_code < 0 ) {
            // TODO: error
            status = OTHER_ERROR;
        } else if ((seekType == SeekType::ABSOLUTE) && (actual != offset)) {
            status = Os::File::Status::OTHER_ERROR;
        }
    }
    return status;
}

ZephyrFile::Status ZephyrFile::flush() {
    ZephyrFile::Status status = OP_OK;
    int return_code = fs_sync(&this->m_handle.m_file);
    if (return_code != 0) {
        // TODO: errors
        status = Os::File::Status::OTHER_ERROR;
    }
    return status;
}

ZephyrFile::Status ZephyrFile::read(U8* buffer, FwSizeType& size, ZephyrFile::WaitType wait) {
    Status status = OP_OK;
    FwSizeType accumulated = 0;
    // Loop up to 2 times for each by, bounded to prevent overflow
    const FwSizeType maximum =
        (size > (std::numeric_limits<FwSizeType>::max() / 2)) ? std::numeric_limits<FwSizeType>::max() : size * 2;
    // POSIX APIs are implementation dependent when dealing with sizes larger than the signed return value
    // thus we ensure a clear decision: BAD_SIZE
    if (size > SSIZE_T_MAX_LIMIT) {
        return BAD_SIZE;
    }

    for (FwSizeType i = 0; i < maximum && accumulated < size; i++) {
        // char* for some Zephyr implementations
        ssize_t read_size = fs_read(&this->m_handle.m_file, &buffer[accumulated], static_cast<size_t>(size - accumulated));
        // Non-interrupt error
        if (read_size < 0) {
            //TODO: status
            status = Os::File::Status::OTHER_ERROR;
            break;
        }
        // End-of-file
        else if (read_size == 0) {
            break;
        }
        accumulated += static_cast<FwSizeType>(read_size);
        // Stop looping when we had a good read and are not waiting
        if (not wait) {
            break;
        }
    }
    size = accumulated;
    return status;
}

ZephyrFile::Status ZephyrFile::write(const U8* buffer, FwSizeType& size, ZephyrFile::WaitType wait) {
    Status status = OP_OK;
    FwSizeType accumulated = 0;
    // Loop up to 2 times for each by, bounded to prevent overflow
    const FwSizeType maximum =
        (size > (std::numeric_limits<FwSizeType>::max() / 2)) ? std::numeric_limits<FwSizeType>::max() : size * 2;
    // POSIX APIs are implementation dependent when dealing with sizes larger than the signed return value
    // thus we ensure a clear decision: BAD_SIZE
    if (size > SSIZE_T_MAX_LIMIT) {
        return BAD_SIZE;
    }

    for (FwSizeType i = 0; i < maximum && accumulated < size; i++) {
        // char* for some Zephyr implementations
        ssize_t write_size = fs_write(&this->m_handle.m_file, reinterpret_cast<const CHAR*>(&buffer[accumulated]),
                    static_cast<size_t>(size - accumulated));
        // Non-interrupt error
        if (write_size < 0) {
            //TODO: error
            status = Os::File::Status::OTHER_ERROR;
            break;
        }
        accumulated += static_cast<FwSizeType>(write_size);
    }
    size = accumulated;
    // When waiting, sync to disk
    if (wait) {
       this->flush();
    }
    return status;
}

FileHandle* ZephyrFile::getHandle() {
    return &this->m_handle;
}

}  // namespace File
}  // namespace Zephyr
}  // namespace Os
