/* Copyright 2024 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include "libs/base/filesystem.h"
#include "libs/base/queue_task.h"
#include "third_party/littlefs-fuse/littlefs/lfs.h"

#include "../../lib/arduprof/ArduProf.h"

class FsUtil
{
public:
    static void printFilesystem(void)
    {
        lfs_dir_t root;
        CHECK(lfs_dir_open(coralmicro::Lfs(), &root, "/") >= 0);
        PRINTLN("filesystem:\r\n==================================================");
        printDirectory(&root, "", 0);
        PRINTLN("==================================================");
        CHECK(lfs_dir_close(coralmicro::Lfs(), &root) >= 0);
    }

    static void printDirectory(lfs_dir_t *dir, const char *path, int num_tabs)
    {
        constexpr int kMaxDepth = 3;
        if (num_tabs > kMaxDepth)
        {
            return;
        }

        lfs_info info;
        while (lfs_dir_read(coralmicro::Lfs(), dir, &info) > 0)
        {
            if (info.name[0] == '.')
            {
                continue;
            }

            for (int i = 0; i < num_tabs; ++i)
            {
                PRINT("\t");
            }

            PRINT("%s", info.name);

            if (info.type == LFS_TYPE_DIR)
            {
                char subpath[LFS_NAME_MAX];
                PRINT("/\r\n");
                lfs_dir_t subdir;
                snprintf(subpath, LFS_NAME_MAX, "%s/%s", path, info.name);
                CHECK(lfs_dir_open(coralmicro::Lfs(), &subdir, subpath) >= 0);
                printDirectory(&subdir, subpath, num_tabs + 1);
                CHECK(lfs_dir_close(coralmicro::Lfs(), &subdir) >= 0);
            }
            else
            {
                PRINT("\t\t%ld\r\n", info.size);
            }
        }
    }
};
