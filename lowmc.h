#ifndef LOWMC_H
#define LOWMC_H

#include <m4ri/m4ri.h>
#include "lowmc_pars.h"

/**
 * Implements LowMC encryption 
 *
 * \param  lowmc the lowmc parameters
 * \param  p     the plaintext
 * \return       the ciphertext
 */
mzd_t *lowmc_call(lowmc_t *lowmc, lowmc_key_t *lowmc_key, mzd_t *p);

#endif
