/***************************************************************
 * Implementation of unix terminal ls command 				   *
 * Author: Shayne Kelly II                                     *
 * Date: July 15, 2017                                         *
 ***************************************************************/

/***************************************************************
 * Imports                                                     *
 ***************************************************************/
#include "UnixLs.h"
#include "list.h"
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
#define DATE_LEN        18
#define NAME_LEN        150
#define PERMISSION_LEN  9
#define STAT_FAIL_CODE  -1

/***************************************************************
 * Statics                                                     *
 ***************************************************************/
FLAGS *flags; /* ls options specified in args */
char *dirName; /* The directory to ls, if specified. */
char fileNameBuf[NAME_LEN]; /* Store a directory name with file name appended. */
char linkNameBuf[NAME_LEN]; /* Store the filename that a symbolic link points to. */

/***************************************************************
 * Function Prototypes                                         *
 ***************************************************************/
static uint8_t ParseFlagsFromArgs(char *argString);
static void PrintFileNameInfo(char *dirName, LIST *fileNames);
static void PrintFileDescLine(char *dirName, char *fileName);
static void BuildPermissionString(char *string, mode_t permissions);
static void BuildDateString(char *string, time_t *time);

/***************************************************************
 * Global Functions                                            *
 ***************************************************************/

int main(int argc, char *argv[]) {
    DIR *dir = NULL;
    struct dirent *dp = NULL;
    LIST *fileNames = ListCreate();
    flags = (FLAGS *)malloc(sizeof(FLAGS));
    dirName = ".";
    
    /**
     * Parse all flag arguments from terminal.
     * Only the last arg can specify a directory. 
     */
    for (int i = 1; i < argc - 1; i++) {
        char *currentArg = argv[i];
        if (!ParseFlagsFromArgs(currentArg)) {
            free(flags);
            exit(0);
        }
    }
    
    /* Check if last argument specifies a directory or a flag (if arguments are specified). */
    if (argc > 1) {
        char *currentArg = argv[argc - 1];
        if (currentArg[0] == '-') {
            /* Parse flags from last arg. Directory to search is current directory. */
            if (!ParseFlagsFromArgs(currentArg)) {
                free(flags);
                exit(0);
            }
        } else {
            dirName = argv[argc - 1];
        }
    }
    
    /* Open the directory. (current directory if none specified) */
    if ((dir = opendir(dirName)) == NULL) {
        perror("Failed to open the directory");
        free(flags);
        exit(0);
    }

    /* Loop reading each filename and add it to the list until the end of the directory. */
    do {
        errno = 0;
        if ((dp = readdir(dir)) != NULL) {
            /* Don't add hidden files to the list. */
            if (dp->d_name[0] != '.') {
                ListAppend(fileNames, dp->d_name);
            }
        }
    } while (dp != NULL);
    
    /* Print the filenames (based on the flags specified). */
    PrintFileNameInfo(dirName, fileNames);
    
    /* Close the directory. */
    if (closedir(dir)) {
        printf("Closing the directory failed.\n");
        free(flags);
        exit(0);
    }
    
    free(flags);
    exit(0);
}

/***************************************************************
 * Static Functions                                            *
 ***************************************************************/
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
static void PrintFileNameInfo(char *dirName, LIST *fileNames) {
    ListFirst(fileNames);
    
    /* Formatting for l flag = false. */
    if (!flags->fields.l) {
        for (int i = 0; i < ListCount(fileNames); i++) {
            printf("%s\n",ListCurr(fileNames));
            ListNext(fileNames);
        }
    } else { /* Formatting for l flag = true. */
        for (int i = 0; i < ListCount(fileNames); i++) {
            PrintFileDescLine(dirName, ListCurr(fileNames));
            ListNext(fileNames);
        }
    }
}

/**
 * Used for printing to the terminal when the -l flag is specified.
 * Gets the relevant file info for ls -l and prints a single line for the
 * specified file or directory name.
 */
static void PrintFileDescLine(char *dirName, char *fileName) {
    struct stat statBuf;
    
    /* Append directory name to file name. */
    memset(fileNameBuf, 0, NAME_LEN);
    strncpy(fileNameBuf, dirName, strlen(dirName));
    strncpy(fileNameBuf + strlen(dirName), "/", 1);
    strncpy(fileNameBuf + strlen(dirName) + 1, fileName, strlen(fileName));
    
    /* Get file info using stat system call. */
    if (lstat(fileNameBuf, &statBuf) == STAT_FAIL_CODE) {
        fprintf(stderr, "Stat call failed for %s: ", fileNameBuf);
        perror("");
        free(flags);
        exit(0);
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
        if ((len = readlink(fileNameBuf,
                            linkNameBuf + strlen(" -> "),
                            sizeof(linkNameBuf) - 3))
            != -1) {
            linkNameBuf[len + strlen(" -> ") + 1] = '\0';
        }
    }
    
    /* Parse permission string from st_mode. */
    char *permissionString = (char *)malloc(sizeof(char) * PERMISSION_LEN);
    BuildPermissionString(permissionString, statBuf.st_mode);
    
    /* Parse date string from last modified time. */
    char *dateString = (char *)malloc(sizeof(char) * DATE_LEN);
    BuildDateString(dateString, &(statBuf.st_mtime));
    
    /* Print entire file desc string. */
    printf("%c%s %u %s  %s  %5llu %s %s%s\n",
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
