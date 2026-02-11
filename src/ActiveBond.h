// Copyright (c) 2009-2025 The Regents of the University of Michigan.
// Part of HOOMD-blue, released under the BSD 3-Clause License.

#include "hoomd/md/MeshForceCompute.h"
#include "hoomd/GPUArray.h"
#include <memory>

#include <vector>

/*! \file ActiveBond.h
    \brief Declares ActiveBond
*/

#ifdef __HIPCC__
#error This header cannot be compiled by nvcc
#endif

#include <pybind11/pybind11.h>

#ifndef __ACTIVEBOND_H__
#define __ACTIVEBOND_H__

namespace hoomd
    {
namespace md
    {
/*! Bond potential with evaluator support

    \ingroup computes
*/
template<class evaluator, class Bonds> class ActiveBond : public MeshForceCompute
    {
    public:
    //! Param type from evaluator
    typedef typename evaluator::param_type param_type;

    //! Constructs the compute
    ActiveBond(std::shared_ptr<SystemDefinition> sysdef);

    //! Constructs the compute with external Bond data
    ActiveBond(std::shared_ptr<SystemDefinition> sysdef,
                  std::shared_ptr<MeshDefinition> meshdef);

    //! Destructor
    virtual ~ActiveBond();

    /// Set the parameters
    virtual void setParams(unsigned int type, const param_type& param);
    virtual void setParamsPython(std::string type, pybind11::dict param);

    /// Get the parameters
    pybind11::dict getParams(std::string type);

    /// Validate bond type
    virtual void validateType(unsigned int type, std::string action);

#ifdef ENABLE_MPI
    //! Get ghost particle fields requested by this pair potential
    virtual CommFlags getRequestedCommFlags(uint64_t timestep);
#endif

    protected:
    GPUArray<param_type> m_params;      //!< Bond parameters per type
    std::shared_ptr<Bonds> m_bond_data; //!< Bond data to use in computing bonds

    //! Actually compute the forces
    virtual void computeForces(uint64_t timestep);
    };

template<class evaluator, class Bonds>
ActiveBond<evaluator, Bonds>::ActiveBond(std::shared_ptr<SystemDefinition> sysdef)
    : MeshForceCompute(sysdef, NULL)
    {
    m_exec_conf->msg->notice(5) << "Constructing ActiveBond<" << evaluator::getName() << ">"
                                << std::endl;
    assert(m_pdata);

    // access the bond data for later use
    m_bond_data = m_sysdef->getBondData();

    // allocate the parameters
    GPUArray<param_type> params(m_bond_data->getNTypes(), m_exec_conf);
    m_params.swap(params);
    }

template<class evaluator, class Bonds>
ActiveBond<evaluator, Bonds>::ActiveBond(std::shared_ptr<SystemDefinition> sysdef,
                                               std::shared_ptr<MeshDefinition> meshdef)
    : MeshForceCompute(sysdef, meshdef)
    {
    m_exec_conf->msg->notice(5) << "Constructing PotentialMeshBond<" << evaluator::getName() << ">"
                                << std::endl;
    assert(m_pdata);

    // access the bond data for later use
    m_bond_data = meshdef->getMeshBondData();

    // allocate the parameters
    GPUArray<param_type> params(m_bond_data->getNTypes(), m_exec_conf);
    m_params.swap(params);
    }

template<class evaluator, class Bonds> ActiveBond<evaluator, Bonds>::~ActiveBond()
    {
    m_exec_conf->msg->notice(5) << "Destroying ActiveBond<" << evaluator::getName() << ">"
                                << std::endl;
    }

/*! \param type Type of the bond to set parameters for
    \param param Parameter to set

    Sets the parameters for the potential of a particular bond type
*/
template<class evaluator, class Bonds>
void ActiveBond<evaluator, Bonds>::validateType(unsigned int type, std::string action)
    {
    // make sure the type is valid
    if (type >= m_bond_data->getNTypes())
        {
        std::string err = "Invalid bond type specified.";
        err += "Error " + action + " in ActiveBond";
        throw std::runtime_error(err);
        }
    }

template<class evaluator, class Bonds>
void ActiveBond<evaluator, Bonds>::setParams(unsigned int type, const param_type& param)
    {
    // make sure the type is valid
    validateType(type, "setting params");
    ArrayHandle<param_type> h_params(m_params, access_location::host, access_mode::readwrite);
    h_params.data[type] = param;
    }

/*! \param types Type of the bond to set parameters for using string
    \param param Parameter to set

    Sets the parameters for the potential of a particular bond type
*/
template<class evaluator, class Bonds>
void ActiveBond<evaluator, Bonds>::setParamsPython(std::string type, pybind11::dict param)
    {
    auto itype = m_bond_data->getTypeByName(type);
    auto struct_param = param_type(param);
    setParams(itype, struct_param);
    }

/*! \param types Type of the bond to set parameters for using string
    \param param Parameter to set

    Sets the parameters for the potential of a particular bond type
*/
template<class evaluator, class Bonds>
pybind11::dict ActiveBond<evaluator, Bonds>::getParams(std::string type)
    {
    auto itype = m_bond_data->getTypeByName(type);
    validateType(itype, "getting params");
    ArrayHandle<param_type> h_params(m_params, access_location::host, access_mode::read);
    return h_params.data[itype].asDict();
    }

/*! Actually perform the force computation
    \param timestep Current time step
 */
template<class evaluator, class Bonds>
void ActiveBond<evaluator, Bonds>::computeForces(uint64_t timestep)
    {
    assert(m_pdata);

    // access the particle data arrays
    ArrayHandle<Scalar4> h_pos(m_pdata->getPositions(), access_location::host, access_mode::read);
    ArrayHandle<unsigned int> h_rtag(m_pdata->getRTags(), access_location::host, access_mode::read);
    ArrayHandle<Scalar> h_charge(m_pdata->getCharges(), access_location::host, access_mode::read);

    ArrayHandle<Scalar4> h_force(m_force, access_location::host, access_mode::readwrite);
    ArrayHandle<Scalar> h_virial(m_virial, access_location::host, access_mode::readwrite);

    // access the parameters
    ArrayHandle<param_type> h_params(m_params, access_location::host, access_mode::read);

    // there are enough other checks on the input data: but it doesn't hurt to be safe
    assert(h_force.data);
    assert(h_virial.data);
    assert(h_pos.data);
    assert(h_charge.data);

    // Zero data for force calculation
    m_force.zeroFill();
    m_virial.zeroFill();

    // we are using the minimum image of the global box here
    // to ensure that ghosts are always correctly wrapped (even if a bond exceeds half the domain
    // length)
    const BoxDim box = m_pdata->getGlobalBox();

    PDataFlags flags = this->m_pdata->getFlags();
    bool compute_virial = flags[pdata_flag::pressure_tensor];

    Scalar bond_virial[6];
    for (unsigned int i = 0; i < 6; i++)
        bond_virial[i] = Scalar(0.0);

    ArrayHandle<typename Bonds::members_t> h_bonds(m_bond_data->getMembersArray(),
                                                   access_location::host,
                                                   access_mode::read);
    ArrayHandle<typeval_t> h_typeval(m_bond_data->getTypeValArray(),
                                     access_location::host,
                                     access_mode::read);

    unsigned int max_local = m_pdata->getN() + m_pdata->getNGhosts();

    // for each of the bonds
    const unsigned int size = (unsigned int)m_bond_data->getN();

// NEW LOGIC (CORRECT NON-NEWTONIAN FORCE USING TAGS)
    for (unsigned int i = 0; i < size; i++)
    {
        const typename Bonds::members_t& bond = h_bonds.data[i];
        unsigned int bond_type = h_typeval.data[i].type;

        // Use permanent tags to determine a consistent direction
        unsigned int tag_a = bond.tag[0];
        unsigned int tag_b = bond.tag[1];

        // Get the current memory indices for these tags
        unsigned int idx_a = h_rtag.data[tag_a];
        unsigned int idx_b = h_rtag.data[tag_b];

        // Skip if both particles are ghosts
        if (idx_a >= m_pdata->getN() && idx_b >= m_pdata->getN())
            continue;

        // Determine actor/recipient INDICES based on which particle has the smaller TAG
        unsigned int actor_idx     = (tag_a < tag_b) ? idx_a : idx_b;
        unsigned int recipient_idx = (tag_a < tag_b) ? idx_b : idx_a;

        // Get positions based on the actor/recipient indices
        Scalar3 pos_actor = make_scalar3(h_pos.data[actor_idx].x, h_pos.data[actor_idx].y, h_pos.data[actor_idx].z);
        Scalar3 pos_recipient = make_scalar3(h_pos.data[recipient_idx].x, h_pos.data[recipient_idx].y, h_pos.data[recipient_idx].z);

        // Calculate the vector from actor to recipient
        Scalar3 dx = pos_recipient - pos_actor;
        dx = box.minImage(dx);
        Scalar rsq = dot(dx, dx);

        Scalar force_divr = Scalar(0.0);
        Scalar bond_eng = Scalar(0.0);
        evaluator eval(rsq, h_params.data[bond_type]);

        if (eval.evalForceAndEnergy(force_divr, bond_eng))
            {
            int mode = eval.getMode();
            if (mode == 0) // Unidirectional force on recipient
                {
                if (recipient_idx < m_pdata->getN()) // Only apply force if recipient is a local particle
                    {
                    h_force.data[recipient_idx].x += dx.x * force_divr;
                    h_force.data[recipient_idx].y += dx.y * force_divr;
                    h_force.data[recipient_idx].z += dx.z * force_divr;
                    }
                }
            else if (mode == 1) // Split force
                {
                Scalar force_divr_half = force_divr * Scalar(0.5);
                if (actor_idx < m_pdata->getN())
                    {
                    h_force.data[actor_idx].x += dx.x * force_divr_half;
                    h_force.data[actor_idx].y += dx.y * force_divr_half;
                    h_force.data[actor_idx].z += dx.z * force_divr_half;
                    }
                if (recipient_idx < m_pdata->getN())
                    {
                    h_force.data[recipient_idx].x += dx.x * force_divr_half;
                    h_force.data[recipient_idx].y += dx.y * force_divr_half;
                    h_force.data[recipient_idx].z += dx.z * force_divr_half;
                    }
                }
            }
        }
    }

#ifdef ENABLE_MPI
/*! \param timestep Current time step
 */
template<class evaluator, class Bonds>
CommFlags ActiveBond<evaluator, Bonds>::getRequestedCommFlags(uint64_t timestep)
    {
    CommFlags flags = CommFlags(0);

    flags[comm_flag::tag] = 1;

    if (evaluator::needsCharge())
        flags[comm_flag::charge] = 1;

    flags |= MeshForceCompute::getRequestedCommFlags(timestep);

    return flags;
    }
#endif

namespace detail
    {
//! Exports the ActiveBond class to python
/*! \param name Name of the class in the exported python module
    \tparam T Evaluator type to export.
*/
template<class T> void export_ActiveBond(pybind11::module& m, const std::string& name)
    {
    pybind11::class_<ActiveBond<T, BondData>,
                     MeshForceCompute,
                     std::shared_ptr<ActiveBond<T, BondData>>>(m, name.c_str())
        .def(pybind11::init<std::shared_ptr<SystemDefinition>>())
        .def("setParams", &ActiveBond<T, BondData>::setParamsPython)
        .def("getParams", &ActiveBond<T, BondData>::getParams);
    }

//! Exports the PotentialMeshBond class to python
/*! \param name Name of the class in the exported python module
    \tparam T Evaluator type to export.
*/
template<class T> void export_PotentialMeshBond(pybind11::module& m, const std::string& name)
    {
    pybind11::class_<ActiveBond<T, MeshBondData>,
                     MeshForceCompute,
                     std::shared_ptr<ActiveBond<T, MeshBondData>>>(m, name.c_str())
        .def(pybind11::init<std::shared_ptr<SystemDefinition>, std::shared_ptr<MeshDefinition>>())
        .def("setParams", &ActiveBond<T, MeshBondData>::setParamsPython)
        .def("getParams", &ActiveBond<T, MeshBondData>::getParams);
    }

    } // end namespace detail
    } // end namespace md
    } // end namespace hoomd

#endif