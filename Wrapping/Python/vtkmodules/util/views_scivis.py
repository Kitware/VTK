# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Pythonic overrides for the ``vtkViewsScivis`` module.

The wrapping layer already exposes snake_case properties (``color``,
``opacity``, ``representation`` ...) for every class in this module, and enum
properties backed by ``Set<Prop>To<Name>()`` methods already accept strings
(e.g. ``rep.representation = "wireframe"``, ``view.selector.mode = "frustum"``).
These overrides add the few things the wrapping does not provide on its own:

* named colors -- ``color="tomato"``, resolved via :mod:`vtkmodules.util.colors`
* delegation of properties the C++ classes deliberately do not carry -- setting
  ``rep.specular`` or ``view.key_light_intensity`` reaches the property,
  geometry filter or light kit that owns it, so the flat spelling that makes
  keyword arguments natural survives the smaller C++ API
* the same in the constructor, including a dict per sub-object --
  ``vtkScivisView(selector={"mode": "frustum"})``
* a fluent ``view.show(source, color=...)`` helper that creates, configures,
  and adds a representation in one call
* ``view += representation`` / ``view -= representation``
* ``view.size = (width, height)``

The layer is registered for ``vtkViewsScivis`` in
``vtkmodules/__init__.py.in`` and loaded automatically when that module is
imported.
"""

from vtkmodules.util import colors as _colors
from vtkmodules.vtkViewsScivis import (
    vtkScivisView,
    vtkSurfaceRepresentation,
    vtkVolumeRepresentation,
)

#: Representation classes :meth:`ScivisView.show` can build, keyed by
#: the name accepted as its ``representation_type`` argument.  Add an entry to
#: make a representation of your own available by name; ``show()`` also accepts
#: a class directly, so registering is a convenience rather than a requirement.
REPRESENTATION_TYPES = {
    "surface": vtkSurfaceRepresentation,
    "volume": vtkVolumeRepresentation,
}


def _make_representation(representation_type):
    """Build a representation for *representation_type*.

    Accepts ``None`` (a surface representation), a name from
    :data:`REPRESENTATION_TYPES`, or a representation class.
    """
    if representation_type is None:
        return vtkSurfaceRepresentation()
    if isinstance(representation_type, str):
        try:
            factory = REPRESENTATION_TYPES[representation_type.lower()]
        except KeyError:
            raise ValueError(
                f"unknown representation type {representation_type!r}; use one of "
                f"{sorted(REPRESENTATION_TYPES)} or pass a representation class"
            ) from None
        return factory()
    return representation_type()


def _resolve_color(color):
    """Return an ``(r, g, b)`` tuple for *color*.

    *color* is either a 3-sequence with components in ``[0, 1]`` or a
    snake_case named color from :mod:`vtkmodules.util.colors` (e.g.
    ``"tomato"``, ``"steel_blue"``).
    """
    if isinstance(color, str):
        try:
            return tuple(getattr(_colors, color))
        except AttributeError:
            raise ValueError(
                f"unknown color name {color!r}; use an (r, g, b) tuple or a "
                "snake_case name from vtkmodules.util.colors"
            )
    return tuple(color)



def _check_property(obj, name):
    """Raise for a keyword argument nothing on *obj* owns.

    Attribute assignment on a Python subclass of a VTK object quietly creates an
    instance attribute, which would turn a misspelled keyword argument into a
    property that silently does nothing.  Keyword arguments are checked here so
    that they fail where the mistake was made.
    """
    if hasattr(type(obj), name):
        return
    owner = getattr(obj, "_delegate_owning", None)
    if owner is not None and owner(name) is not None:
        return
    looked = getattr(type(obj), "_delegates", ())
    raise ValueError(
        f"{type(obj).__name__} has no property {name!r}"
        + (f"; looked in {', '.join(a + '()' for a in looked)}" if looked else "")
    )


class _Delegating:
    """Route properties to the object that owns them.

    The C++ classes carry only what it takes to make data visible and legible;
    everything else lives on the actor, property, mapper or filter underneath.
    That is the right shape for C++, where reaching one object further is a
    method call, but it would spoil ``view.show(src, specular=0.3)`` in Python,
    where flat keyword arguments are the whole point.

    So an attribute this class does not define is looked for on each delegate in
    turn, and set on the first that has it.  ``rep.specular = 0.3`` and
    ``rep.property.specular = 0.3`` are then the same thing, and an attribute
    nobody owns still fails, naming the delegates it looked in.
    """

    #: Accessors for the objects to look in, in order.
    _delegates = ()

    def __init__(self, **kwargs):
        """Accept in the constructor everything assignment accepts.

        VTK builds its own constructor keyword handling from the properties a
        class declares, and it runs before any Python attribute assignment -- so
        it never reaches the delegation below, and ``view.specular = 0.3`` would
        work while ``Class(specular=0.3)`` did not.  Anything VTK does not
        recognise is applied here instead, which puts the two back in step.

        A dict value configures the sub-object of that name, so the grouping the
        C++ API has is available at construction too::

            view = vtkScivisView(
                window_title="demo",
                selector={"mode": "frustum"},
                light_kit={"key_light_intensity": 0.8},
            )

        There is no way to spell that as a keyword name -- ``selector.mode=`` is
        a syntax error -- and a dict means nothing to an ordinary VTK property,
        so there is nothing for it to collide with.
        """
        native, deferred = {}, {}
        for name, value in kwargs.items():
            # A dict is always ours to apply: VTK would try to assign it.
            if isinstance(value, dict) or not hasattr(type(self), name):
                deferred[name] = value
            else:
                native[name] = value
        super().__init__(**native)

        for name, value in deferred.items():
            if isinstance(value, dict):
                _check_property(self, name)
                target = getattr(self, name)
                if target is None:
                    raise ValueError(f"{type(self).__name__}.{name} is not set, so it cannot be "
                                     "configured from the constructor")
                for subname, subvalue in value.items():
                    _check_property(target, subname)
                    setattr(target, subname, subvalue)
            else:
                _check_property(self, name)
                setattr(self, name, value)

    def _delegate_owning(self, name):
        # Properties only.  VTK methods are CamelCase and properties are
        # snake_case, and forwarding methods would quietly hand back API the C++
        # classes deliberately do not offer -- GetActor() has a GetMapper(), so
        # rep.GetMapper would otherwise resolve through it.
        if not name[:1].islower():
            return None
        for accessor in self._delegates:
            target = getattr(self, accessor)()
            if target is not None and hasattr(type(target), name):
                return target
        return None

    def __getattr__(self, name):
        # Only reached when normal lookup fails, so there is no risk of
        # shadowing a property this class or the wrapping already defines.
        if not name.startswith("_"):
            target = self._delegate_owning(name)
            if target is not None:
                return getattr(target, name)
        raise AttributeError(
            f"{type(self).__name__!r} object has no attribute {name!r}; looked in "
            f"{', '.join(a + '()' for a in self._delegates)}"
        )

    def __setattr__(self, name, value):
        if not name.startswith("_") and not hasattr(type(self), name):
            target = self._delegate_owning(name)
            if target is not None:
                setattr(target, name, value)
                return
        super().__setattr__(name, value)


@vtkSurfaceRepresentation.override
class SurfaceRepresentation(_Delegating, vtkSurfaceRepresentation):
    """``vtkSurfaceRepresentation`` with named colors and property delegation."""

    _delegates = ("GetProperty", "GetGeometryFilter", "GetActor")

    @property
    def color(self):
        return self.GetColor()

    @color.setter
    def color(self, value):
        self.SetColor(*_resolve_color(value))

    @property
    def edge_color(self):
        return self.GetEdgeColor()

    @edge_color.setter
    def edge_color(self, value):
        self.SetEdgeColor(*_resolve_color(value))


@vtkScivisView.override
class ScivisView(_Delegating, vtkScivisView):
    """``vtkScivisView`` with named colors, ``size``, ``show()``."""

    _delegates = ("GetLightKit", "GetOrientationMarkerWidget", "GetRenderer")

    @property
    def background(self):
        return self.GetBackground()

    @background.setter
    def background(self, value):
        self.SetBackground(*_resolve_color(value))

    @property
    def background2(self):
        return self.GetBackground2()

    @background2.setter
    def background2(self, value):
        self.SetBackground2(*_resolve_color(value))

    @property
    def size(self):
        window = self.GetRenderWindow()
        return tuple(window.GetSize()) if window else (0, 0)

    @size.setter
    def size(self, value):
        self.SetWindowSize(*value)

    def __iadd__(self, representation):
        self.AddRepresentation(representation)
        return self

    def __isub__(self, representation):
        self.RemoveRepresentation(representation)
        return self

    def show(self, input, representation_type=None, **properties):
        """Add a representation for *input* to the view and return it.

        *input* is a ``vtkAlgorithm`` (its output port is connected) or a
        ``vtkDataObject``.  Keyword arguments are applied as properties on the
        created representation::

            view.show(sphere, color="tomato", opacity=0.8,
                      representation="surfacewithedges")

        *representation_type* selects what to build: a name from
        :data:`REPRESENTATION_TYPES`, a representation class, or ``None`` for a
        surface representation::

            view.show(wavelet, "volume", scalar_opacity_unit_distance=0.8)
            view.show(wavelet, MyRepresentation)
        """
        representation = _make_representation(representation_type)
        input >> representation
        for name, value in properties.items():
            _check_property(representation, name)
            setattr(representation, name, value)
        self.AddRepresentation(representation)
        return representation


@vtkVolumeRepresentation.override
class VolumeRepresentation(_Delegating, vtkVolumeRepresentation):
    """``vtkVolumeRepresentation`` with property delegation."""

    _delegates = ("GetVolumeProperty", "GetVolumeMapper", "GetVolume")
