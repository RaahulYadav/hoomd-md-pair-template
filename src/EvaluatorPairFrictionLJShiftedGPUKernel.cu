#include "EvaluatorPairFrictionLJShifted.h"
#include "hoomd/md/FrictionPairGPU.cuh"

namespace hoomd
    {
namespace md
    {
namespace kernel
    {
template hipError_t gpu_compute_pair_friction_forces<EvaluatorPairFrictionLJShiftedLinear>(
    const a_pair_args_t& pair_args,
    const EvaluatorPairFrictionLJShiftedLinear::param_type* d_params);
    }
    } // namespace md
    } // namespace hoomd
