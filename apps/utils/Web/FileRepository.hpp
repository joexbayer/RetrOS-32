#ifndef FILEREPOSITORY_HPP
#define FILEREPOSITORY_HPP

#include <utils/StdLib.hpp>
#include <lib/syscall.h>
#include <fs/fat16.h>
#include <fs/fs.h>

namespace web {

    /* Allocated content is managed by repository, not the user. */
    typedef struct {
        char* content;
        size_t size;
    } FileData;

    class FileRepository {
    public:
        FileRepository() {}
        ~FileRepository() {
            for (size_t i = 0; i < MAX_CACHE_SIZE; i++) {
                resetCache(i);
            }
        }

        static const size_t MAX_FILE_SIZE = 6 * 1024; /* 6KB */

        FileData getFile(const char* path) {
            /* Check if file is already cached */
            FileData cached = findInCache(path);
            if (cached.content != nullptr) {
                return cached;
            }

            /* Load file from disk */
            int fd = open(path, FS_FILE_FLAG_READ);
            if (fd < 0) {
                return {nullptr, 0};
            }

            /* For now we only support 6kb files */
            char* buffer = (char*)malloc(MAX_FILE_SIZE);
            if (!buffer) {
                fclose(fd);
                return {nullptr, 0};
            }

            int size = read(fd, buffer, MAX_FILE_SIZE);
            if (size <= 0) {
                free(buffer);
                fclose(fd);
                return {nullptr, 0};
            }

            /* Cache the file */
            size_t slot = getNextCacheSlot();
            cache[slot].path = path;
            cache[slot].content = buffer;
            cache[slot].size = size;
            cache[slot].loaded = true;

            fclose(fd);

            return {buffer, size};
        }

    private:
        struct CachedFile {
            const char* path;
            char* content;
            size_t size;
            bool loaded;
        };

        static const size_t MAX_CACHE_SIZE = 32;
        CachedFile cache[MAX_CACHE_SIZE];
        size_t cache_count = 0;
        size_t next_cache_index = 0;

        void resetCache(int index){
            if (index < 0 || (size_t)index >= cache_count) {
                return;
            }
            if (cache[index].loaded) {
                free(cache[index].content);
                cache[index].content = nullptr;
                cache[index].loaded = false;
            }
        }

        size_t getNextCacheSlot() {
            size_t slot = next_cache_index;
            resetCache(slot);
            
            next_cache_index = (next_cache_index + 1) % MAX_CACHE_SIZE;
            if (cache_count < MAX_CACHE_SIZE) {
                cache_count++;
            }
            
            return slot;
        }
        
        FileData findInCache(const char* path) {
            for (size_t i = 0; i < cache_count; i++) {
                if (cache[i].loaded && strcmp(cache[i].path, path) == 0) {
                    return {cache[i].content, cache[i].size};
                }
            }
            return {nullptr, 0};
        }
    };
}

#endif // FILEREPOSITORY_HPP
