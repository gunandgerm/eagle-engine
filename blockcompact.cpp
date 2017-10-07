/*
 * Copyright (c) 2017 LIHAIBING. All Rights Reserved
 *
 * @file blockcompact.cpp
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
#include "eagleengine/crc32c.h"
#include "eagleengine/blockcompact.h"

namespace eagleengine {

BlockCompact::BlockCompact(EagleBlock* block, Log* log) {
    block_ = block;
    log_ = log;
    data_offset_ = 0;
    max_sequence_number_ = -1;
    internal_buf_ = (char*)malloc(kMaxObjectSize);
}

BlockCompact::~BlockCompact() {
    free(internal_buf_);
}

Status BlockCompact::OpenOldFDs(BlockFDs* fds) {
    Status status;
    std::string data_file = block_->GetFilePath(block_->current_subdir(), kDataFile);

    errno = 0;
    fds->data_fd = open(data_file.c_str(), O_RDONLY);
    if (fds->data_fd < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to open %s, %m", data_file.c_str());
        return status;
    }

    std::string index_file = block_->GetFilePath(block_->current_subdir(), kIndexFile);
    errno = 0;
    fds->index_fd = open(index_file.c_str(), O_RDONLY);
    if (fds->index_fd < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to open %s, %m", index_file.c_str());
    }

    return status;

}

Status BlockCompact::CreateNewFDs(const std::string& subdir, BlockFDs* fds) {
    Status status;
    // 1. create target sub dir 
    std::string newblock_dir = block_->root_dir();
    newblock_dir.append("/");
    newblock_dir.append(subdir);
    errno = 0;
    if (0 != mkdir(newblock_dir.c_str(), 0744) && errno != EEXIST ) {
        status.set_code(kIOError);
        status.set_msg("failed to create %s, %m", newblock_dir.c_str());
        return status;
    }

    // 2. create data file
    std::string data_file = newblock_dir;
    data_file.append("/");
    data_file.append(kDataFile);
    errno = 0;
    fds->data_fd = open(data_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND | O_LARGEFILE, 0644);
    if (fds->data_fd < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to open %s, %m", data_file.c_str());
        return status;
    }

    // 3. create index file
    std::string index_file = newblock_dir;
    index_file.append("/");
    index_file.append(kIndexFile);
    errno = 0;
    fds->index_fd = open(index_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);
    if (fds->index_fd < 0) {
        status.set_code(kIOError);
        status.set_msg("failed to open %s, %m", index_file.c_str());
    }

    return status;
}

Status BlockCompact::CopyObject(const BlockFDs& new_block_fds, int data_fd, const IndexEntry& entry) {
    Status status;
    int written_size = -1;
    ObjectHeader header;
    const int header_size = sizeof(header);
    int64_t start_offset = entry.offset - header_size;
    errno = 0;
    // seek to the begin offset of object header
    if (lseek(data_fd, start_offset, SEEK_SET) == -1) {
        status.set_code(kIOError);
        status.set_msg("failed to seek to %ld for data file,last sequence_number %ld, %m",
                start_offset, entry.sequence_number);
        return status;
    }

    // read object header
    errno = 0;
    int read_size = read(data_fd, &header, header_size);
    if (read_size != header_size) {
        status.set_code(kIOError);
        status.set_msg("only read %d bytes for object header but expect %d bytes, object"
                " sequence_number %ld, %m", read_size, header_size, entry.sequence_number);
        return status;
    }

    // check object id
    if (header.object_id != entry.object_id) {
        status.set_code(kDataCorrupted);
        status.set_msg("object id in header %ld but object id in index is %ld",
                header.object_id, entry.object_id);
        return status;
    }

    IndexEntry new_entry;
    const int entry_size = sizeof(new_entry);
    new_entry.sequence_number = entry.sequence_number;
    new_entry.object_id = entry.object_id;
    new_entry.size = entry.size;
    if (entry.size > 0) {
        // read object data
        errno = 0;
        read_size = read(data_fd, internal_buf_, entry.size);
        if (read_size != entry.size) {
            status.set_code(kIOError);
            status.set_msg("only read %d bytes for object but expect %d bytes, sequence_number "
                    "%ld, %m", read_size, entry.size, entry.sequence_number);
            return status;
        }

        // check crc
        if (header.crc != Adler32_Value(internal_buf_, entry.size)) {
            status.set_code(kDataCorrupted);
            status.set_msg("failed to check crc for object %ld", entry.object_id);
            return status;
        }

        // write object header
        errno = 0;
        written_size = write(new_block_fds.data_fd, &header, header_size);
        if (written_size != header_size) {
            status.set_code(kIOError);
            status.set_msg("failed to write object header, only write %d bytes but expect "
                    "%d bytes, %m", written_size, header_size);
            return status;
        }

        // write data
        errno = 0;
        written_size = write(new_block_fds.data_fd, internal_buf_, entry.size);
        if (written_size != entry.size) {
            status.set_code(kIOError);
            status.set_msg("failed to write object, only write %d bytes but expect %d bytes, "
                    "%m", written_size, entry.size);
            return status;
        }

        // add it to indexes
        new_entry.offset = data_offset_ + header_size;
        indexes_.insert(std::pair<int64_t, IndexEntry>(new_entry.object_id, new_entry));
        data_offset_ += header_size;
        data_offset_ += entry.size;
    } else {
        // this object is marked as deleted
        std::map<int64_t, IndexEntry>::iterator it = indexes_.find(entry.object_id);
        if (it != indexes_.end()) {
            new_entry.offset = (it->second).offset;
        } else {
            log_->Write(LL_FATAL, "no object %ld exist!this is should not happen", entry.object_id);
            exit(1);
        }
        indexes_.erase(entry.object_id);
    }

    // write index file
    max_sequence_number_ = new_entry.sequence_number;
    written_size = write(new_block_fds.index_fd, &new_entry, entry_size);
    if (written_size != entry_size) {
        status.set_code(kIOError);
        status.set_msg("failed to write index, only write %d bytes but expect %d bytes, "
                       "%m", written_size, entry_size);
    }

    return status;
}

Status BlockCompact::CopySyncedObjects(int64_t end_sequence_number, const BlockFDs& new_block_fds,
                                       const BlockFDs& old_block_fds, IndexEntry* last_entry) {
    Status status;
    const int entry_size = sizeof(*last_entry);
    std::map<int64_t, IndexEntry> indexes;
    while (last_entry->sequence_number < end_sequence_number) {
        // read indexes for all synced objects
        errno = 0;
        int read_size = read(old_block_fds.index_fd, last_entry, entry_size);
        if (read_size != entry_size) {
            status.set_code(kIOError);
            status.set_msg("only read %d bytes from index file but expect %d bytes, last "
                           "sequence_number %ld, %m", read_size, entry_size,
                           last_entry->sequence_number);
            return status;
        }

        if (last_entry->sequence_number > end_sequence_number) {
            break;
        }

        if (last_entry->size <= 0) {
            // this object is marked as deleted
            indexes.erase(last_entry->object_id);
        } else {
            indexes.insert(std::pair<int64_t, IndexEntry>(last_entry->object_id, *last_entry));
        }
    }

    std::map<int64_t, IndexEntry>::const_iterator it = indexes.cbegin();
    for (; it != indexes.cend(); ++it) {
        // copy undeleted object
        status = CopyObject(new_block_fds, old_block_fds.data_fd, it->second);
        if (status.code() != kOk) {
            return status;
        }
    }

    return status;
}

Status BlockCompact::CopyRemainingObjects(int64_t end_sequence_number, const BlockFDs& new_block_fds,
                                      const BlockFDs& old_block_fds, IndexEntry* last_entry) {
    Status status;
    const int entry_size = sizeof(*last_entry);
    bool copy_last_object = false;
    if (last_entry->sequence_number > end_sequence_number) {
        copy_last_object = true;
    }
    while (last_entry->sequence_number < block_->max_sequence_number()) {
        if (!copy_last_object) {
            // read indexes for all remainning objects
            errno = 0;
            int read_size = read(old_block_fds.index_fd, last_entry, entry_size);
            if (read_size != entry_size) {
                status.set_code(kIOError);
                status.set_msg("only read %d bytes from index file but expect %d bytes, last "
                        "sequence_number %ld, %m", read_size, entry_size,
                        last_entry->sequence_number);
                return status;
            }
        } else {
            copy_last_object = false;
        }

        status = CopyObject(new_block_fds, old_block_fds.data_fd, *last_entry);
        if (status.code() != kOk) {
            return status;
        }
    }

    return status;
}

Status BlockCompact::Run(int64_t end_sequence_number) {
    Status status;
    if (end_sequence_number > block_->synced_sequence_number()) {
        status.set_code(kInvalidArg);
        status.set_msg("end_sequence_number cannot larger than synced_sequence_number");
        return status;
    }
    std::string subdir;
    if (block_->current_subdir().compare(kDefaultSubdir) == 0) {
        subdir = "1";
    } else {
        subdir = kDefaultSubdir;
    }

    // create fds for new block
    BlockFDs new_block_fds;
    status = CreateNewFDs(subdir, &new_block_fds);
    if (status.code() != kOk) {
        return status;
    }

    // open fds for old block
    BlockFDs old_block_fds;
    status = OpenOldFDs(&old_block_fds);
    if (status.code() != kOk) {
        return status;
    }

    // copy synced objects
    IndexEntry last_entry;
    status = CopySyncedObjects(end_sequence_number, new_block_fds, old_block_fds,
                               &last_entry);
    if (status.code() == kOk) {
        // copy remainning objects
        status = CopyRemainingObjects(end_sequence_number, new_block_fds, old_block_fds,
                                      &last_entry);
    }

    // sync data & index
    if (status.code() == kOk) {
        if (0 != fsync(new_block_fds.data_fd)) {
            status.set_code(kIOError);
            status.set_msg("failed to fsync data file %s, %m", block_->GetFilePath(subdir, kDataFile).c_str());
        }
        if (0 != fsync(new_block_fds.index_fd)) {
            status.set_code(kIOError);
            status.set_msg("failed to fsync index file %s, %m", block_->GetFilePath(subdir, kIndexFile).c_str());
        }
    }

    // create manifest
    if (status.code() == kOk) {
        Manifest manifest;
        manifest.max_block_size = block_->max_block_size();
        manifest.synced_sequence_number = max_sequence_number_;
        status = block_->StoreManifestEx(manifest, block_->GetFilePath(subdir, kManifestFile));
    }

    // set new current dir
    if (status.code() == kOk) {
        status = block_->StoreCurrentSubdir(subdir);
    }

    return status;
}

}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
