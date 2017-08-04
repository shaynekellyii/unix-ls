/***************************************************************
 * Implementation of unix terminal ls command 				   *
 * Author: Shayne Kelly II                                     *
 * Date: July 15, 2017                                         *
 ***************************************************************/

/***************************************************************
 * Imports                                                     *
 ***************************************************************/
#include <stdint.h>

/***************************************************************
 * Structs                                                     *
 ***************************************************************/

/** 
 * FLAGS struct, for keeping track of which arguments are supplied to ls.
 * Only -i, -l, -R implemented.
 */
typedef union FLAGS {
    struct {
        unsigned int i : 1;
        unsigned int l : 1;
        unsigned int R : 1;
    } fields;
    uint8_t bits;
} FLAGS;