// Copyright (c) 2009-2025 The Regents of the University of Michigan.
// Part of HOOMD-blue, released under the BSD 3-Clause License.

#ifndef __PAIR_EVALUATOR_FRICTIONLJSHIFTED_H__
#define __PAIR_EVALUATOR_FRICTIONLJSHIFTED_H__

#include "EvaluatorPairFrictionLJShiftedBase.h"

// need to declare these class methods with __device__ qualifiers when building
// in nvcc.  HOSTDEVICE is __host__ __device__ when included in nvcc and blank
// when included into the host compiler
#ifdef __HIPCC__
#define HOSTDEVICE __host__ __device__
#define DEVICE __device__
#else
#define HOSTDEVICE
#define DEVICE
#endif

namespace hoomd
    {
namespace md
    {

class EvaluatorPairFrictionLJShiftedLinear
    : public EvaluatorPairFrictionLJShiftedBase<EvaluatorPairFrictionLJShiftedLinear>
    {
    public:
    using EvaluatorPairFrictionLJShiftedBase<
        EvaluatorPairFrictionLJShiftedLinear>::EvaluatorPairFrictionLJShiftedBase;

    HOSTDEVICE void eval_factors(Scalar& factor_f, Scalar& factor_r, Scalar w, Scalar du)
        {
        Scalar f = gamma;

        factor_f = w * f;
        factor_r = fast::sqrt(factor_f);
        }

#ifndef __HIPCC__
    static std::string getName()
        {
        return "FrictionLJShiftedLinear";
        }
#endif
    };

    } // end namespace md
    } // end namespace hoomd

#undef DEVICE
#undef HOSTDEVICE

#endif // __PAIR_EVALUATOR_FRICTIONLJSHIFTED_H__
