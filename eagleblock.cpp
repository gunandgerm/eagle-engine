/*
 * Copyright (c) 2017 LIHAIBING. All Rights Reserved
 *
 * @file eagleblock.cpp
 * @author lihaibing(593255200@qq.com)
 * @date 2017/07/22 16:11:01
 * @brief
 *
*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <assert.h>
#include <map>
#include "eagleengine/blockcompact.h"
#include "eagleengine/crc32c.h"

namespace eagleengine {

EagleBlock::EagleBlock() {
    log_ = NULL;
    indexs_ = NULL;
    internal_buf_ = NULL;

    data_fd_ = -1;
    index_fd_ = -1;

    max_sequence_number_ = -1;
    synced_sequence_number_ = -1;
    data_offset_ = 0;
    index_offset_ = 0;

    num_objects_ = 0;

    status_ = kNormal;
}

EagleBlock::~EagleBlock() {
    delete log_;
    delete indexs_;
    free(internal_buf_);

    if (data_fd_ >= 0) {
        close(data_fd_);
    }
    if (index_fd_ >= 0) {
        close(index_fd_);
    }
}

Status EagleBlock::OpenBlock(const std::string& folder, EagleBlock** result) {
    EagleBlock* tmp_block = new EagleBlock();
    Status status = tmp_block->Open(folder);
    if (status.code() == kOk) {
        *result = tmp_block;
    } else {
        delete tmp_block;
    }
    return status;
}

Status EagleBlock::CreateBlock(const std::string& folder, EagleBlock** result, int64_t max_block_size) {
    Status status;
    if (max_block_size > kMaxBlockSize) {
        status.set_code(kInvalidArg);
        status.set_msg("max_block_size exceeds %ld", kMaxBlockSize);
        return status;
    }

    EagleBlock* tmp_block = new EagleBlock();
    status = tmp_block->Create(folder, max_block_size);
    if (status.code() == kOk) {
        // return the new block
        *result = tmp_block;
    } else {
        delete tmp_block;
    }

    return status;
}

Status EagleBlock::PutObject(const std::string& content, int64_t* object_id) {
    Status status;
    if (content.length() <= 0) {
        status.set_code(kInvalidArg);
        status.set_msg("content is empty");
        return status;
    }
    if (content.length() > kMaxObjectSize) {
        status.set_code(kInvalidArg);
        status.set_msg("object size exceeds %d", kMaxObjectSize);
        return status;
    }

    if (!IsNormal()) {
        status.set_code(kInternalError);
        status.set_msg("block status %d, it is not normal", status_);
        return status;
    }

    ObjectHeader header;
    const int header_size = sizeof(header);
    int64_t tmp_size = data_offset_;
    tmp_size += header_size;
    tmp_size += content.length();
    if (tmp_size > max_block_size_) {
        status.set_code(kInternalError);
        status.set_msg("current block size is %ld, max object size is %ld, no free space "
                       "to hold this object", data_offset_, max_block_size_);
        return status;
    }

    int64_t current_max_seq = max_sequence_number_ + 1;
    // 1. write data file
    // 1). write object header
    header.object_id = current_max_seq;
    header.size = content.size();
    header.crc = Adler32_Value(content.data(), content.size());
    errno = 0;
    int64_t start_offset = data_offset_;
    int written_size = pwrite(data_fd_, &header, header_size, start_offset);
    if (written_size != header_size) {
        status.set_code(kIOError);
        status.set_msg("failed to write object header,only write %d bytes but expect %d bytes, %m",
                       written_size, header_size);
        return status;
    }
    // 2). write data
    errno = 0;
    start_offset += header_size;
    written_size = pwrite(data_fd_, content.data(), content.length(), start_offset);
    if (written_size != (int)content.length()) {
        status.set_code(kIOError);
        status.set_msg("failed to write object content, only write %d bytes but expect %d bytes, %m",
                       written_size, content.length());
        return status;
    }

    // 2. write index file
    IndexEntry entry;
    entry.sequence_number = current_max_seq;
    entry.object_id = header.object_id;
    entry.offset = start_offset;
    entry.size = content.length();
    const int entry_size = sizeof(entry);
    errno = 0;
    written_size = pwrite(index_fd_, &entry, entry_size, index_offset_);
    if (written_size != entry_size) {
        status.set_code(kIOError);
        status.set_msg("failed to write index, only written %d bytes but expect %d bytes, %m",
                       written_size, entry_size);
        return status;
    }

    // 3. update index
    IndexEntry old_entry;
    if (!indexs_->Insert(entry, &old_entry)) {
        status.set_code(kInternalError);
        status.set_msg("duplicated object id %ld", entry.object_id);
        log_->Write(LL_FATAL, "duplicated object id %ld, exit!", entry.object_id);
        exit(1);
        return status;
    }

    start_offset += content.length();
    data_offset_ = start_offset;
    index_offset_ += entry_size;
    max_sequence_number_ = current_max_seq;
    *object_id = max_sequence_number_;

    num_objects_++;

    return status;
}

Status EagleBlock::GetObject(int64_t object_id, std::string* result) {
    Status status;
    IndexEntry entry;
    if (!indexs_->Get(object_id, &entry)) {
        status.set_code(kObjectNotFound);
        status.set_msg("object %ld doesn't exist", object_id);
        return status;
    }

    int read_size = pread(data_fd_, internal_buf_, entry.size, entry.offset);
    if (read_size != entry.size) {
        status.set_code(kIOError);
        status.set_msg("only read %d bytes but expect %d bytes for object %ld", read_size,
                       entry.size, object_id);
        return status;
    }
    result->assign(internal_buf_, entry.size);

    return status;
}

Status EagleBlock::DeleteObject(int64_t object_id) {
    Status status;
    if (!IsNormal()) {
        status.set_code(kInternalError);
        status.set_msg("block status %d, it is not normal", status_);
        return status;
    }

    IndexEntry entry;
    if (!indexs_->Get(object_id, &entry)) {
        // not exist; return ok
        status.set_msg("object %ld doesn't exist", object_id);
        return status;
    }

    int64_t current_max_seq = max_sequence_number_ + 1;
    IndexEntry delete_entry;
    delete_entry.sequence_number = current_max_seq;
    delete_entry.object_id = object_id;
    delete_entry.offset = entry.offset;
    delete_entry.size = 0;
    errno = 0;
    const int entry_size = sizeof(delete_entry);
    int written_size = pwrite(index_fd_, &delete_entry, entry_size, index_offset_);
    if (written_size != entry_size) {
        status.set_code(kIOError);
        status.set_msg("failed to write index, only written %d bytes but expect %d bytes",
                       written_size, entry_size);
        return status;
    }

    indexs_->Delete(object_id);
    max_sequence_number_ = current_max_seq;
    index_offset_ += entry_size;
    return status;
}

Status EagleBlock::GetCurrentSubdir(std::string* result) {
    Status status;
    std::string current_file = root_dir_;
    current_file.append("/");
    current_file.append(kCurrentFile);

    errno = 0;
    int current_fd = open(current_file.c_str(), O_RDWR);
    if (current_fd < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to open current file %s, %m", current_file.c_str());
        return status;
    }

    char tmp_buf[32] = {0};
    int size = (int)read(current_fd, tmp_buf, 32);
    if (size <= 0) {
        status.set_code(kIOError);
        status.set_msg("failed to read current file,%m");
    } else {
        result->assign(tmp_buf, size);
    }

    close(current_fd);
    return status;
}

Status EagleBlock::StoreCurrentSubdir(const std::string& subdir) {
    Status status;
    std::string current_file = root_dir_;
    current_file.append("/");
    current_file.append(kCurrentFile);

    std::string tmp_current_file = current_file;
    tmp_current_file.append("_tmp");
    errno = 0;
    int tmp_fd = open(tmp_current_file.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (tmp_fd < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to create file %s, %m", tmp_current_file.c_str());
        return status;
    }

    // write to tmp current file
    //
    errno = 0;
    const int current_size = (int)subdir.size();
    int size = (int)write(tmp_fd, subdir.data(), current_size);
    if (size != current_size) {
        status.set_code(kIOError);
        status.set_msg("failed to write file %s, only written %d bytes but expect %d bytes,"
                       "%m", tmp_current_file.c_str(), size, current_size);
    } else if (0 != fsync(tmp_fd)) {
        status.set_code(kIOError);
        status.set_msg("failed to fsync file %s, %m", tmp_current_file.c_str());
    }

    // rename tmp current file to formal current file
    errno = 0;
    if (status.code() == kOk && rename(tmp_current_file.c_str(), current_file.c_str()) != 0) {
        status.set_code(kIOError);
        status.set_msg("failed to rename file %s to %s, %m", tmp_current_file.c_str(),
                       current_file.c_str());
    }
    close(tmp_fd);

    return status;
}

Status EagleBlock::StoreManifestEx(const Manifest& manifest, const std::string& manifest_file) {
    Status status;
    std::string tmp_manifest_file = manifest_file;
    tmp_manifest_file.append("_tmp");

    errno = 0;
    int tmp_fd = open(tmp_manifest_file.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (tmp_fd < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to create file %s, %m", tmp_manifest_file.c_str());
        return status;
    }

    // write to tmp manifest file
    //
    errno = 0;
    const int manifest_size = sizeof(manifest);
    int size = (int)write(tmp_fd, &manifest, manifest_size);
    if (size != manifest_size) {
        status.set_code(kIOError);
        status.set_msg("failed to write file %s, only written %d bytes but expect %d bytes,"
                       "%m", tmp_manifest_file.c_str(), size, manifest_size);
    } else if (fsync(tmp_fd) != 0) {
        status.set_code(kIOError);
        status.set_msg("failed to fsync file %s, %m", tmp_manifest_file.c_str());
    }

    // rename tmp manifest file to formal manifest file
    errno = 0;
    if (status.code() == kOk && rename(tmp_manifest_file.c_str(), manifest_file.c_str()) != 0) {
        status.set_code(kIOError);
        status.set_msg("failed to rename file %s to %s, %m", tmp_manifest_file.c_str(),
                       manifest_file.c_str());
    }

    close(tmp_fd);
    return status;
}

Status EagleBlock::StoreManifest() {
    Status status;
    Manifest manifest;
    manifest.max_block_size = max_block_size_;
    manifest.synced_sequence_number = synced_sequence_number_;

    std::string manifest_file = GetFilePath(current_subdir_, kManifestFile);
    return StoreManifestEx(manifest, manifest_file);
}

std::string EagleBlock::GetBlockRootDir(const std::string& subdir) {
    std::string full_path = root_dir_;
    full_path.append("/");
    full_path.append(subdir);
    return full_path;
}

std::string EagleBlock::GetFilePath(const std::string& subdir, const std::string& file_name) {
    std::string full_path = GetBlockRootDir(subdir);
    full_path.append("/");
    full_path.append(file_name);
    return full_path;
}

Status EagleBlock::GetManifest(Manifest* manifest) {
    Status status;
    std::string manifest_file = GetFilePath(current_subdir_, kManifestFile);
    errno = 0;
    int manifest_fd = open(manifest_file.c_str(), O_RDWR);
    if (manifest_fd < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to open manifest file, %m");
        return status;
    }

    int expect_size = sizeof(*manifest);
    int size = (int)read(manifest_fd, (char*)manifest, expect_size);
    if (size != expect_size) {
        status.set_code(kIOError);
        status.set_msg("failed to read manifest file,only read %d bytes but expect %d bytes,"
                       "%m", size, expect_size);
    } else if (manifest->magic_number != kMagicNumber) {
        // check magic
        status.set_code(kIOError);
        status.set_msg("manifest is corrupted, magic number read is not equal with expected value");
    }

    close(manifest_fd);
    return status;
}

Status EagleBlock::Init(const std::string& folder, int64_t max_block_size, bool exist) {
    Status status;
    int folder_len = folder.length();
    if (folder_len < 1) {
        status.set_code(kInvalidArg);
        status.set_msg("root dir cannot be empty");
        return status;
    }
    if (folder.at(folder_len - 1) == '/') {
        root_dir_ = folder.substr(0, folder_len - 1);
    } else {
        root_dir_ = folder;
    }
    internal_buf_ = (char*)malloc(kMaxObjectSize);

    if (!exist) {
        // default sub dir is 0
        current_subdir_ = kDefaultSubdir;
        // mkdir sub dir 0
        errno = 0;
        std::string block_root_dir = GetBlockRootDir(current_subdir_);
        if (0 != mkdir(block_root_dir.c_str(), 0744)) {
            status.set_code(kIOError);
            status.set_msg("failed to create dir %s, %m", block_root_dir.c_str());
            return status;
        }
    } else {
        status = GetCurrentSubdir(&current_subdir_);
        if (status.code() != kOk) {
            return status;
        }
    }

    // 1. create log file
    log_ = new Log(GetBlockRootDir(current_subdir_));
    if (!log_->Init()) {
        status.set_code(kInternalError);
        status.set_msg("failed to create internal log file");
        return status;
    }

    // 2. construct index fd
    std::string index_file = GetFilePath(current_subdir_, kIndexFile);
    errno = 0;
    if (exist) {
        index_fd_ = open(index_file.c_str(), O_RDWR | O_APPEND);
    } else {
        index_fd_ = open(index_file.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
    }
    if (index_fd_ < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to open index file %s, %m", index_file.c_str());
        return status;
    }

    // 3. construct data fd
    std::string data_file = GetFilePath(current_subdir_, kDataFile);
    errno = 0;
    if (exist) {
        data_fd_ = open(data_file.c_str(), O_RDWR | O_APPEND);
    } else {
        data_fd_ = open(data_file.c_str(), O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE, 0644);
    }
    if (data_fd_ < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to open data file %s, %m", data_file.c_str());
        return status;
    }

    // 4. init max block size
    if (!exist) {
        max_block_size_ = max_block_size;
    } else {
        Manifest manifest;
        status = GetManifest(&manifest);
        if (status.code() != kOk) {
            return status;
        }
        max_block_size_ = manifest.max_block_size;
        synced_sequence_number_ = manifest.synced_sequence_number;
    }

    // 5. init mem indexs
    int slot_num = (int)(max_block_size_ / kAveObjectSize);
    indexs_ = new HashTable<IndexEntry>(slot_num);

    return status;
}

Status EagleBlock::ValidateObject(const IndexEntry& entry) {
    Status status;
    ObjectHeader header;
    const int header_size = sizeof(header);
    int64_t start_offset = entry.offset - header_size;
    if (start_offset < 0) {
        status.set_code(kDataCorrupted);
        status.set_msg("data offset %ld less than header size %d, sequence_number %ld",
                       start_offset, header_size, entry.sequence_number);
        return status;
    }

    // read object header
    int read_size = pread(data_fd_, &header, header_size, start_offset);
    if (read_size != header_size) {
        status.set_code(kDataCorrupted);
        status.set_msg("only read %d bytes for object header, expect %d bytes, data "
                "offset %ld, sequence_number %ld", read_size, header_size, start_offset,
                entry.sequence_number);
        return status;
    }

    // validate header
    if (header.object_id != entry.object_id) {
        status.set_code(kDataCorrupted);
        status.set_msg("object id in header %ld but in index is %ld, sequence_number %ld",
                       header.object_id, entry.object_id, entry.sequence_number);
        return status;
    }

    if (entry.size > 0) {
        if (header.object_id != entry.sequence_number) {
            status.set_code(kDataCorrupted);
            status.set_msg("object id in header %ld but sequence_number in index is %ld",
                    header.object_id, entry.sequence_number);
            return status;
        }
    }

    int size = entry.size;
    if (size <= 0) {
        // this object is marked as deleted
        size = header.size;
    }

    // read object data
    start_offset += header_size;
    read_size = pread(data_fd_, internal_buf_, size, start_offset);
    if (read_size != size) {
        status.set_code(kDataCorrupted);
        status.set_msg("only read %d bytes for object , expect %d bytes, data "
                "offset %ld, sequence_number %ld", read_size, size, start_offset,
                entry.sequence_number);
        return status;
    }

    // check crc
    uint32_t crc = Adler32_Value(internal_buf_, size);
    if (crc != header.crc) {
        status.set_code(kDataCorrupted);
        status.set_msg("crc check error!crc calculated is %d but stored in header is %d, data "
                "offset %ld, sequence_number %ld", crc, header.crc, entry.offset, entry.sequence_number);
    }

    return status;
}

Status EagleBlock::Open(const std::string& folder) {
    Status status = Init(folder, 0, true);
    if (status.code() != kOk) {
        return status;
    }
    log_->Write(LL_NOTICE, "begin to open block");

    // 1. load index file to consturct memory indexs
    struct stat index_buf;
    errno = 0;
    if (0 != fstat(index_fd_, &index_buf)) {
        status.set_code(kIOError);
        status.set_msg("failed to get index file info, %m");
        return status;
    }

    IndexEntry entry;
    const int entry_size = sizeof(entry);
    int64_t end_offset = index_buf.st_size - (index_buf.st_size % entry_size);
    while (index_offset_ < end_offset) {
        errno = 0;
        int read_size = read(index_fd_, &entry, entry_size);
        if (read_size != entry_size) {
            status.set_code(kIOError);
            status.set_msg("failed to read index file, only read %d bytes but expect %d bytes"
                           ", offset %ld, %m", read_size, entry_size, index_offset_);
            return status;
        }

        if (entry.sequence_number > synced_sequence_number_) {
            // for unsynced objects; need to validate
            status = ValidateObject(entry);
            if (status.code() != kOk) {
                log_->Write(LL_ERROR, status.ToString().c_str());
                break;
            }
        }

        // for synced objects, no need to validate 
        // update mem indexes
        if (entry.size > 0) {
            num_objects_++;
            // normal object, is not deleted, add it to indexes
            IndexEntry old_entry;
            if (!indexs_->Insert(entry, &old_entry)) {
                status.set_code(kDataCorrupted);
                status.set_msg("index data maybe corrupted! duplicated sequence_number %ld",
                        entry.sequence_number);
                return status;
            }
            assert(entry.offset + entry.size > data_offset_);
            data_offset_ = entry.offset + entry.size;
        } else {
            // object is deleted
            indexs_->Delete(entry.object_id);
        }
        index_offset_ += entry_size;
        max_sequence_number_ = entry.sequence_number;
    }

    if (max_sequence_number_ < synced_sequence_number_) {
        status.set_code(kDataCorrupted);
        status.set_msg("max_sequence_number %ld less than synced_sequence_number %ld, block "
                       "maybe corrupted", max_sequence_number_, synced_sequence_number_);
    }

    log_->Write(LL_NOTICE, "finish open block with %s", status.ToString().c_str());
    return status;
}

Status EagleBlock::Create(const std::string& folder, int64_t max_block_size) {
    Status status;
    // 1. folder should be empty
    DIR* dir;
    struct dirent* file;
    errno = 0;
    dir = opendir(folder.c_str());
    if (NULL == dir) {
        status.set_code(kIOError);
        status.set_msg("failed to open %s, %m", folder.c_str());
        return status;
    }
    while ((file = readdir(dir)) != NULL) {
        std::string tmp_file(file->d_name);
        if (tmp_file.compare(".") == 0 || tmp_file.compare("..") == 0) {
            continue;
        }

        status.set_code(kInvalidArg);
        status.set_msg("folder %s is not empty!", folder.c_str());
        closedir(dir);
        return status;

    }
    closedir(dir);

    // 2. init some member variables
    status = Init(folder, max_block_size, false);
    if (status.code() != kOk) {
        return status;
    }

    // 5. persistent manifest
    status = StoreManifest();
    if (status.code() != kOk) {
        return status;
    }

    // 6. create current file
    status = StoreCurrentSubdir(current_subdir_);
    if (status.code() != kOk) {
        return status;
    }

    log_->Write(LL_NOTICE, "finish create block with %s", status.ToString().c_str());
    return status;
}

void EagleBlock::Sync() {
    int64_t current_max_seq = max_sequence_number_;
    // sync data file
    errno = 0;
    if (0 != fsync(data_fd_)) {
        log_->Write(LL_ERROR, "failed to sync data file, %m");
        return;
    }

    // sync index file
    errno = 0;
    if (0 != fsync(index_fd_)) {
        log_->Write(LL_ERROR, "failed to sync index file, %m");
        return;
    }

    synced_sequence_number_ = current_max_seq;
    // persistent manifest
    Status status = StoreManifest();
    if (status.code() != kOk) {
        log_->Write(LL_ERROR, "failed to store manifest with %s", status.ToString().c_str());
    }
}

Status EagleBlock::Compact(int64_t end_sequence_number, EagleBlock** new_block) {
    // set block status; preventing new put & delete
    BlockStatus old_status = GetStatus();
    SetStatus(kCompacting);

    log_->Write(LL_NOTICE, "start to compact");
    BlockCompact block_compact(this, log_);
    Status status = block_compact.Run(end_sequence_number);
    if (status.code() == kOk) {
        // open new block
        status = OpenBlock(root_dir_, new_block);
    }

    if (status.code() != kOk) {
        // reset block status
        SetStatus(old_status);
    }
    log_->Write(LL_NOTICE, "finish compact %s", status.ToString().c_str());
    return status;
}

}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
