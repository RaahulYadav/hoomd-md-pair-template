# Copyright (c) 2009-2025 The Regents of the University of Michigan.
# Part of HOOMD-blue, released under the BSD 3-Clause License.

"""Example pair potential."""

# Import the C++ module.
from hoomd.data.parameterdicts import TypeParameterDict, ParameterDict
from hoomd.data.typeparam import TypeParameter

# Import the hoomd Python package and other necessary components.
from hoomd.md import pair
from hoomd.md.pair.friction import FrictionalPair
from hoomd.active import _active


class ExamplePair(pair.Pair):
    """Example pair potential."""

    # set static class data
    _ext_module = _active
    _cpp_class_name = 'PotentialPairExample'
    _accepted_modes = ('none', 'shift', 'xplor')

    def __init__(self, nlist, default_r_cut=None, default_r_on=0.0, mode='none'):
        super().__init__(nlist, default_r_cut, default_r_on, mode)
        params = TypeParameter(
            'params',
            'particle_types',
            TypeParameterDict(k=float, sigma=float, len_keys=2),
        )
        self._add_typeparam(params)



class ShiftedLJ(pair.Pair):
    r"""Lennard-Jones pair force.

    Args:
        nlist (hoomd.md.nlist.NeighborList): Neighbor list.
        default_r_cut (float): Default cutoff radius :math:`[\mathrm{length}]`.
        default_r_on (float): Default turn-on radius :math:`[\mathrm{length}]`.
        mode (str): Energy shifting/smoothing mode.
        tail_correction (bool): Whether to apply the isotropic integrated long
            range tail correction.

    `LJ` computes the Lennard-Jones pair force on every particle in the
    simulation state.

    .. math::
        U(r) = 4 \varepsilon \left[ \left(
        \frac{\sigma}{r} \right)^{12} - \left( \frac{\sigma}{r}
        \right)^{6} \right]

    Example::

        nl = nlist.Cell()
        lj = pair.LJ(nl, default_r_cut=3.0)
        lj.params[("A", "A")] = {"sigma": 1.0, "epsilon": 1.0}
        lj.r_cut[("A", "B")] = 3.0

    {inherited}

    ----------

    **Members defined in** `LJ`:

    .. py:attribute:: params

        The LJ potential parameters. The dictionary has the following keys:

        * ``epsilon`` (`float`, **required**) -
          energy parameter :math:`\varepsilon` :math:`[\mathrm{energy}]`
        * ``sigma`` (`float`, **required**) -
          particle size :math:`\sigma` :math:`[\mathrm{length}]`

        Type: `TypeParameter` [`tuple` [``particle_type``, ``particle_type``],
        `dict`]

    .. py:attribute:: tail_correction

        Whether to apply the isotropic integrated long range tail correction.

        Type: `bool`
    """

    _cpp_class_name = "ShiftedLJ"
    _accepted_modes = ('none', 'shift', 'xplor')
    _ext_module = _active

    def __init__(
        self,
        nlist,
        default_r_cut=None,
        default_r_on=0.0,
        mode="none",
        tail_correction=False,
    ):
        super().__init__(nlist, default_r_cut, default_r_on, mode)
        params = TypeParameter(
            "params",
            "particle_types",
            TypeParameterDict(epsilon=float, sigma=float, r_shift=float, len_keys=2),
        )
        self._add_typeparam(params)
        # self._param_dict.update(ParameterDict(tail_correction=bool(tail_correction)))
class FrictionLJShiftedLinear(FrictionalPair):
    r"""Linear frictional model pair force with the Shifted LJ conservative force.

    Args:
        nlist (hoomd.md.nlist.NeighborList): Neighbor list
        default_r_cut (float): Default cutoff radius :math:`[\mathrm{length}]`.
        default_r_on (float): Default turn-on radius :math:`[\mathrm{length}]`.
        mode (str): Energy shifting/smoothing mode.

    `FrictionLJShiftedLinear` computes the frictional interaction
    between pairs of particles with a linear friction model and a shifted
    Lennard-Jones conservative force.

    The conservative potential is:
    .. math::
        V(r) = 4 \varepsilon \left[ \left( \frac{\sigma}{r - r_0} \right)^{12} - \left( \frac{\sigma}{r - r_0} \right)^{6} \right]

    for :math:`r > r_0`.

    The friction model is Linear:
    .. math::
        f_\mathrm{l}(u,r) = \gamma_\mathrm{f}u

    """

    _cpp_class_name = "FrictionLJShiftedLinear"
    _ext_module = _active

    def __init__(self, nlist, default_r_cut=None, default_r_on=0.0, mode="none"):
        super().__init__(nlist, default_r_cut, mode)
        params = TypeParameter(
            "params",
            "particle_types",
            TypeParameterDict(
                epsilon=float, 
                sigma=float, 
                gamma_f=float, 
                r_shift=float, 
                kT=float, 
                len_keys=2
            ),
        )
        self._add_typeparam(params)
