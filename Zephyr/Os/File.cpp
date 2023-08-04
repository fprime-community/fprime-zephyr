#include <cstdlib>
#include <Os/File.hpp>
#include <Fw/Types/Assert.hpp>

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>

#define DEBUG_PRINT(x,...) printk(x,##__VA_ARGS__);
// #define DEBUG_PRINT(x,...)

namespace Os {

    File::File() :m_fd(-1),m_mode(OPEN_NO_MODE),m_lastError(0) {}

    File::~File() {
        this->close();
    }

    File::Status File::open(const char* fileName, Mode mode) {
        return this->open(fileName,mode,false);
    }

    File::Status File::open(const char* fileName, File::Mode mode, bool include_excl) {
        struct fs_file_t *file = reinterpret_cast<struct fs_file_t *>(k_malloc(sizeof(struct fs_file_t)));
        if (file == nullptr) {
            FW_ASSERT(0);
            return NO_SPACE;
        }

        fs_mode_t flags = 0;
        switch (mode) {
            case OPEN_READ:
                flags = FS_O_READ;
                break;
            case OPEN_SYNC_WRITE:
            case OPEN_WRITE:
                flags = FS_O_WRITE | FS_O_CREATE;
                break;
            case OPEN_APPEND:
                flags = FS_O_WRITE | FS_O_APPEND | FS_O_CREATE;
                break;
            case OPEN_SYNC_DIRECT_WRITE:
                DEBUG_PRINT("Not supported mode %d\n", mode);
                FW_ASSERT(0, mode);
                break;
            default:
                FW_ASSERT(0, mode);
                break;
        }

        fs_file_t_init(file);
        int rc;
        
        rc = fs_open(file, fileName, flags);
        if (rc < 0) {
            DEBUG_PRINT("Error opening file %s: %d\n", fileName, rc);
            this->m_lastError = rc;
            free(file);
            return NOT_OPENED;
        }

        // Wild, wild west. Template this out later.
        this->m_fd = reinterpret_cast<NATIVE_INT_TYPE>(file);
        this->m_mode = mode;
        return OP_OK;
    }

    void File::close() {
        if (this->m_fd == -1) return;

        struct fs_file_t *file = reinterpret_cast<struct fs_file_t *>(this->m_fd);
        int ret = fs_close(file);
        if (ret < 0) {
            DEBUG_PRINT("Error closing file: %d\n", ret);
        }
        FW_ASSERT(ret == 0, ret);
        
        k_free(file);
        this->m_fd = -1;
        return;
    }

    File::Status File::seek(NATIVE_INT_TYPE offset, bool absolute) {

        // make sure file has been opened
        if (this->m_mode == OPEN_NO_MODE) {
            return NOT_OPENED;
        }
        FW_ASSERT(this->m_fd != -1);

        struct fs_file_t *file = reinterpret_cast<struct fs_file_t *>(this->m_fd);
        NATIVE_INT_TYPE whence = absolute ? FS_SEEK_SET : FS_SEEK_CUR;
        int ret = fs_seek(file, offset, whence);
        if (ret < 0) {
            DEBUG_PRINT("Error seeking file: %d\n", ret);
            return OTHER_ERROR;
        }

        return OP_OK;
    }

    File::Status File::write(const void * buffer, NATIVE_INT_TYPE &size, bool waitForDone) {
     
        // make sure file has been opened
        if (OPEN_NO_MODE == this->m_mode) {
            size = 0;
            return NOT_OPENED;
        }
        FW_ASSERT(this->m_fd != -1);
        FW_ASSERT(size >= 0, size);

        struct fs_file_t *file = reinterpret_cast<struct fs_file_t *>(this->m_fd);
        int ret = fs_write(file, buffer, size);
        if (ret < 0) {
            DEBUG_PRINT("Error writing file: %d\n", ret);
            return OTHER_ERROR;
        }

        if (this->m_mode == OPEN_SYNC_WRITE || waitForDone) {
            ret = fs_sync(file);
            if (ret < 0) {
                DEBUG_PRINT("Error syncing file: %d\n", ret);
                return OTHER_ERROR;
            }
        }

        return OP_OK;
    }

    File::Status File::read(void * buffer, NATIVE_INT_TYPE &size, bool waitForFull) {
        
        FW_ASSERT(buffer);
        FW_ASSERT(size >= 0, size);
        FW_ASSERT(this->m_fd != -1);

        // make sure it has been opened
        if (OPEN_NO_MODE == this->m_mode) {
            size = 0;
            return NOT_OPENED;
        }

        struct fs_file_t *file = reinterpret_cast<struct fs_file_t *>(this->m_fd);
        NATIVE_INT_TYPE accSize = 0; // accumulated size
        Status stat = OP_OK;
        NATIVE_INT_TYPE maxIters = size*2; // loop limit; couldn't block more times than number of bytes

        while (maxIters > 0) {

            ssize_t rc = fs_read(file, buffer, size-accSize);

            // could be an error
            if (rc < 0) {
                stat = OTHER_ERROR;
                this->m_lastError = rc;
                accSize = 0;
                break; // break out of while loop
            } else if (0 == rc || rc == size - accSize || not waitForFull) { // a good read
                accSize += rc;
                break;
            } else { // good read and waitForFull
                accSize += rc;
                // move the buffer pointer
                U8* charPtr = static_cast<U8*>(buffer);
                charPtr = &charPtr[rc];
                buffer = static_cast<void*>(charPtr);
            }
            
            maxIters--; // decrement loop count
        } // end read while loop

        // make sure we didn't exceed loop count
        FW_ASSERT(maxIters >= 0);

        size = accSize;

        return stat;
    }
    
    NATIVE_INT_TYPE File::getLastError() {
        return this->m_lastError;
    }

}