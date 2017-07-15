/***************************************************************
 * Imports                                                     *
 ***************************************************************/
#include <stdint.h>

/***************************************************************
 * Structs                                                     *
 ***************************************************************/
typedef union FLAGS {
    struct {
        unsigned int i : 1;
        unsigned int L : 1;
        unsigned int r : 1;
    } fields;
    uint8_t bits;
} FLAGS;
