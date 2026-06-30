"""Pythonic API for VTK implicit functions.

Adds CSG operators, a callable protocol, and an informative repr::

    from vtkmodules.vtkCommonDataModel import vtkSphere, vtkPlane, vtkBox

    # Constructor kwargs (handled by the Python wrapping layer, which maps
    # snake_case keyword arguments to the corresponding Set methods):
    s = vtkSphere(center=(0, 0, 0), radius=1.0)
    p = vtkPlane(origin=(0, 0, 0), normal=(1, 0, 0))

    # CSG operators:
    union        = s | p       # vtkImplicitBoolean(VTK_UNION)
    intersection = s & p       # vtkImplicitBoolean(VTK_INTERSECTION)
    difference   = s - p       # vtkImplicitBoolean(VTK_DIFFERENCE)
    negated      = ~s           # vtkImplicitSum(weight=-1.0)

    # Callable protocol:
    s(1, 2, 3)                 # scalar → float
    s([1, 2, 3])               # 3-element sequence → float
    s(Nx3_array)               # batch → 1D numpy array

    # Repr:
    repr(s)  # → "vtkSphere(center=(0.0, 0.0, 0.0), radius=1.0)"
"""

from vtkmodules.vtkCommonDataModel import (
    vtkAnnulus,
    vtkBox,
    vtkCone,
    vtkCoordinateFrame,
    vtkCylinder,
    vtkFrustum,
    vtkImplicitBoolean,
    vtkImplicitDataSet,
    vtkImplicitFunction,
    vtkImplicitHalo,
    vtkImplicitSelectionLoop,
    vtkImplicitSum,
    vtkImplicitVolume,
    vtkImplicitWindowFunction,
    vtkPerlinNoise,
    vtkPlane,
    vtkPlanes,
    vtkPolyPlane,
    vtkQuadric,
    vtkSphere,
    vtkSpheres,
    vtkSuperquadric,
)

# ---------------------------------------------------------------------------
# Operation type names for the vtkImplicitBoolean repr
# ---------------------------------------------------------------------------

_OP_TYPE_REVERSE = {0: "Union", 1: "Intersection", 2: "Difference", 3: "UnionOfMagnitudes"}


# ---------------------------------------------------------------------------
# Helper: format a 3-tuple for repr
# ---------------------------------------------------------------------------

def _fmt3(vals):
    return "(%s, %s, %s)" % (vals[0], vals[1], vals[2])


# ---------------------------------------------------------------------------
# Base mixin: CSG operators + callable + generic repr
# ---------------------------------------------------------------------------

class _ImplicitFunctionMixin:

    def __or__(self, other):
        if not isinstance(other, vtkImplicitFunction):
            return NotImplemented
        b = vtkImplicitBoolean()
        b.SetOperationTypeToUnion()
        b.AddFunction(self)
        b.AddFunction(other)
        return b

    def __ror__(self, other):
        if not isinstance(other, vtkImplicitFunction):
            return NotImplemented
        b = vtkImplicitBoolean()
        b.SetOperationTypeToUnion()
        b.AddFunction(other)
        b.AddFunction(self)
        return b

    def __and__(self, other):
        if not isinstance(other, vtkImplicitFunction):
            return NotImplemented
        b = vtkImplicitBoolean()
        b.SetOperationTypeToIntersection()
        b.AddFunction(self)
        b.AddFunction(other)
        return b

    def __rand__(self, other):
        if not isinstance(other, vtkImplicitFunction):
            return NotImplemented
        b = vtkImplicitBoolean()
        b.SetOperationTypeToIntersection()
        b.AddFunction(other)
        b.AddFunction(self)
        return b

    def __sub__(self, other):
        if not isinstance(other, vtkImplicitFunction):
            return NotImplemented
        b = vtkImplicitBoolean()
        b.SetOperationTypeToDifference()
        b.AddFunction(self)
        b.AddFunction(other)
        return b

    def __rsub__(self, other):
        if not isinstance(other, vtkImplicitFunction):
            return NotImplemented
        b = vtkImplicitBoolean()
        b.SetOperationTypeToDifference()
        b.AddFunction(other)
        b.AddFunction(self)
        return b

    def __invert__(self):
        s = vtkImplicitSum()
        s.AddFunction(self, -1.0)
        return s

    def __call__(self, *args):
        if len(args) == 3:
            return self.FunctionValue(float(args[0]), float(args[1]), float(args[2]))
        if len(args) == 1:
            arg = args[0]
            try:
                length = len(arg)
            except TypeError:
                raise TypeError(
                    "Expected 3 scalars, a 3-element sequence, or an Nx3 array"
                )
            if length == 3:
                # Could be a single point [x,y,z] or an Nx3 array with N==3.
                # Check for nested structure (array-like with sub-elements).
                try:
                    inner = arg[0]
                    inner_len = len(inner)
                    # It's Nx3 with N==3
                except TypeError:
                    # It's a flat 3-element sequence
                    return self.FunctionValue(
                        float(arg[0]), float(arg[1]), float(arg[2])
                    )
            # Batch path: Nx3 array
            import numpy as np
            from vtkmodules.util.numpy_support import numpy_to_vtk, vtk_to_numpy

            arr = np.asarray(arg, dtype=np.float64)
            if arr.ndim != 2 or arr.shape[1] != 3:
                if arr.ndim == 1 and arr.shape[0] == 3:
                    return self.FunctionValue(
                        float(arr[0]), float(arr[1]), float(arr[2])
                    )
                raise ValueError(
                    "Expected an Nx3 array, got shape %s" % (arr.shape,)
                )
            inp = numpy_to_vtk(arr, deep=False)
            from vtkmodules.vtkCommonCore import vtkDoubleArray

            out = vtkDoubleArray()
            self.FunctionValue(inp, out)
            return vtk_to_numpy(out)
        raise TypeError(
            "Expected 3 scalars, a 3-element sequence, or an Nx3 array"
        )

    def __repr__(self):
        return "%s()" % self.GetClassName()


# ---------------------------------------------------------------------------
# Tier 1: classes with a custom repr (constructor kwargs come for free from
# the Python wrapping layer, which maps snake_case kwargs to Set methods)
# ---------------------------------------------------------------------------

class _SphereMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkSphere(center=%s, radius=%s)" % (
            _fmt3(self.GetCenter()),
            self.GetRadius(),
        )


class _PlaneMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkPlane(origin=%s, normal=%s, offset=%s)" % (
            _fmt3(self.GetOrigin()),
            _fmt3(self.GetNormal()),
            self.GetOffset(),
        )


class _BoxMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        b = self.GetBounds()
        return "vtkBox(bounds=(%s, %s, %s, %s, %s, %s))" % (
            b[0], b[1], b[2], b[3], b[4], b[5],
        )


class _CylinderMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkCylinder(center=%s, axis=%s, radius=%s)" % (
            _fmt3(self.GetCenter()),
            _fmt3(self.GetAxis()),
            self.GetRadius(),
        )


class _ConeMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkCone(origin=%s, axis=%s, angle=%s)" % (
            _fmt3(self.GetOrigin()),
            _fmt3(self.GetAxis()),
            self.GetAngle(),
        )


class _QuadricMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        c = self.GetCoefficients()
        return "vtkQuadric(coefficients=(%s,))" % ", ".join(str(v) for v in c)


class _SuperquadricMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkSuperquadric(center=%s, phi_roundness=%s, theta_roundness=%s)" % (
            _fmt3(self.GetCenter()),
            self.GetPhiRoundness(),
            self.GetThetaRoundness(),
        )


class _PerlinNoiseMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkPerlinNoise(frequency=%s, amplitude=%s)" % (
            _fmt3(self.GetFrequency()),
            self.GetAmplitude(),
        )


class _ImplicitBooleanMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        n = self.GetFunction().GetNumberOfItems() if self.GetFunction() else 0
        return "vtkImplicitBoolean(operation='%s', functions=%d)" % (
            _OP_TYPE_REVERSE.get(self.GetOperationType(), "Unknown"),
            n,
        )


class _ImplicitSumMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkImplicitSum(normalize_by_weight=%s)" % bool(
            self.GetNormalizeByWeight()
        )


class _AnnulusMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkAnnulus(center=%s, inner_radius=%s, outer_radius=%s)" % (
            _fmt3(self.GetCenter()),
            self.GetInnerRadius(),
            self.GetOuterRadius(),
        )


class _FrustumMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return (
            "vtkFrustum(near_plane_distance=%s, "
            "horizontal_angle=%s, vertical_angle=%s)"
            % (
                self.GetNearPlaneDistance(),
                self.GetHorizontalAngle(),
                self.GetVerticalAngle(),
            )
        )


class _ImplicitHaloMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkImplicitHalo(center=%s, radius=%s, fade_out=%s)" % (
            _fmt3(self.GetCenter()),
            self.GetRadius(),
            self.GetFadeOut(),
        )


class _CoordinateFrameMixin(_ImplicitFunctionMixin):
    def __repr__(self):
        return "vtkCoordinateFrame(origin=%s)" % _fmt3(self.GetOrigin())


# ---------------------------------------------------------------------------
# Register tier 1/2 overrides (classes with a custom repr)
# ---------------------------------------------------------------------------

@vtkSphere.override
class Sphere(_SphereMixin, vtkSphere):
    pass


@vtkPlane.override
class Plane(_PlaneMixin, vtkPlane):
    pass


@vtkBox.override
class Box(_BoxMixin, vtkBox):
    pass


@vtkCylinder.override
class Cylinder(_CylinderMixin, vtkCylinder):
    pass


@vtkCone.override
class Cone(_ConeMixin, vtkCone):
    pass


@vtkQuadric.override
class Quadric(_QuadricMixin, vtkQuadric):
    pass


@vtkSuperquadric.override
class Superquadric(_SuperquadricMixin, vtkSuperquadric):
    pass


@vtkPerlinNoise.override
class PerlinNoise(_PerlinNoiseMixin, vtkPerlinNoise):
    pass


@vtkImplicitBoolean.override
class ImplicitBoolean(_ImplicitBooleanMixin, vtkImplicitBoolean):
    pass


@vtkImplicitSum.override
class ImplicitSum(_ImplicitSumMixin, vtkImplicitSum):
    pass


@vtkAnnulus.override
class Annulus(_AnnulusMixin, vtkAnnulus):
    pass


@vtkFrustum.override
class Frustum(_FrustumMixin, vtkFrustum):
    pass


@vtkImplicitHalo.override
class ImplicitHalo(_ImplicitHaloMixin, vtkImplicitHalo):
    pass


@vtkCoordinateFrame.override
class CoordinateFrame(_CoordinateFrameMixin, vtkCoordinateFrame):
    pass


# ---------------------------------------------------------------------------
# Tier 3: classes with only CSG operators, callable, and generic repr
# ---------------------------------------------------------------------------

_TIER3_CLASSES = [
    vtkImplicitWindowFunction,
    vtkImplicitSelectionLoop,
    vtkImplicitDataSet,
    vtkImplicitVolume,
    vtkPlanes,
    vtkSpheres,
    vtkPolyPlane,
]

for _cls in _TIER3_CLASSES:
    # Create a new override class for each tier 3 class.
    # The class name matches the VTK class name without the 'vtk' prefix.
    _name = _cls.__name__[3:]  # strip 'vtk' prefix
    _override_cls = type(
        _name,
        (_ImplicitFunctionMixin, _cls),
        {},
    )
    _cls.override(_override_cls)

del _cls, _name, _override_cls
