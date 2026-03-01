"""Pythonic API for VTK widgets.

Reduces boilerplate by auto-creating representations, forwarding attribute
access to the representation, and providing decorator-style event callbacks::

    from vtkmodules.vtkInteractionWidgets import vtkImplicitPlaneWidget2

    widget = vtkImplicitPlaneWidget2(iren, bounds=actor.GetBounds(), normal=(0, 0, 1))

    @widget.on_interaction
    def update():
        widget.GetPlane(plane)

    widget.on()
"""

from vtkmodules.vtkInteractionWidgets import (
    vtkBoxWidget2,
    vtkCoordinateFrameWidget,
    vtkImplicitAnnulusWidget,
    vtkImplicitConeWidget,
    vtkImplicitCylinderWidget,
    vtkImplicitPlaneWidget2,
    vtkLineWidget2,
    vtkSliderWidget,
    vtkSphereWidget2,
    vtkSplineWidget2,
)


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def _fmt3(vals):
    return "(%s, %s, %s)" % (vals[0], vals[1], vals[2])


# ---------------------------------------------------------------------------
# Base mixin
# ---------------------------------------------------------------------------

class _WidgetMixin:
    """Shared functionality for all pythonic widget overrides."""

    def __getattr__(self, name):
        # Only called when normal attribute lookup fails.
        # Forward to the representation.
        rep = super().__getattribute__("GetRepresentation")()
        if rep is not None:
            return getattr(rep, name)
        raise AttributeError(name)

    def on_interaction(self, callback):
        """Decorator: call *callback()* on every InteractionEvent."""
        self.AddObserver("InteractionEvent", lambda w, e: callback())
        return callback

    def on_end_interaction(self, callback):
        """Decorator: call *callback()* on EndInteractionEvent."""
        self.AddObserver("EndInteractionEvent", lambda w, e: callback())
        return callback

    def on(self):
        """Enable the widget. Returns *self* for chaining."""
        self.On()
        return self

    def off(self):
        """Disable the widget. Returns *self* for chaining."""
        self.Off()
        return self

    def __enter__(self):
        self.On()
        return self

    def __exit__(self, *exc):
        self.Off()
        return False

    def __repr__(self):
        return "%s()" % self.GetClassName()


# ---------------------------------------------------------------------------
# Per-widget mixins
# ---------------------------------------------------------------------------

class _BoxWidget2Mixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, bounds=None):
        if args and isinstance(args[0], str):
            return
        self.CreateDefaultRepresentation()
        if interactor is not None:
            self.SetInteractor(interactor)
        if bounds is not None:
            self.GetRepresentation().PlaceWidget(bounds)

    def __repr__(self):
        return "vtkBoxWidget2()"


class _SphereWidget2Mixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, bounds=None, center=None,
                 radius=None):
        if args and isinstance(args[0], str):
            return
        self.CreateDefaultRepresentation()
        if interactor is not None:
            self.SetInteractor(interactor)
        rep = self.GetRepresentation()
        if bounds is not None:
            rep.PlaceWidget(bounds)
        if center is not None:
            rep.SetCenter(center)
        if radius is not None:
            rep.SetRadius(radius)

    def __repr__(self):
        rep = self.GetRepresentation()
        if rep is not None:
            return "vtkSphereWidget2(center=%s, radius=%s)" % (
                _fmt3(rep.GetCenter()), rep.GetRadius())
        return "vtkSphereWidget2()"


class _ImplicitPlaneWidget2Mixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, bounds=None, origin=None,
                 normal=None):
        if args and isinstance(args[0], str):
            return
        self.CreateDefaultRepresentation()
        if interactor is not None:
            self.SetInteractor(interactor)
        rep = self.GetRepresentation()
        if bounds is not None:
            rep.PlaceWidget(bounds)
        if origin is not None:
            rep.SetOrigin(origin)
        if normal is not None:
            rep.SetNormal(normal)

    def __repr__(self):
        rep = self.GetRepresentation()
        if rep is not None:
            return "vtkImplicitPlaneWidget2(origin=%s, normal=%s)" % (
                _fmt3(rep.GetOrigin()), _fmt3(rep.GetNormal()))
        return "vtkImplicitPlaneWidget2()"


class _LineWidget2Mixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, bounds=None, point1=None,
                 point2=None):
        if args and isinstance(args[0], str):
            return
        self.CreateDefaultRepresentation()
        if interactor is not None:
            self.SetInteractor(interactor)
        rep = self.GetRepresentation()
        if bounds is not None:
            rep.PlaceWidget(bounds)
        if point1 is not None:
            rep.SetPoint1WorldPosition(point1)
        if point2 is not None:
            rep.SetPoint2WorldPosition(point2)

    def __repr__(self):
        rep = self.GetRepresentation()
        if rep is not None:
            return "vtkLineWidget2(point1=%s, point2=%s)" % (
                _fmt3(rep.GetPoint1WorldPosition()),
                _fmt3(rep.GetPoint2WorldPosition()))
        return "vtkLineWidget2()"


class _SplineWidget2Mixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, bounds=None):
        if args and isinstance(args[0], str):
            return
        self.CreateDefaultRepresentation()
        if interactor is not None:
            self.SetInteractor(interactor)
        if bounds is not None:
            self.GetRepresentation().PlaceWidget(bounds)

    def __repr__(self):
        return "vtkSplineWidget2()"


class _SliderWidgetMixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, value=None, min_value=None,
                 max_value=None, title=None, point1=None, point2=None):
        if args and isinstance(args[0], str):
            return
        # SliderWidget needs explicit 2D representation
        from vtkmodules.vtkInteractionWidgets import vtkSliderRepresentation2D
        rep = vtkSliderRepresentation2D()
        # Default horizontal placement near bottom of viewport
        p1 = point1 if point1 is not None else (0.1, 0.1)
        p2 = point2 if point2 is not None else (0.9, 0.1)
        rep.GetPoint1Coordinate().SetCoordinateSystemToNormalizedViewport()
        rep.GetPoint1Coordinate().SetValue(p1[0], p1[1])
        rep.GetPoint2Coordinate().SetCoordinateSystemToNormalizedViewport()
        rep.GetPoint2Coordinate().SetValue(p2[0], p2[1])
        if min_value is not None:
            rep.SetMinimumValue(min_value)
        if max_value is not None:
            rep.SetMaximumValue(max_value)
        if value is not None:
            rep.SetValue(value)
        if title is not None:
            rep.SetTitleText(title)
        self.SetRepresentation(rep)
        if interactor is not None:
            self.SetInteractor(interactor)

    def __repr__(self):
        rep = self.GetRepresentation()
        if rep is not None:
            return "vtkSliderWidget(value=%s, range=[%s, %s])" % (
                rep.GetValue(), rep.GetMinimumValue(), rep.GetMaximumValue())
        return "vtkSliderWidget()"


class _ImplicitCylinderWidgetMixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, bounds=None, center=None,
                 axis=None, radius=None):
        if args and isinstance(args[0], str):
            return
        self.CreateDefaultRepresentation()
        if interactor is not None:
            self.SetInteractor(interactor)
        rep = self.GetRepresentation()
        if bounds is not None:
            rep.PlaceWidget(bounds)
        if center is not None:
            rep.SetCenter(center)
        if axis is not None:
            rep.SetAxis(axis)
        if radius is not None:
            rep.SetRadius(radius)

    def __repr__(self):
        rep = self.GetRepresentation()
        if rep is not None:
            return "vtkImplicitCylinderWidget(center=%s, axis=%s, radius=%s)" % (
                _fmt3(rep.GetCenter()), _fmt3(rep.GetAxis()), rep.GetRadius())
        return "vtkImplicitCylinderWidget()"


class _ImplicitConeWidgetMixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, bounds=None, origin=None,
                 axis=None, angle=None):
        if args and isinstance(args[0], str):
            return
        self.CreateDefaultRepresentation()
        if interactor is not None:
            self.SetInteractor(interactor)
        rep = self.GetRepresentation()
        if bounds is not None:
            rep.PlaceWidget(bounds)
        if origin is not None:
            rep.SetOrigin(origin)
        if axis is not None:
            rep.SetAxis(axis)
        if angle is not None:
            rep.SetAngle(angle)

    def __repr__(self):
        rep = self.GetRepresentation()
        if rep is not None:
            return "vtkImplicitConeWidget(origin=%s, axis=%s, angle=%s)" % (
                _fmt3(rep.GetOrigin()), _fmt3(rep.GetAxis()), rep.GetAngle())
        return "vtkImplicitConeWidget()"


class _ImplicitAnnulusWidgetMixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, bounds=None, center=None,
                 axis=None, inner_radius=None, outer_radius=None):
        if args and isinstance(args[0], str):
            return
        self.CreateDefaultRepresentation()
        if interactor is not None:
            self.SetInteractor(interactor)
        rep = self.GetRepresentation()
        if bounds is not None:
            rep.PlaceWidget(bounds)
        if center is not None:
            rep.SetCenter(center)
        if axis is not None:
            rep.SetAxis(axis)
        if inner_radius is not None:
            rep.SetInnerRadius(inner_radius)
        if outer_radius is not None:
            rep.SetOuterRadius(outer_radius)

    def __repr__(self):
        rep = self.GetRepresentation()
        if rep is not None:
            return ("vtkImplicitAnnulusWidget(center=%s, "
                    "inner_radius=%s, outer_radius=%s)") % (
                _fmt3(rep.GetCenter()),
                rep.GetInnerRadius(), rep.GetOuterRadius())
        return "vtkImplicitAnnulusWidget()"


class _CoordinateFrameWidgetMixin(_WidgetMixin):
    def __init__(self, *args, interactor=None, bounds=None, origin=None):
        if args and isinstance(args[0], str):
            return
        self.CreateDefaultRepresentation()
        if interactor is not None:
            self.SetInteractor(interactor)
        rep = self.GetRepresentation()
        if bounds is not None:
            rep.PlaceWidget(bounds)
        if origin is not None:
            rep.SetOrigin(origin)

    def __repr__(self):
        rep = self.GetRepresentation()
        if rep is not None:
            return "vtkCoordinateFrameWidget(origin=%s)" % _fmt3(rep.GetOrigin())
        return "vtkCoordinateFrameWidget()"


# ---------------------------------------------------------------------------
# Register overrides
# ---------------------------------------------------------------------------

@vtkBoxWidget2.override
class BoxWidget2(_BoxWidget2Mixin, vtkBoxWidget2):
    pass


@vtkSphereWidget2.override
class SphereWidget2(_SphereWidget2Mixin, vtkSphereWidget2):
    pass


@vtkImplicitPlaneWidget2.override
class ImplicitPlaneWidget2(_ImplicitPlaneWidget2Mixin, vtkImplicitPlaneWidget2):
    pass


@vtkLineWidget2.override
class LineWidget2(_LineWidget2Mixin, vtkLineWidget2):
    pass


@vtkSplineWidget2.override
class SplineWidget2(_SplineWidget2Mixin, vtkSplineWidget2):
    pass


@vtkSliderWidget.override
class SliderWidget(_SliderWidgetMixin, vtkSliderWidget):
    pass


@vtkImplicitCylinderWidget.override
class ImplicitCylinderWidget(_ImplicitCylinderWidgetMixin, vtkImplicitCylinderWidget):
    pass


@vtkImplicitConeWidget.override
class ImplicitConeWidget(_ImplicitConeWidgetMixin, vtkImplicitConeWidget):
    pass


@vtkImplicitAnnulusWidget.override
class ImplicitAnnulusWidget(_ImplicitAnnulusWidgetMixin, vtkImplicitAnnulusWidget):
    pass


@vtkCoordinateFrameWidget.override
class CoordinateFrameWidget(_CoordinateFrameWidgetMixin, vtkCoordinateFrameWidget):
    pass
