// Copyright (c) 2009-2025 The Regents of the University of Michigan.
// Part of HOOMD-blue, released under the BSD 3-Clause License.

#include "EvaluatorBondActive.h"
#include "ActiveBondGPU.cuh"

namespace hoomd
    {
    namespace md
    {
    namespace kernel
    {
template __attribute__((visibility("default"))) hipError_t
gpu_compute_active_bond_forces<EvaluatorBondActive, 2>(
                const bond_args_t<2>& bond_args,
                const typename EvaluatorBondActive::param_type* d_params, // <<< COMMA WAS MISSING HERE
                unsigned int* d_flags);
    } // end namespace kernel
    } // end namespace md
    } // end namespace hoomd