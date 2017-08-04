/***************************************************************
 * Implementation of unix terminal ls command 				   *
 * Author: Shayne Kelly II                                     *
 * Date: July 15, 2017                                         *
 ***************************************************************/

/***************************************************************
 * Imports                                                     *
 ***************************************************************/
#include "UnixLs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/types.h>

/***************************************************************
 * Defines and Constants                                       *
 ***************************************************************/
#define ALL_FLAGS_SET   7
#define CURR_DIRECTORY  "."
#define DATE_LEN        22
#define INODE_LEN       14
#define NAME_LEN        400
#define PERMISSION_LEN  12
#define STAT_FAIL_CODE  -1

/***************************************************************
 * Statics                                                     *
 ***************************************************************/
FLAGS *flags;               /* ls options specified in args */
char *dirName;              /* The directory to ls, if specified. */
char fileNameBuf[NAME_LEN]; /* Store a directory name with file name appended. */
char linkNameBuf[NAME_LEN]; /* Store the filename that a symbolic link points to. */
char inodeBuf[INODE_LEN];   /* Store the inode number to print to the screen. */
uint8_t printDirTitle = 0;  /* Keeps track if the directory name should be printed before its contents. */

/***************************************************************
 * Function Prototypes                                         *
 ***************************************************************/
static void OpenDirAndPrintContents(char *dirToPrint);
static void HandleRecursion(DIR *dir, char *path);
static uint8_t ParseFlagsFromArgs(char *argString);
static void PrintFileNameInfo(char *pathToFile, char *dirName);
static void PrintSimpleNameWithIno(char *dirName, char *fileName);
static void PrintFileDescLine(char *dirName, char *fileName);
static void BuildPermissionString(char *string, mode_t permissions);
static void BuildDateString(char *string, time_t *time);

/***************************************************************
 * Global Functions                                            *
 ***************************************************************/

int main(int argc, char *argv[]) {
    flags = (FLAGS *)malloc(sizeof(FLAGS));
    dirName = ".";
    uint8_t argIndex = 1;

    /* Parse all arguments starting with - for flags. */
    char *currentArg = argv[argIndex];
    while (argIndex < argc && currentArg[0] == '-') {
        if (!ParseFlagsFromArgs(argv[argIndex])) {
            free(flags);
            exit(0);
        }
        currentArg = argv[++argIndex];
    }

    /**
     * Check if there are multiple directories to be printed.
     * The directory title should be printed before the contents).
     */
    if (argc - argIndex > 1) {
        printDirTitle = 1;
    }

    /* Print directory contents for the remaining arguments (if any, if not print contents of "."). */
    for (int i = argIndex; i < argc; i++) {
        currentArg = argv[i];
        OpenDirAndPrintContents(currentArg);
    }
    
    /* Clean up allocated dynamic memory. */
    free(flags);
    exit(0);
}

/***************************************************************
 * Static Functions                                            *
 ***************************************************************/

/**
 * Goes through the process of printing all the directory contents
 * based on the flags specified. Will recurse if -R is specified.
 */
static void OpenDirAndPrintContents(char *dirToPrint) {
    DIR *dir = NULL;
    struct dirent *dp = NULL;

    char localDir[NAME_LEN];
    memset(localDir, 0, sizeof(localDir));
    strncpy(localDir, dirToPrint, strlen(dirToPrint));

    /* Print the directory name if more than one is being printed (for recursion). */
    if (printDirTitle) {
        printf("\n%s:\n", localDir);
    }
    
    /* Open the directory. (current directory if none specified) */
    if ((dir = opendir(localDir)) == NULL) {
        perror("Failed to open the directory\n");
        free(flags);
        exit(0);
    }
    
    /* Loop reading each filename and add it to the list until the end of the directory. */
    do {
        errno = 0;
        if ((dp = readdir(dir)) != NULL) {
            /* Don't add hidden files to the list. */
            if (dp->d_name[0] != '.') {
                PrintFileNameInfo(localDir, dp->d_name);
            }
        }
    } while (dp != NULL);
    printDirTitle = 1;
    
    /* Handle recursion if necessary. */
    if (flags->fields.R == 1) {
        // rewinddir(dir);
        HandleRecursion(dir, localDir);
    }
    
    /* Close the directory. */
    if (closedir(dir)) {
        printf("Closing the directory failed.\n");
        free(flags);
        exit(0);
    }
}

/**
 * Re-traverses the directory specified, and prints the contents of any
 * directory within the specified directory.
 * dirName parameter should specify the entire path.
 */
static void HandleRecursion(DIR *dir, char *path) {
    struct stat statBuf;
    struct dirent *dp = NULL;
    rewinddir(dir);
    
    /* Loop reading each name in the directory and re-traverse any directories found.
     * Skip over anything that isn't a directory.
     */
    do {
        errno = 0;
        if ((dp = readdir(dir)) != NULL) {
            /* Don't include hidden file/directory names. */
            if (dp->d_name[0] != '.') {
                /* Append dirname to get full path if needed. */
                memset(fileNameBuf, 0, NAME_LEN);
                //printf("[DBG] dirName: %s\n", path);
                realpath(path, fileNameBuf);
                // printf("[DBG] realPath: %s\n", fileNameBuf);
                if (fileNameBuf[strlen(fileNameBuf) - 1] != '/') {
                    strncpy(fileNameBuf + strlen(fileNameBuf), "/", 1);
                }
                strncpy(fileNameBuf + strlen(fileNameBuf), &dp->d_name[0], strlen(&dp->d_name[0]));
                // printf("[DBG] fileNameBuf: %s\n", fileNameBuf);

                if (lstat(fileNameBuf, &statBuf) == STAT_FAIL_CODE) {
                    fprintf(stderr, "[DBG] Recur stat call failed for %s: \n", fileNameBuf);
                    perror("");
                    free(flags);
                    exit(0);
                }
                if (S_ISDIR(statBuf.st_mode)) {
                    // printf("[DBG] open dir: %s\n", fileNameBuf);
                    OpenDirAndPrintContents(fileNameBuf);
                }
            }
        }
    } while (dp != NULL);
}

/**
 * Take a string of args beginning with '-' and set flags for ls.
 * Only accept 'i', 'l', and 'R' flags.
 * Return 1 on success, 0 on failure.
 */
static uint8_t ParseFlagsFromArgs(char *argString) {
    /* Check if all the flags are already set. */ {
        if (flags->bits == ALL_FLAGS_SET) {
            return 1;
        }
    }
    
    /* Must specify a flag string with a dash. */
    if (argString[0] == '-') {
        /* Parse through all characters for flags */
        for (size_t i = 1; i < strlen(argString); i++) {
            switch (argString[i]) {
                case 'i':
                    flags->fields.i = 1;
                    break;
                case 'l':
                    flags->fields.l = 1;
                    break;
                case 'R':
                    flags->fields.R = 1;
                    break;
                default:
                    /* Fail on an invalid argument. */
                    printf("Invalid argument specified. Only i, l, or R allowed.\n");
                    return 0;
            }
        }
    } else {
        printf("Only the last argument can specify a directory.\n");
        return 0;
    }
    return 1;
}

/**
 * Takes a list of file and directory names, then outputs to terminal the files
 * and directories in the format specified by any flags inputted by the user.
 */
static void PrintFileNameInfo(char *pathToFile, char *fileName) {
    /* Formatting for l flag = false. */
    if (!flags->fields.l) {
        if (flags->fields.i) {
            PrintSimpleNameWithIno(pathToFile, fileName);
        } else {
            printf("%s\n", fileName);
        }
    } else { /* Formatting for l flag = true. */
        PrintFileDescLine(pathToFile, fileName);
    }
}

/**
 * Prints the file name with the inode number prepended.
 */
static void PrintSimpleNameWithIno(char *dirName, char *fileName) {
    struct stat statBuf;
    char nameBuf[NAME_LEN];

    /* Append directory name to file name. */
    memset(nameBuf, 0, NAME_LEN);
    strncpy(nameBuf, dirName, strlen(dirName));
    if (nameBuf[strlen(nameBuf) - 1] != '/') {
        strncpy(nameBuf + strlen(nameBuf), "/", 1);
    }
    strncpy(nameBuf + strlen(nameBuf), fileName, strlen(fileName));
            
    /* Get file info using stat system call. */
    if (lstat(nameBuf, &statBuf) == STAT_FAIL_CODE) {
        fprintf(stderr, "Stat call failed for %s: ", nameBuf);
        perror("");
        free(flags);
        exit(0);
    }
    printf("%llu %s\n", statBuf.st_ino, fileName);
}

/**
 * Used for printing to the terminal when the -l flag is specified.
 * Gets the relevant file info for ls -l and prints a single line for the
 * specified file or directory name.
 */
static void PrintFileDescLine(char *dirName, char *fileName) {
    struct stat statBuf;
    char nameBuf[NAME_LEN];
    memset(&statBuf, 0, sizeof(struct stat));
    
    /* Append directory name to file name. */
    memset(nameBuf, 0, NAME_LEN);
    strncpy(nameBuf, dirName, strlen(dirName));
    if (nameBuf[strlen(nameBuf) - 1] != '/') {
        strncpy(nameBuf + strlen(nameBuf), "/", 1);
    }
    strncpy(nameBuf + strlen(nameBuf), fileName, strlen(fileName));
    
    /* Get file info using stat system call. */
    //printf("Desc line namebuf: %s\n", nameBuf);
    if (lstat(nameBuf, &statBuf) == STAT_FAIL_CODE) {
        fprintf(stderr, "Stat call failed for %s: ", nameBuf);
        perror("");
        free(flags);
        exit(0);
    }
    
    /* Check if we should print the inode number. */
    memset(inodeBuf, 0, INODE_LEN);
    if (flags->fields.i) {
        snprintf(inodeBuf, INODE_LEN, "%-10llu ", statBuf.st_ino);
    }
    
    /* Check if the file is a directory or symbolic link. */
    char dirChar = '-'; /* char preceding the permission string. */
    memset(linkNameBuf, 0, NAME_LEN);
    if (S_ISDIR(statBuf.st_mode)) {
        dirChar = 'd';
    } else if (((statBuf.st_mode)&(S_IFMT)) == (S_IFLNK)) {
        dirChar = 'l';
        /* Set string denoting which file the link points to. */
        ssize_t len = 0;
        snprintf(linkNameBuf, strlen(" -> ") + 1, " -> ");
        if ((len = readlink(nameBuf,
                            linkNameBuf + strlen(linkNameBuf),
                            sizeof(linkNameBuf) - strlen(linkNameBuf)))
            != -1) {
            //linkNameBuf[len + strlen(" -> ")] = '\0';
        }
    }
    
    /* Parse permission string from st_mode. */
    char *permissionString = (char *)malloc(sizeof(char) * PERMISSION_LEN);
    BuildPermissionString(permissionString, statBuf.st_mode);
    
    /* Parse date string from last modified time. */
    char *dateString = (char *)malloc(sizeof(char) * DATE_LEN);
    BuildDateString(dateString, &(statBuf.st_mtime));
    
    /* Print entire file desc string. */
    printf("%s%c%s %3u %s  %s  %9llu %s %s%s\n",
           inodeBuf,
           dirChar,
           permissionString,
           statBuf.st_nlink,
           getpwuid(statBuf.st_uid)->pw_name,
           getgrgid(statBuf.st_gid)->gr_name,
           statBuf.st_size,
           dateString,
           fileName,
           linkNameBuf);
    
    /* Free memory. */
    free(permissionString);
    free(dateString);
}

/**
 * Builds a permission string to be printed with ls -l.
 * Converts a mode_t variable specifying permissions into a permission string
 * (e.g. -rw-r--r--) and stores it in the char pointer given.
 */
static void BuildPermissionString(char *string, mode_t permissions) {
    memset(string, 0, PERMISSION_LEN);
    string[0] = (permissions & S_IRUSR) ? 'r' : '-';
    string[1] = (permissions & S_IWUSR) ? 'w' : '-';
    string[2] = (permissions & S_IXUSR) ? 'x' : '-';
    string[3] = (permissions & S_IRGRP) ? 'r' : '-';
    string[4] = (permissions & S_IRGRP) ? 'w' : '-';
    string[5] = (permissions & S_IRGRP) ? 'x' : '-';
    string[6] = (permissions & S_IROTH) ? 'r' : '-';
    string[7] = (permissions & S_IROTH) ? 'w' : '-';
    string[8] = (permissions & S_IROTH) ? 'x' : '-';
}

/**
 * Takes a time_t parameter, formats a date string, and stores it in the
 * provided char pointer.
 * The format is: mmm dd yyyy hh:mm
 */
static void BuildDateString(char *string, time_t *time) {
    strftime(string, DATE_LEN, "%b %e %Y %R", localtime(time));
}
