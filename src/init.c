/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2021. All rights reserved.
/// @author mn-sc
///
/// @brief SCR* infra init routines: cpu, memory, etc

#include "platform_config.h"
#include "scbl.h"

#define MIP_MMFIP 0x00008000 /* memory fault */

#ifdef PLF_MEM_MAP
const plf_mem_region_info mem_regions[] = {
    PLF_MEM_MAP,
    {0, 0, 0, 0}
};
#endif // PLF_MEM_MAP
