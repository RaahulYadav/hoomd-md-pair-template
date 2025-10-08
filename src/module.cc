// // Copyright (c) 2009-2025 The Regents of the University of Michigan.
// // Part of HOOMD-blue, released under the BSD 3-Clause License.

// // TODO: Include the header files of classes that will be exported to Python.
// #include "EvaluatorBondActive.h"

// #include "ActiveBond.h"
// #include <pybind11/pybind11.h>

// #ifdef ENABLE_HIP
// #include "ActiveBondGPU.h"
// #endif

// namespace hoomd
//     {
// namespace md
//     {

// // TODO: Set the name of the python module to match ${COMPONENT_NAME} (set in
// // CMakeLists.txt), prefixed with an underscore.
// PYBIND11_MODULE(_active, m)
//     {
//     // TODO: Call export_Class(m) for each C++ class to be exported to Python.

//     detail::export_PotentialPair<EvaluatorPairExample>(m, "PotentialPairExample");
//     detail::export_ActiveBond<EvaluatorBondActive>(m, "ActiveBond");
// #ifdef ENABLE_HIP
//     // TODO: Call export_ClassGPU(m) for each GPU enabled C++ class to be exported
//     // to Python.
//     detail::export_PotentialPairGPU<EvaluatorPairExample>(m, "PotentialPairExampleGPU");
//         detail::export_ActiveBondGPU<EvaluatorBondActive>(m, "ActiveBondGPU");
// #endif
//     }

//     } // end namespace md
//     } // end namespace hoomd



// ... (includes)
#include "EvaluatorBondActive.h"
#include "ActiveBond.h"
#include "EvaluatorPairShiftedLJ.h"
#include "hoomd/md/PotentialPair.h"
#include <pybind11/pybind11.h>

#ifdef ENABLE_HIP
#include "ActiveBondGPU.h"
#include "hoomd/md/PotentialPairGPU.h"
#endif

namespace hoomd
    {
    namespace md
    {

// Set the name of the python module to match ${COMPONENT_NAME}
PYBIND11_MODULE(_active, m) // <<< Changed from _template to _active
    {
    // REMOVED the old export_PotentialPair lines
    detail::export_ActiveBond<EvaluatorBondActive>(m, "ActiveBond");
    detail::export_PotentialPair<EvaluatorPairShiftedLJ>(m, "ShiftedLJ");
#ifdef ENABLE_HIP
    // REMOVED the old export_PotentialPairGPU lines
    detail::export_ActiveBondGPU<EvaluatorBondActive>(m, "ActiveBondGPU");
    detail::export_PotentialPairGPU<EvaluatorPairShiftedLJ>(m, "ShiftedLJGPU");

#endif
    }

    } // end namespace md
    } // end namespace hoomd
