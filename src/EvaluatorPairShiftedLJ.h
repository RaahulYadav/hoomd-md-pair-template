// Copyright (c) 2009-2025 The Regents of the University of Michigan.
// Part of HOOMD-blue, released under the BSD 3-Clause License.

// ==========================================================
// CHANGE 1: Update header guards to be unique
// ==========================================================
#ifndef __PAIR_EVALUATOR_SHIFTEDLJ_H__
#define __PAIR_EVALUATOR_SHIFTEDLJ_H__

#ifndef __HIPCC__
#include <string>
#endif

#include "hoomd/HOOMDMath.h"

/*! \file EvaluatorPairShiftedLJ.h
    \brief Defines the pair evaluator class for a shifted LJ potential
*/

#ifdef __HIPCC__
#define DEVICE __device__
#define HOSTDEVICE __host__ __device__
#else
#define DEVICE
#define HOSTDEVICE
#endif

namespace hoomd
    {
namespace md
    {
// ==========================================================
// CHANGE 2: Update class name to be unique
// ==========================================================
//! Class for evaluating the shifted LJ pair potential
class EvaluatorPairShiftedLJ
    {
    public:
    //! Define the parameter type used by this pair potential evaluator
    struct param_type
        {
        // ==========================================================
        // CHANGE 3: Add r_shift to the parameter struct
        // ==========================================================
        Scalar sigma_6;
        Scalar epsilon_x_4;
        Scalar r_shift; // The distance shift parameter

        DEVICE void load_shared(char*& ptr, unsigned int& available_bytes) { }
        HOSTDEVICE void allocate_shared(char*& ptr, unsigned int& available_bytes) const { }

#ifdef ENABLE_HIP
        void set_memory_hint() const { }
#endif

#ifndef __HIPCC__
        // ==========================================================
        // CHANGE 4: Update constructors to handle r_shift
        // ==========================================================
        param_type() : sigma_6(0), epsilon_x_4(0), r_shift(0) { }

        param_type(pybind11::dict v, bool managed = false)
            {
            auto sigma(v["sigma"].cast<Scalar>());
            auto epsilon(v["epsilon"].cast<Scalar>());
            r_shift = v["r_shift"].cast<Scalar>(); // Read r_shift from Python dict

            sigma_6 = sigma * sigma * sigma * sigma * sigma * sigma;
            epsilon_x_4 = Scalar(4.0) * epsilon;
            }

        // Constructor for unit testing
        param_type(Scalar sigma, Scalar epsilon, Scalar shift, bool managed = false)
            {
            sigma_6 = sigma * sigma * sigma * sigma * sigma * sigma;
            epsilon_x_4 = Scalar(4.0) * epsilon;
            r_shift = shift;
            }

        // ==========================================================
        // CHANGE 5: Update asDict to return r_shift to Python
        // ==========================================================
        pybind11::dict asDict()
            {
            pybind11::dict v;
            v["sigma"] = pow(sigma_6, 1. / 6.);
            v["epsilon"] = epsilon_x_4 / 4.0;
            v["r_shift"] = r_shift;
            return v;
            }
#endif
        }
#if HOOMD_LONGREAL_SIZE == 32
        __attribute__((aligned(8)));
#else
        __attribute__((aligned(16)));
#endif

    //! Constructs the pair potential evaluator
    DEVICE EvaluatorPairShiftedLJ(Scalar _rsq, Scalar _rcutsq, const param_type& _params)
        // ==========================================================
        // CHANGE 6: Store r_shift in the evaluator's member variables
        // ==========================================================
        : rsq(_rsq), rcutsq(_rcutsq),
          lj1(_params.epsilon_x_4 * _params.sigma_6 * _params.sigma_6),
          lj2(_params.epsilon_x_4 * _params.sigma_6),
          r_shift(_params.r_shift)
        {
        }

    //! LJ doesn't use charge
    DEVICE static bool needsCharge() { return false; }
    DEVICE void setCharge(Scalar qi, Scalar qj) { }

    //! Evaluate the force and energy
    //! Evaluate the force and energy
    DEVICE bool evalForceAndEnergy(Scalar& force_divr, Scalar& pair_eng, bool energy_shift)
    {
        // ======================================================================
        // THE DEFINITIVE FIX: Implement V(r - r_shift) and correct the cutoff check.
        // ======================================================================

        Scalar r = fast::sqrt(rsq);
        // Calculate the shifted distance. This moves the potential to the RIGHT.
        Scalar r_shifted = r - r_shift;

        // The interaction is active only if the SHIFTED distance is within the cutoff range.
        // We also must check that r_shifted > 0 to avoid the potential at negative distances.
        // rcutsq here is the cutoff for the unshifted LJ potential (e.g., 2**(1/6)*sigma)
        // when r_shift is 0, but for C-H it's (r_cut_base + r_shift)^2.
        // The check 'rsq < rcutsq' from the calling function is correct for the overall
        // neighbor list cutoff. We now need a local check for the potential's domain.
        
        // Let's define the base cutoff for the LJ part of the potential.
        // Let's assume the rcut passed to the evaluator IS the final cutoff.
        // The interaction happens if r < rcut.
        if (rsq < rcutsq && lj1 != 0)
        {
            // Only calculate if the shifted distance is positive (i.e., r > r_shift)
            if (r_shifted > 0)
            {
                Scalar r_shifted_sq = r_shifted * r_shifted;
                Scalar r2inv = Scalar(1.0) / r_shifted_sq;
                Scalar r6inv = r2inv * r2inv * r2inv;
                
                // Calculate force_divr_shifted = -(1/r_shifted) * dV/dr_shifted
                Scalar force_divr_shifted = r2inv * r6inv * (Scalar(12.0) * lj1 * r6inv - Scalar(6.0) * lj2);
                
                // Chain rule for V(r - r_shift): dV/dr = dV/dr_shifted
                // So, force_divr = -(1/r)*dV/dr = (r_shifted/r) * force_divr_shifted
                force_divr = (r_shifted / r) * force_divr_shifted;

                pair_eng = r6inv * (lj1 * r6inv - lj2);

                if (energy_shift)
                {
                    // Energy shift should be V(rcut).
                    // For the shifted potential, this is V_LJ(rcut - r_shift)
                    Scalar rcut = fast::sqrt(rcutsq);
                    Scalar rcut_shifted = rcut - r_shift;
                    if (rcut_shifted > 0)
                    {
                        Scalar rcut_shifted_sq = rcut_shifted * rcut_shifted;
                        Scalar rcut2inv = Scalar(1.0) / rcut_shifted_sq;
                        Scalar rcut6inv = rcut2inv * rcut2inv * rcut2inv;
                        pair_eng -= rcut6inv * (lj1 * rcut6inv - lj2);
                    }
                }
                return true;
            }
        }
        
        // If we are outside the cutoff OR r <= r_shift, there is no force.
        return false;
    }

    // NOTE: The long-range correction integrals are non-trivial for this potential
    // and are left as an exercise. For now, they return 0.
    DEVICE Scalar evalPressureLRCIntegral() { return Scalar(0.0); }
    DEVICE Scalar evalEnergyLRCIntegral() { return Scalar(0.0); }

#ifndef __HIPCC__
    // ==========================================================
    // CHANGE 8: Update the potential's name
    // ==========================================================
    static std::string getName()
        {
        return std::string("shifted_lj");
        }

    std::string getShapeSpec() const
        {
        throw std::runtime_error("Shape definition not supported for this pair potential.");
        }
#endif

    protected:
    Scalar rsq;
    Scalar rcutsq;
    Scalar lj1;
    Scalar lj2;
    // ==========================================================
    // CHANGE 9: Add r_shift as a member variable
    // ==========================================================
    Scalar r_shift;
    };

    } // end namespace md
    } // end namespace hoomd

#endif // __PAIR_EVALUATOR_SHIFTEDLJ_H__