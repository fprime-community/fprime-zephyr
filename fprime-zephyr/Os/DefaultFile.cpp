// ======================================================================
// \title fprime-zephyr/Os/DefaultFile.cpp
// \brief sets default Os::File to Zephyr implementation via linker
// ======================================================================
#include "Os/Delegate.hpp"
#include "Os/Directory.hpp"
#include "Os/File.hpp"
#include "Os/FileSystem.hpp"
#include "fprime-zephyr/Os/Directory.hpp"
#include "fprime-zephyr/Os/File.hpp"
#include "fprime-zephyr/Os/FileSystem.hpp"

namespace Os {
FileInterface* FileInterface::getDelegate(FileHandleStorage& aligned_new_memory, const FileInterface* to_copy) {
    return Os::Delegate::makeDelegate<FileInterface, Os::Zephyr::File::ZephyrFile>(aligned_new_memory, to_copy);
}
FileSystemInterface* FileSystemInterface::getDelegate(FileSystemHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<FileSystemInterface, Os::Zephyr::FileSystem::ZephyrFileSystem>(aligned_new_memory);
}
DirectoryInterface* DirectoryInterface::getDelegate(DirectoryHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<DirectoryInterface, Os::Zephyr::Directory::ZephyrDirectory>(aligned_new_memory);
}
}  // namespace Os
