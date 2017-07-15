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
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

/***************************************************************
 * Defines and Constants                                       *
 ***************************************************************/
#define ALL_FLAGS_SET   7
#define CURR_DIRECTORY  "."

/***************************************************************
 * Statics                                                     *
 ***************************************************************/
FLAGS *flags; /* ls options specified in args */
char *dirName; /* The directory to ls, if specified. */

/***************************************************************
 * Function Prototypes                                         *
 ***************************************************************/
static uint8_t ParseFlagsFromArgs(char *argString);
static void PrintFileNameInfo(LIST *fileNames);

/***************************************************************
 * Global Functions                                            *
 ***************************************************************/

int main(int argc, char *argv[]) {
    DIR *dir = NULL;
    struct dirent *dirent = NULL;
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
        if ((dirent = readdir(dir)) != NULL) {
            /* Don't add hidden files to the list. */
            if (dirent->d_name[0] != '.') {
                ListAppend(fileNames, dirent->d_name);
            }
        }
    } while (dirent != NULL);
    
    /* Print the filenames (based on the flags specified). */
    PrintFileNameInfo(fileNames);
    
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

static void PrintFileNameInfo(LIST *fileNames) {
    struct stat *statBuf = NULL;
    ListFirst(fileNames);
    
    /* Formatting for l flag = false. */
    if (!flags->fields.l) {
        for (int i = 0; i < ListCount(fileNames); i++) {
            printf("%-25s ", ListCurr(fileNames));
            if (i != 0 && !((i + 1) % 4)) {
                printf("\n");
            }
            ListNext(fileNames);
        }
        /* Add a line break if needed. */
        if (ListCount(fileNames) % 4) {
            printf("\n");
        }
    }
}
