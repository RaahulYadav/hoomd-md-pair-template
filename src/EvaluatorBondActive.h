// Copyright (c) 2009-2025 The Regents of the University of Michigan.
// Part of HOOMD-blue, released under the BSD 3-Clause License.

#ifndef __BOND_EVALUATOR_ACTIVE_H__
#define __BOND_EVALUATOR_ACTIVE_H__

#ifndef __HIPCC__
#include <string>
#endif

#include "hoomd/HOOMDMath.h"

/*! \file EvaluatorBondActive.h
    \brief Defines the bond evaluator class for active forces
*/

// need to declare these class methods with __device__ qualifiers when building in nvcc
// DEVICE is __host__ __device__ when included in nvcc and blank when included into the host
// compiler
#ifdef __HIPCC__
#define DEVICE __device__
#else
#define DEVICE
#endif

namespace hoomd
    {
namespace md
    {
struct active_bond_params
    {
    Scalar F0; // Force Magnitude
    int mode;  // 0 : reciever gets full force, 1 : force gets divided

#ifndef __HIPCC__
    active_bond_params()
        {
        F0 = 0;
        mode = 0;
        }

    active_bond_params(Scalar F0, int mode) : F0(F0), mode(mode) { }

    active_bond_params(pybind11::dict v)
        {
        F0 = v["F0"].cast<Scalar>();
        mode = v["mode"].cast<int>();
        }

    pybind11::dict asDict()
        {
        pybind11::dict v;
        v["F0"] = F0;
        v["mode"] = mode;
        return v;
        }
#endif
    }
#if HOOMD_LONGREAL_SIZE == 32
    __attribute__((aligned(8)));
#else
    __attribute__((aligned(16)));
#endif

//! Class for evaluating the active force along bond
/*! Evaluates the active force along bond in an identical manner to EvaluatorPairLJ for pair
   potentials. See that class for a full motivation and design specifics.

    params.x is the F0 active force magnitude, and
     params.y is the mode 
     which can take two inputs 0 : force on just reciever
     1 : force get divided in half between reciever and actor
*/
class EvaluatorBondActive
    {
    public:
    //! Define the parameter type used by this pair potential evaluator
    typedef active_bond_params param_type;

    //! Constructs the active force along bond evaluator
    /*! \param _rsq Squared distance between the particles
        \param _params Per type pair parameters of this potential
    */
    DEVICE EvaluatorBondActive(Scalar _rsq, const param_type& _params)
        : rsq(_rsq), F0(_params.F0), mode(_params.mode)
        {
        }

    //! Harmonic doesn't use charge
    DEVICE static bool needsCharge()
        {
        return false;
        }

    //! Accept the optional charge values
    /*! \param qa Charge of particle a
        \param qb Charge of particle b
    */
    DEVICE void setCharge(Scalar qa, Scalar qb) { }

    //! Evaluate the force and energy
    /*! \param force_divr Output parameter to write the computed force divided by r.
        \param bond_eng Output parameter to write the computed bond energy

        \return True if they are evaluated or false if the bond
                energy is not defined
    */
    DEVICE bool evalForceAndEnergy(Scalar& force_divr, Scalar& bond_eng)
        {
        Scalar r = sqrt(rsq);
        force_divr = F0/r;

// if the result is not finite, it is likely because of a division by 0, setting force_divr to 0
// will correctly result in a 0 force in this case
#ifdef __HIPCC__
        if (!isfinite(force_divr))
#else
        if (!std::isfinite(force_divr))
#endif
            {
            force_divr = Scalar(0.0);
            }
        bond_eng = Scalar(0.0);

        return true;
        }

    // We'll need to access the mode from the kernel
    DEVICE int getMode() const { return mode; }

#ifndef __HIPCC__
    //! Get the name of this potential
    /*! \returns The potential name.
     */
    static std::string getName()
        {
        return std::string("active_bond");
        }
#endif

    protected:
    Scalar rsq; //!< Stored rsq from the constructor
    Scalar F0;   //!< F0 parameter
    int mode; //!< mode parameter
    };

    } // end namespace md
    } // end namespace hoomd

#endif // __BOND_EVALUATOR_ACTIVE_H__