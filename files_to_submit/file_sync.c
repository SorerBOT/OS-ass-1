#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_PATH 1024
#define READ_BATCH_SIZE 100

typedef struct timespec timespec;

typedef enum  {
    false,
    true
} boolean;

typedef struct {
    char* name;
    timespec lastModified;
} FileData;

typedef struct {
    char* path;
    FileData* files;
    int filesCount;
} DirData;

boolean isDirExists(const char* path);
char* getDirName(const char* path);
char* getFullPath(const char* basePath, const char* path);
void sortFilesLexicographically(DirData* dir);
void freeDirData(DirData* dir);
void syncDirs(const DirData* src, const DirData* dest);
FileData* findFile(const FileData* file, const DirData* dir);
boolean isFirstNewer(const FileData* first, const FileData* second);

void __DEBUG_print_files_data(const DirData dir, const char* message);

char* __pwd();
FileData*__ls(const char* cwd, int* length);
void __mkdir(const char* dirName);
void __cd(const char* path);
boolean __diff(const DirData* dest, const DirData* src, const FileData* file);
void __cp(const DirData* dest, const DirData* src, const FileData* file);

int main(int argc, char** argv)
{
    DirData src;
    DirData dest;

    char* srcDirName = NULL;
    char* destDirName = NULL;
    char* cwd = NULL;

    cwd = __pwd();
    printf("Current working directory: %s\n", cwd);

    if (argc != 3)
    {
        printf("Usage: file_sync <source_directory> <destination_directory>\n");
        exit(EXIT_FAILURE);
    }

    srcDirName = getDirName(argv[1]);
    destDirName = getDirName(argv[2]);

    if (!isDirExists(argv[1]))
    {
        printf("Error: Source directory '%s' does not exist.\n", srcDirName);
        exit(EXIT_FAILURE);
    }
    if (!isDirExists(argv[2]))
        __mkdir(destDirName);

    src.path = getFullPath(cwd, srcDirName);
    dest.path = getFullPath(cwd, destDirName);

    printf("Synchronizing from %s to %s\n", src.path, dest.path);

    __cd(src.path);
    free(cwd);
    cwd = __pwd();
    src.files =__ls(cwd, &src.filesCount);

    __cd(dest.path);
    free(cwd);
    cwd = __pwd();
    dest.files = __ls(cwd, &dest.filesCount);

    sortFilesLexicographically(&src);
    sortFilesLexicographically(&dest);

    syncDirs(&src, &dest);

    printf("Synchronization complete.\n");

    free(srcDirName);
    free(destDirName);
    free(cwd);
    freeDirData(&src);
    freeDirData(&dest);
}

boolean isDirExists(const char* path)
{
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return false;
}

char* getDirName(const char* path)
{
    int idx = 0, lastSlashIdx = 0;
    char* result = NULL;

    while (*(path + idx))
    {
        if (path[idx] == '/')
            lastSlashIdx = idx;
        idx++;
    }

    result = lastSlashIdx
        ? strdup(path + lastSlashIdx + 1)
        : strdup(path);

    if (result == NULL)
    {
        exit(EXIT_FAILURE);
    }

    return result;
}

void __mkdir(const char* dirName)
{
    int status = 0;
    pid_t pid = fork();

    switch (pid)
    {
        case -1:
            perror("fork failed");
            exit(EXIT_FAILURE);
        case 0:
            execlp("/usr/bin/mkdir", "mkdir", dirName, NULL);

            perror("Failed to create dir");
            exit(EXIT_FAILURE);
        default:
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int exit_status = WEXITSTATUS(status);
                if (exit_status == EXIT_SUCCESS)
                    printf("Created destination directory '%s'.\n", dirName);
                else
                {
                    perror("mkdir failed");
                    exit(EXIT_FAILURE);
                }
            }
    }
}

char* __pwd()
{
    char* buffer = (char*)malloc(MAX_PATH * sizeof(char));

    if (buffer == NULL)
    {
        perror("Malloc failed");
        exit(EXIT_FAILURE);
    }

    if (getcwd(buffer, MAX_PATH) == NULL)
    {
        perror("The current working directory was more than 1024 characters long");
        exit(EXIT_FAILURE);
    }

    return buffer;
}

void __cd(const char* path)
{
    if (chdir(path) != 0)
    {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }
}

char* getFullPath(const char* basePath, const char* path)
{
    char* fullPath = (char*)malloc((strlen(basePath) + strlen(path) + 2) * sizeof(char));
    if (fullPath == NULL)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    strcpy(fullPath, basePath);
    strcat(fullPath, "/");
    strcat(fullPath, path);

    return fullPath;
}

FileData* __ls(const char* cwd, int* length)
{
    DIR* dir = NULL;
    struct dirent *entry = NULL;
    char* filePath = NULL;
    FileData* filesData = NULL;
    int filesCount = 0;

    struct stat fileStat;
    dir = opendir(cwd);
    if (dir == NULL)
    {
        perror("opendir failed");
        exit(EXIT_FAILURE);
    }


    while ((entry = readdir(dir)) != NULL)
    {
        char* fileName = NULL;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        filePath = getFullPath(cwd, entry->d_name);
        if (stat(filePath, &fileStat) == -1) {
            perror("stat to read file data failed");
            continue;
        }

        if (S_ISDIR(fileStat.st_mode)) continue;
        
        filesCount++;
        filesData = (FileData*)realloc(filesData, filesCount * sizeof(FileData));
        if (filesData == NULL)
        {
            perror("Realloc failed");
            exit(EXIT_FAILURE);
        }

        fileName = (char*)malloc(strlen(entry->d_name) * sizeof(char));
        if (fileName == NULL)
        {
            perror("Malloc failed");
            exit(EXIT_FAILURE);
        }
        strcpy(fileName, entry->d_name);

        filesData[filesCount - 1] = (FileData)
        {
            fileName,
            fileStat.st_mtim
        };
    }

    closedir(dir);

    *length = filesCount;
    return filesData;
}

void sortFilesLexicographically(DirData* dir)
{
    FileData temp;
    char* name1 = NULL;
    char* name2 = NULL;

    for (int i = 0; i < dir->filesCount - 1; i++)
    {
        name1 = dir->files[i].name;
        name2 = dir->files[i + 1].name;

        if (strcmp(name1, name2) <= 0) continue;
        temp = dir->files[i + 1];
        dir->files[i + 1] = dir->files[i];
        dir->files[i] = temp;
        i = -1; // start over
    }
}

void freeDirData(DirData* dir)
{
    for (int i = 0; i < dir->filesCount; i++)
    {
        free(dir->files[i].name);
    }
    free(dir->files);
    free(dir->path);
}

void __DEBUG_print_files_data(const DirData dir, const char* message)
{
    if (message != NULL)
        printf("%s\n", message);

    printf("Reading %d files from path: %s\n", dir.filesCount, dir.path);

    for (int i = 0; i < dir.filesCount; i++)
    {
        printf("%s\n", dir.files[i].name);
        printf("%ld\n", dir.files[i].lastModified.tv_sec);
        printf("======================================\n");
    }
}

void syncDirs(const DirData* src, const DirData* dest)
{
    for (int i = 0; i < src->filesCount; i++)
    {
        FileData* currentFile = &src->files[i];
        FileData* correspondingDestFile = NULL;

        if ((correspondingDestFile = findFile(currentFile, dest)) == NULL)
        {
            printf("New file found: %s\n", currentFile->name);
            __cp(dest, src, currentFile);
            continue;
        }
        else
        {
            if (__diff(dest, src, currentFile))
            {
                if (isFirstNewer(currentFile, correspondingDestFile))
                {
                    __cp(dest, src, currentFile);
                    printf("File %s is newer in source. Updating...\n", currentFile->name);
                    continue;
                }
                else
                {
                    printf("File %s is newer in destination. Skipping...\n", currentFile->name);
                    continue;
                }
            }
            else
            {
                printf("File %s is identical. Skipping...\n", currentFile->name);
                continue;
            }
        }
    }
}

FileData* findFile(const FileData* file, const DirData* dir)
{
    for (int i = 0; i < dir->filesCount; i++)
    {
        if (strcmp(file->name, dir->files[i].name) == 0) return &dir->files[i];
    }
    return NULL;
}

boolean __diff(const DirData* dest, const DirData* src, const FileData* file)
{
    int null_fd = -1;

    char* srcFileFullPath = NULL;
    char* destFileFullPath = NULL;

    int status = 0;
    pid_t pid = -1;

    srcFileFullPath = getFullPath(src->path, file->name);
    destFileFullPath = getFullPath(dest->path, file->name);

    pid = fork();

    switch (pid)
    {
        case -1:
            perror("fork failed");
            exit(EXIT_FAILURE);
        case 0:
            null_fd = open("/dev/null", O_WRONLY);
            dup2(null_fd, STDOUT_FILENO);
            close(null_fd);

            execlp("/usr/bin/diff", "diff", "-q", srcFileFullPath, destFileFullPath, NULL);

            perror("failed to get diffs");
            exit(EXIT_FAILURE);
        default:
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int exit_status = WEXITSTATUS(status);
                switch (exit_status)
                {
                    case 0:
                        return false;
                    case 1:
                        return true;
                    case 2:
                    default:
                        perror("diff failed");
                        exit(EXIT_FAILURE);
                }
            }
    }
    perror("unexpected error");
    exit(EXIT_FAILURE);
}

boolean isFirstNewer(const FileData* first, const FileData* second)
{
    if (first->lastModified.tv_sec < second->lastModified.tv_sec) return true;
    else if (first->lastModified.tv_sec == second->lastModified.tv_sec)
    {
        if (first->lastModified.tv_nsec < second->lastModified.tv_nsec) return true;

    }
    return false;
}

void __cp(const DirData* dest, const DirData* src, const FileData* file)
{
    char* srcFileFullPath = NULL;
    char* destFileFullPath = NULL;

    int status = 0;
    pid_t pid = -1;

    srcFileFullPath = getFullPath(src->path, file->name);
    destFileFullPath = getFullPath(dest->path, file->name);

    pid = fork();

    switch (pid)
    {
        case -1:
            perror("fork failed");
            exit(EXIT_FAILURE);
        case 0:
            execlp("/usr/bin/cp", "cp", srcFileFullPath, destFileFullPath, NULL);

            perror("failed to cp");
            exit(EXIT_FAILURE);
        default:
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int exit_status = WEXITSTATUS(status);
                if (exit_status == EXIT_SUCCESS)
                {
                    printf("Copied: %s -> %s\n", srcFileFullPath, destFileFullPath);
                }
                if (exit_status == EXIT_FAILURE)
                {
                    perror("cp failed");
                    exit(EXIT_FAILURE);
                }
            }
    }
}
