/*
 * Copyright (c) 2017 LIHAIBING. All Rights Reserved
 *
 * @file eagleblock.h
 * @author lihaibing(593255200@qq.com)
 * @date 2017/07/22 16:12:10
 * @brief
 *
*/
#ifndef _EAGLEFS_EAGLEBLOCK_H_
#define _EAGLEFS_EAGLEBLOCK_H_

#include <unistd.h>
#include "eagleengine/common.h"
#include "eagleengine/status.h"
#include "eagleengine/hash_table.h"
#include "eagleengine/log/log.h"

namespace eagleengine {

static const int kAveObjectSize = 700 * 1024;
static const char* const kCurrentFile = "current";
static const char* const kDataFile = "dat";
static const char* const kIndexFile = "idx";
static const char* const kManifestFile = "manifest";
static const uint64_t kMagicNumber = 0x7e7e7e7e7e7e7e7eul;
static const int kManifestSizeLimit = 1024;

struct IndexEntry {
    int64_t sequence_number;
    int64_t object_id;
    int64_t offset;
    int size;

    IndexEntry() : sequence_number(-1), object_id(-1), offset(0), size(0) {
    }

    int64_t key() const {
        return object_id;
    }
};

struct ObjectHeader {
    uint64_t magic;
    int64_t object_id;
    int size;
    uint32_t crc;
    char reserved[8];

    ObjectHeader() : magic(kMagicNumber), object_id(-1), size(0), crc(0) {
        memset(reserved, 0, sizeof(reserved));
    }
};

struct Manifest {
    int64_t max_block_size;
    int64_t synced_sequence_number;
    int64_t magic_number;
    Manifest() : max_block_size(0), synced_sequence_number(-1), magic_number(kMagicNumber) {
    }
};


// Note:
// 1. PutObject, DeleteObject , are not thread safe; those func should be called serialized;
//    GetObject also is not thread safee, but it canbe called with PutObject & DeleteObject
//    concurrently;
// 2. Sync() should be called periodically ; thus objects and indexes canbe flushed to disk
//    permanently
//
class EagleBlock {
public:
    virtual ~EagleBlock();
    Status PutObject(const std::string& content, int64_t* object_id);
    Status DeleteObject(int64_t object_id);
    Status GetObject(int64_t object_id, std::string* result);

    // should call this func periodically
    // this func fsync data&index to disk
    void Sync();

    // when there is lots of deleted objects in this block, should call this func to
    // recycle space; when this func is called, the block's status will be set as kCompacting,
    // and putobject & deleteobject will return error
    //
    // end_sequence_number: compact all deleted objects whose object id doesn't larger than it;
    // value of end_sequence_number cannot larger than synced_sequence_number;
    // after calling this func; the old EagleBlock object should be deleted ASAP;
    Status Compact(int64_t end_sequence_number, EagleBlock** new_block);

    void SetStatus(BlockStatus status) {
        status_ = status;
    }
    BlockStatus GetStatus() {
        return status_;
    }
    bool IsNormal() { return status_ == kNormal; }

    int64_t max_sequence_number() {
        return max_sequence_number_;
    }
    int64_t synced_sequence_number() {
        return synced_sequence_number_;
    }

    int64_t num_objects() {
        return num_objects_;
    }
    int64_t deleted_num_objects() {
        return num_objects_ - indexs_->size();
    }

    std::string current_subdir() {
        return current_subdir_;
    }
    std::string root_dir() {
        return root_dir_;
    }
    int64_t max_block_size() {
        return max_block_size_;
    }


    static Status OpenBlock(const std::string& folder, EagleBlock** result);
    static Status CreateBlock(const std::string& folder, EagleBlock** result,
                              int64_t max_block_size = kDefaultMaxBlockSize);
private:
    friend class BlockCompact;
    EagleBlock();
    Status ValidateObject(const IndexEntry& entry);
    Status Create(const std::string& folder, int64_t max_block_size);
    Status Open(const std::string& folder);
    Status Init(const std::string& folder, int64_t max_block_size, bool exist);

    std::string GetBlockRootDir(const std::string& subdir);
    std::string GetFilePath(const std::string& subdir, const std::string& file_name);
    Status StoreCurrentSubdir(const std::string& subdir);

    static Status StoreManifestEx(const Manifest& manifest, const std::string& manifest_file);
    Status StoreManifest();
    Status GetManifest(Manifest* manifest);

    Status GetCurrentSubdir(std::string* result);
private:
    DISALLOW_COPY_AND_ASSIGN(EagleBlock);
    std::string current_subdir_;
    std::string root_dir_;
    int64_t max_block_size_;
    int data_fd_;
    int index_fd_;

    volatile BlockStatus status_;
    volatile int64_t max_sequence_number_;
    volatile int64_t synced_sequence_number_;
    int64_t data_offset_;
    int64_t index_offset_;
    HashTable<IndexEntry>* indexs_;
    char* internal_buf_;

    int64_t num_objects_;

    Log* log_;
};

}

#endif  //_EAGLEFS_EAGLEBLOCK_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
