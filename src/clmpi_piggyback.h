#include <mpi.h>
//#include "clmpi_status.h"

#ifndef _PNMPI_MOD_PIGGYBACK
#define _PNMPI_MOD_PIGGYBACK

#define PNMPI_MODULE_PB "piggyback"


/*..........................................................*/
/* Request additional memory in the status object
   This must be called from within MPI_Init (after
   calling PMPI_Init)

   IN:  size = number of bytes requested
               (if 0, just storage of standard parameters
   OUT: >=0: offset of new storage relative to request pointer
         <0: error message 
*/
extern int piggyback_offset;

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*PNMPIMOD_Piggyback_t)(int,char*);
int PNMPIMOD_Piggyback(int, char*);
typedef int (*PNMPIMOD_Piggyback_Size_t)(int);
int PNMPIMOD_Piggyback_Size(int);

#ifdef __cplusplus
}
#endif

#endif /* _PNMPI_MOD_STATUS */
