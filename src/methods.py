# Copyright (c) 2009-2026 The Regents of the University of Michigan.
# Part of HOOMD-blue, released under the BSD 3-Clause License.

"""MD integration methods.

.. code-block:: python

    simulation = hoomd.util.make_example_simulation()
    simulation.operations.integrator = hoomd.md.Integrator(dt=0.001)
    logger = hoomd.logging.Logger()

    # Rename pytest's tmp_path fixture for clarity in the documentation.
    path = tmp_path
"""

from hoomd.md import _md
import hoomd
from hoomd.operation import AutotunedObject
from hoomd.data.parameterdicts import ParameterDict, TypeParameterDict
from hoomd.data.typeparam import TypeParameter
from hoomd.data.typeconverter import OnlyTypes, OnlyIf, to_type_converter
from hoomd.filter import ParticleFilter
from hoomd.variant import Variant
from collections.abc import Sequence
from hoomd.md.methods import Method
import inspect
from hoomd.active import _active


class BrownianModified(Method):
    r"""Brownian dynamics.

    Args:
        filter (hoomd.filter.filter_like): Subset of particles to
            apply this method to.

        kT (hoomd.variant.variant_like): Temperature of the simulation
            :math:`[\mathrm{energy}]`.

        default_gamma (float): Default drag coefficient for all particle types
            :math:`[\mathrm{mass} \cdot \mathrm{time}^{-1}]`.

        default_gamma_r ([`float`, `float`, `float`]): Default rotational drag
            coefficient tensor for all particles :math:`[\mathrm{time}^{-1}]`.

    `Brownian` integrates particles forward in time according to the overdamped
    Langevin equations of motion, sometimes called Brownian dynamics or the
    diffusive limit.

    The translational degrees of freedom follow:

    .. math::

        \begin{split}
        \frac{d\vec{r}}{dt} &= \frac{\vec{F}_\mathrm{C}
        + \vec{F}_\mathrm{R}}{\gamma}, \\
        \langle \vec{F}_\mathrm{R} \rangle &= 0, \\
        \langle |\vec{F}_\mathrm{R}|^2 \rangle &= 2 d k T \gamma / \delta t, \\
        \langle \vec{v}(t) \rangle &= 0, \\
        \langle |\vec{v}(t)|^2 \rangle &= d k T / m, \\
        \end{split}

    where :math:`\vec{F}_\mathrm{C} = \vec{F}_\mathrm{net}` is the net force on
    the particle from all forces (`hoomd.md.Integrator.forces`) and constraints
    (`hoomd.md.Integrator.constraints`), :math:`\gamma` is the translational
    drag coefficient (`gamma`), :math:`\vec{F}_\mathrm{R}` is a uniform random
    force, :math:`\vec{v}` is the particle's velocity, and :math:`d` is the
    dimensionality of the system. The magnitude of the random force is chosen
    via the fluctuation-dissipation theorem to be consistent with the specified
    drag and temperature, :math:`kT`.

    About axes where :math:`I^i > 0`, the rotational degrees of freedom follow:

    .. math::

        \begin{split}
        \frac{d\mathbf{q}}{dt} &= \frac{\vec{\tau}_\mathrm{C} +
        \vec{\tau}_\mathrm{R}}{\gamma_r}, \\
        \langle \vec{\tau}_\mathrm{R} \rangle &= 0, \\
        \langle \tau_\mathrm{R}^i \cdot \tau_\mathrm{R}^i \rangle &=
        2 k T \gamma_r^i / \delta t, \\
        \langle \vec{L}(t) \rangle &= 0, \\
        \langle L^i(t) \cdot L^i(t) \rangle &= k T \cdot I^i, \\
        \end{split}

    where :math:`\vec{\tau}_\mathrm{C} = \vec{\tau}_\mathrm{net}`,
    :math:`\gamma_r^i` is the i-th component of the rotational drag coefficient
    (`gamma_r`), :math:`\tau_\mathrm{R}^i` is a component of the uniform random
    the torque, :math:`L^i` is the i-th component of the particle's angular
    momentum and :math:`I^i` is the i-th component of the particle's
    moment of inertia. The magnitude of the random torque is chosen
    via the fluctuation-dissipation theorem to be consistent with the specified
    drag and temperature, :math:`kT`.

    `Brownian` uses the numerical integration method from `I. Snook 2007`_, The
    Langevin and Generalised Langevin Approach to the Dynamics of Atomic,
    Polymeric and Colloidal Systems, section 6.2.5, with the exception that
    :math:`\vec{F}_\mathrm{R}` is drawn from a uniform random number
    distribution.

    .. _I. Snook 2007: https://dx.doi.org/10.1016/B978-0-444-52129-3.50028-6

    Warning:
        This numerical method has errors in :math:`O(\delta t)`, which is much
        larger than the errors of most other integration methods which are in
        :math:`O(\delta t^2)`. As a consequence, expect to use much smaller
        values of :math:`\delta t` with `Brownian` compared to e.g. `Langevin`
        or `ConstantVolume`.

    In Brownian dynamics, particle velocities and angular momenta are completely
    decoupled from positions. At each time step, `Brownian` draws a new velocity
    distribution consistent with the current set temperature so that
    `hoomd.md.compute.ThermodynamicQuantities` will report appropriate
    temperatures and pressures when logged or used by other methods.

    The attributes `gamma` and `gamma_r` set the translational and rotational
    damping coefficients, respectively, by particle type.

    .. rubric:: Example:

    .. code-block:: python

        brownian = hoomd.md.methods.Brownian(filter=hoomd.filter.All(), kT=1.5)
        simulation.operations.integrator.methods = [brownian]

    {inherited}


    **Members defined in** `Brownian`:

    Attributes:
        filter (hoomd.filter.filter_like): Subset of particles to apply this
            method to.

        kT (hoomd.variant.Variant): Temperature of the simulation
            :math:`[\mathrm{energy}]`.

            .. rubric:: Examples:

            .. code-block:: python

                brownian.kT = 1.0

            .. code-block:: python

                brownian.kT = hoomd.variant.Ramp(
                    A=2.0, B=1.0, t_start=0, t_ramp=1_000_000
                )

        gamma (TypeParameter[ ``particle type``, `float` ]): The drag
            coefficient for each particle type
            :math:`[\mathrm{mass} \cdot \mathrm{time}^{-1}]`.

            .. rubric:: Example:

            .. code-block:: python

                brownian.gamma["A"] = 0.5

        gamma_r (TypeParameter[``particle type``,[`float`, `float` , `float`]]):
            The rotational drag coefficient tensor for each particle type
            :math:`[\mathrm{time}^{-1}]`.

            .. rubric:: Example:

            .. code-block:: python

                brownian.gamma_r["A"] = [1.0, 2.0, 3.0]
    """

    __doc__ = inspect.cleandoc(__doc__).replace(
        "{inherited}", inspect.cleandoc(Method._doc_inherited)
    )

    def __init__(
        self,
        filter,
        kT,
        default_gamma=1.0,
        default_gamma_r=(1.0, 1.0, 1.0),
    ):
        # store metadata
        param_dict = ParameterDict(
            filter=ParticleFilter,
            kT=Variant,
        )
        param_dict.update(dict(kT=kT, filter=filter))

        # set defaults
        self._param_dict.update(param_dict)

        gamma = TypeParameter(
            "gamma",
            type_kind="particle_types",
            param_dict=TypeParameterDict(float, len_keys=1),
        )
        gamma.default = default_gamma

        gamma_r = TypeParameter(
            "gamma_r",
            type_kind="particle_types",
            param_dict=TypeParameterDict((float, float, float), len_keys=1),
        )

        gamma_r.default = default_gamma_r
        self._extend_typeparam([gamma, gamma_r])

    def _attach_hook(self):
        """BrownianModified uses RNGs. Warn the user if they did not set the seed."""
        self._simulation._warn_if_seed_unset()
        sim = self._simulation
        if isinstance(sim.device, hoomd.device.CPU):
            self._cpp_obj = _active.TwoStepBDModified(
                sim.state._cpp_sys_def,
                sim.state._get_group(self.filter),
                self.kT,
                False,
                False,
            )
        else:
            self._cpp_obj = _active.TwoStepBDModifiedGPU(
                sim.state._cpp_sys_def,
                sim.state._get_group(self.filter),
                self.kT,
                False,
                False,
            )

        # Attach param_dict and typeparam_dict
        super()._attach_hook()
