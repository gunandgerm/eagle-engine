/*
 * Copyright (c) 2017 LIHAIBING. All Rights Reserved
 *
 * @file eagleblock.h
 * @author lihaibing(593255200@qq.com)
 * @date 2017/07/22 16:12:10
 * @brief
 *
*/
#ifndef _EAGLEFS_BLOCKCOMPACT_H_
#define _EAGLEFS_BLOCKCOMPACT_H_

#include <unistd.h>
#include "eagleengine/common.h"
#include "eagleengine/status.h"
#include "eagleengine/eagleblock.h"
#include "eagleengine/log/log.h"

namespace eagleengine {

struct BlockFDs {
    int data_fd;
    int index_fd;
    BlockFDs() : data_fd(-1), index_fd(-1) {
    }

    virtual ~BlockFDs() {
        if (data_fd >= 0) {
            close(data_fd);
        }

        if (index_fd >= 0) {
            close(index_fd);
        }
    }
};

class BlockCompact {
public:
    BlockCompact(EagleBlock* block, Log* log);
    virtual ~BlockCompact();
    Status Run(int64_t end_sequence_number);

private:
    // following funcs are related with compacting
    Status CreateNewFDs(const std::string& subdir, BlockFDs* fds);
    Status OpenOldFDs(BlockFDs* fds);
    Status CopyObject(const BlockFDs& new_block_fds, int old_data_fd, const IndexEntry& entry);
    Status CopySyncedObjects(int64_t end_sequence_number, const BlockFDs& new_block_fds,
                             const BlockFDs& old_block_fds, IndexEntry* last_entry);
    Status CopyRemainingObjects(int64_t end_sequence_number, const BlockFDs& new_block_fds,
                                const BlockFDs& old_block_fds, IndexEntry* last_entry);
private:
    std::map<int64_t, IndexEntry> indexes_;
    char* internal_buf_;
    int64_t data_offset_;
    int64_t max_sequence_number_;

    EagleBlock* block_;
    Log* log_;
};

}

#endif  //_EAGLEFS_BLOCKCOMPACT_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
