#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_PATH 1024

typedef enum  {
    false,
    true
} boolean;

boolean isDirExists(const char* path);
char* getDirName(const char* path);
char* getFullPath(const char* basePath, char* path);

char* __pwd();
void __ls(const char* cwd);
void __mkdir(const char* dirName);
void __cd(const char* path);

int main(int argc, char** argv)
{
    char* srcDirName = NULL;
    char* destDirName = NULL;
    char* srcPath = NULL;
    char* destPath = NULL;
    char* cwd = NULL;

    if (argc != 3)
    {
        printf("Usage: ./file_sync <source_directory> <destination_directory>\n");
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

    cwd = __pwd();
    printf("Current working directory: %s\n", cwd);

    srcPath = getFullPath(cwd, srcDirName);
    __cd(srcPath);

    free(cwd);
    cwd = __pwd();
    __ls(cwd);
    // destPath = getFullPath(cwd, destDirName);

    

    free(srcDirName);
    free(destDirName);
    free(srcPath);
    free(destPath);
    free(cwd);
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
    int status;
    pid_t pid = fork();

    switch (pid)
    {
        case -1:
            perror("fork failed");
            exit(EXIT_FAILURE);
        case 0:
            execlp("mkdir", "mkdir", dirName, NULL);

            perror("Failed to create dir");
            exit(EXIT_FAILURE);
        default:
            status = 0;

            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int exit_status = WEXITSTATUS(status);
                if (exit_status == EXIT_SUCCESS)
                    printf("Created destination directory '%s'.\n", dirName);
                else
                    exit(EXIT_FAILURE);
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

char* getFullPath(const char* basePath, char* path)
{
    char* fullPath = (char*)malloc((strlen(basePath) + strlen(path) + 2) * sizeof(char));

    strcpy(fullPath, basePath);
    strcat(fullPath, "/");
    strcat(fullPath, path);

    return fullPath;
}

void __ls(const char* cwd)
{
    DIR* dir = NULL;
    struct dirent *entry = NULL;
    char* filePath = NULL;

    struct stat fileStat;

    dir = opendir(cwd);
    if (dir == NULL)
    {
        perror("opendir failed");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        filePath = getFullPath(cwd, entry->d_name);
        if (stat(filePath, &fileStat) == -1) {
            perror("stat to read file data failed");
            continue;
        }

        printf("Name: %s\n", entry->d_name);
        printf("Type: %s\n", S_ISDIR(fileStat.st_mode) ? "Directory" : "File");
        printf("Size: %ld bytes\n", fileStat.st_size);
        printf("Permissions: %o\n", fileStat.st_mode & 0777);
        printf("----------\n");
    }

    closedir(dir);
}
