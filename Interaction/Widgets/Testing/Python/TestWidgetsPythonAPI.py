"""Tests for the Pythonic widget API (vtkmodules.util.widgets)."""

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 — needed for factory overrides
from vtkmodules.vtkRenderingCore import vtkRenderWindow, vtkRenderWindowInteractor, vtkRenderer
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

# Some representations (e.g. vtkLineRepresentation) need a render window
# for their internal handle representations.
_renWin = vtkRenderWindow()
_renWin.SetOffScreenRendering(True)
_renWin.SetSize(300, 300)
_ren = vtkRenderer()
_renWin.AddRenderer(_ren)
_iren = vtkRenderWindowInteractor()
_iren.SetRenderWindow(_renWin)

BOUNDS = (-1, 1, -1, 1, -1, 1)


# ---------------------------------------------------------------------------
# vtkBoxWidget2
# ---------------------------------------------------------------------------

def test_box_widget_defaults():
    w = vtkBoxWidget2()
    assert w.GetRepresentation() is not None
    assert "vtkBoxWidget2" in repr(w)


def test_box_widget_bounds():
    w = vtkBoxWidget2(bounds=BOUNDS)
    assert w.GetRepresentation() is not None


# ---------------------------------------------------------------------------
# vtkSphereWidget2
# ---------------------------------------------------------------------------

def test_sphere_widget_kwargs():
    w = vtkSphereWidget2(bounds=BOUNDS, center=(1, 2, 3), radius=5.0)
    rep = w.GetRepresentation()
    c = rep.GetCenter()
    assert abs(c[0] - 1.0) < 1e-6
    assert abs(c[1] - 2.0) < 1e-6
    assert abs(c[2] - 3.0) < 1e-6
    assert abs(rep.GetRadius() - 5.0) < 1e-6
    assert "center=" in repr(w)
    assert "radius=" in repr(w)


# ---------------------------------------------------------------------------
# vtkImplicitPlaneWidget2
# ---------------------------------------------------------------------------

def test_implicit_plane_widget_kwargs():
    w = vtkImplicitPlaneWidget2(bounds=BOUNDS, origin=(0, 0, 0),
                                 normal=(0, 0, 1))
    rep = w.GetRepresentation()
    n = rep.GetNormal()
    assert abs(n[2] - 1.0) < 1e-6
    assert "normal=" in repr(w)
    assert "origin=" in repr(w)


# ---------------------------------------------------------------------------
# vtkLineWidget2
# ---------------------------------------------------------------------------

def test_line_widget_kwargs():
    w = vtkLineWidget2(interactor=_iren, bounds=BOUNDS,
                        point1=(-1, 0, 0), point2=(1, 0, 0))
    rep = w.GetRepresentation()
    p1 = rep.GetPoint1WorldPosition()
    p2 = rep.GetPoint2WorldPosition()
    assert abs(p1[0] - (-1.0)) < 1e-6
    assert abs(p2[0] - 1.0) < 1e-6
    assert "point1=" in repr(w)


# ---------------------------------------------------------------------------
# vtkSplineWidget2
# ---------------------------------------------------------------------------

def test_spline_widget_defaults():
    w = vtkSplineWidget2(bounds=BOUNDS)
    assert w.GetRepresentation() is not None
    assert "vtkSplineWidget2" in repr(w)


# ---------------------------------------------------------------------------
# vtkSliderWidget
# ---------------------------------------------------------------------------

def test_slider_widget_kwargs():
    w = vtkSliderWidget(min_value=0, max_value=100, value=50,
                        title="Opacity")
    rep = w.GetRepresentation()
    assert abs(rep.GetMinimumValue() - 0.0) < 1e-6
    assert abs(rep.GetMaximumValue() - 100.0) < 1e-6
    assert abs(rep.GetValue() - 50.0) < 1e-6
    assert "value=" in repr(w)
    assert "range=" in repr(w)


def test_slider_widget_custom_position():
    w = vtkSliderWidget(min_value=0, max_value=1, point1=(0.2, 0.2),
                        point2=(0.8, 0.8))
    rep = w.GetRepresentation()
    assert rep is not None


# ---------------------------------------------------------------------------
# vtkImplicitCylinderWidget
# ---------------------------------------------------------------------------

def test_implicit_cylinder_widget_kwargs():
    w = vtkImplicitCylinderWidget(bounds=BOUNDS, center=(0, 0, 0),
                                   axis=(0, 1, 0), radius=0.5)
    rep = w.GetRepresentation()
    assert abs(rep.GetRadius() - 0.5) < 1e-6
    a = rep.GetAxis()
    assert abs(a[1] - 1.0) < 1e-6
    assert "radius=" in repr(w)


# ---------------------------------------------------------------------------
# vtkImplicitConeWidget
# ---------------------------------------------------------------------------

def test_implicit_cone_widget_kwargs():
    w = vtkImplicitConeWidget(bounds=BOUNDS, origin=(0, 0, 0),
                               axis=(1, 0, 0), angle=30.0)
    rep = w.GetRepresentation()
    assert abs(rep.GetAngle() - 30.0) < 1e-6
    assert "angle=" in repr(w)


# ---------------------------------------------------------------------------
# vtkImplicitAnnulusWidget
# ---------------------------------------------------------------------------

def test_implicit_annulus_widget_kwargs():
    w = vtkImplicitAnnulusWidget(bounds=BOUNDS, center=(0, 0, 0),
                                  axis=(0, 0, 1),
                                  inner_radius=0.2, outer_radius=0.8)
    rep = w.GetRepresentation()
    assert abs(rep.GetInnerRadius() - 0.2) < 1e-6
    assert abs(rep.GetOuterRadius() - 0.8) < 1e-6
    assert "inner_radius=" in repr(w)


# ---------------------------------------------------------------------------
# vtkCoordinateFrameWidget
# ---------------------------------------------------------------------------

def test_coordinate_frame_widget_kwargs():
    w = vtkCoordinateFrameWidget(bounds=BOUNDS, origin=(1, 2, 3))
    rep = w.GetRepresentation()
    o = rep.GetOrigin()
    assert abs(o[0] - 1.0) < 1e-6
    assert abs(o[1] - 2.0) < 1e-6
    assert abs(o[2] - 3.0) < 1e-6
    assert "origin=" in repr(w)


# ---------------------------------------------------------------------------
# __getattr__ forwarding
# ---------------------------------------------------------------------------

def test_getattr_forwarding():
    """Widget attribute access forwards to representation."""
    w = vtkImplicitPlaneWidget2(bounds=BOUNDS, normal=(0, 1, 0))
    # GetNormal is on the representation, not the widget
    n = w.GetNormal()
    assert abs(n[1] - 1.0) < 1e-6

    w2 = vtkSphereWidget2(bounds=BOUNDS, radius=3.0)
    assert abs(w2.GetRadius() - 3.0) < 1e-6


def test_getattr_widget_method_takes_priority():
    """Widget's own methods are not shadowed by __getattr__."""
    w = vtkBoxWidget2(bounds=BOUNDS)
    # GetEnabled is on the widget itself (from vtkAbstractWidget)
    assert w.GetEnabled() == 0


# ---------------------------------------------------------------------------
# Decorator event callbacks
# ---------------------------------------------------------------------------

def test_on_interaction_decorator():
    w = vtkBoxWidget2(bounds=BOUNDS)
    called = []

    @w.on_interaction
    def handler():
        called.append(True)

    # handler should be returned unchanged
    assert callable(handler)


def test_on_end_interaction_decorator():
    w = vtkBoxWidget2(bounds=BOUNDS)
    called = []

    @w.on_end_interaction
    def handler():
        called.append(True)

    assert callable(handler)


# ---------------------------------------------------------------------------
# Context manager
# ---------------------------------------------------------------------------

def test_context_manager():
    w = vtkBoxWidget2(bounds=BOUNDS)
    # Cannot truly enable without an interactor, but test the protocol
    assert hasattr(w, '__enter__')
    assert hasattr(w, '__exit__')


# ---------------------------------------------------------------------------
# on/off chaining
# ---------------------------------------------------------------------------

def test_on_off_returns_self():
    w = vtkBoxWidget2(bounds=BOUNDS)
    # off() should return self for chaining
    result = w.off()
    assert result is w


# ---------------------------------------------------------------------------
# Run all tests
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    import sys
    test_functions = [v for k, v in sorted(globals().items())
                      if k.startswith("test_") and callable(v)]
    failed = []
    for fn in test_functions:
        try:
            fn()
            print("  PASS:", fn.__name__)
        except Exception as e:
            print("  FAIL:", fn.__name__, "-", e)
            failed.append(fn.__name__)

    if failed:
        print("\n%d test(s) FAILED: %s" % (len(failed), ", ".join(failed)))
        sys.exit(1)
    else:
        print("\nAll %d tests passed." % len(test_functions))
        sys.exit(0)
