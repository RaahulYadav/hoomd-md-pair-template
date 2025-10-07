# Copyright (c) 2009-2025 The Regents of the University of Michigan.
# Part of HOOMD-blue, released under the BSD 3-Clause License.

"""Active Bond."""

# Import the C++ module.
from hoomd.data.parameterdicts import TypeParameterDict
from hoomd.data.typeparam import TypeParameter

# Import the hoomd Python package and other necessary components.
from hoomd.md import bond
from hoomd.active import _active


class ActiveBond(bond.Bond):
    """Active Bond."""

    # set static class data
    _ext_module = _active
    _cpp_class_name = 'ActiveBond'
    _accepted_modes = ('none', 'shift', 'xplor')

    def __init__(self):
        super().__init__()
        params = TypeParameter(
            'params',
            'bond_types',
            TypeParameterDict(F0=float, mode=int, len_keys=1),
        )
        self._add_typeparam(params)


