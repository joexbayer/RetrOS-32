#include <stdio.h>
#include <dirent.h>


#define DT_DIR 4

void traverseDirectories(const char *basePath) {
    DIR *dir;
    struct dirent *entry;

    // Open the directory
    dir = opendir(basePath);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    // Traverse the directory
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            // Ignore "." and ".." directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Recursively traverse subdirectories
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
            traverseDirectories(path);
        } else {
            // Print the file name
            printf("%s/%s\n", basePath, entry->d_name);
        }
    }

    // Close the directory
    closedir(dir);
}

int main() {
    const char *basePath = "."; // Current directory

    // Traverse directories and print file names
    traverseDirectories(basePath);

    return 0;
}

