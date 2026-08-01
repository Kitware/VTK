# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Pythonic overrides for the ``vtkViewsRendering`` module.

The wrapping layer already exposes snake_case properties (``color``,
``opacity``, ``representation`` ...) for every class in this module, and enum
properties backed by ``Set<Prop>To<Name>()`` methods already accept strings
(e.g. ``rep.representation = "wireframe"``, ``view.selection_mode = "frustum"``).
These overrides add the few things the wrapping does not provide on its own:

* named colors -- ``color="tomato"``, resolved via :mod:`vtkmodules.util.colors`
* a fluent ``view.show(source, color=...)`` helper that creates, configures,
  and adds a representation in one call
* ``view += representation`` / ``view -= representation``
* ``view.size = (width, height)``

The layer is registered for ``vtkViewsRendering`` in
``vtkmodules/__init__.py.in`` and loaded automatically when that module is
imported.
"""

from vtkmodules.util import colors as _colors
from vtkmodules.vtkViewsRendering import (
    vtkStandardRenderView,
    vtkSurfaceRepresentation,
    vtkVolumeRepresentation,
)

#: Representation classes :meth:`StandardRenderView.show` can build, keyed by
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


@vtkSurfaceRepresentation.override
class SurfaceRepresentation(vtkSurfaceRepresentation):
    """``vtkSurfaceRepresentation`` with named-color ``color``/``edge_color``."""

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


@vtkStandardRenderView.override
class StandardRenderView(vtkStandardRenderView):
    """``vtkStandardRenderView`` with named-color background, ``size``, ``show()``."""

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
            setattr(representation, name, value)
        self.AddRepresentation(representation)
        return representation
