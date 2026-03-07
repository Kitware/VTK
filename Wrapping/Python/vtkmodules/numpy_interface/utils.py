"""Shared utilities for the new VTK array classes.

This module provides common definitions used across VTKAOSArray,
VTKSOAArray, VTKCompositeArray, and VTKNoneArray.  It is intentionally
independent of ``dataset_adapter`` so that the new array classes do not
depend on the legacy module.
"""
import numpy

from ..vtkCommonDataModel import vtkDataObject


class ArrayAssociation:
    """Easy access to vtkDataObject.AttributeTypes."""
    POINT = vtkDataObject.POINT
    CELL  = vtkDataObject.CELL
    FIELD = vtkDataObject.FIELD
    ROW   = vtkDataObject.ROW


# Re-export the singleton NoneArray from its own module.
from .vtk_none_array import NoneArray


def reshape_append_ones(a1, a2):
    """Append trailing 1s to the shorter shape for broadcasting.

    Given two ndarrays, append 1s to the shape of the array with fewer
    dimensions until both have the same number of dimensions.  This
    allows operations like ``(n,3) + (n,)`` that would otherwise fail
    with a broadcast error.

    Returns a list ``[a1, a2]`` where one may have been reshaped.
    """
    l = [a1, a2]
    if isinstance(a1, numpy.ndarray) and isinstance(a2, numpy.ndarray):
        len1 = len(a1.shape)
        len2 = len(a2.shape)
        if (len1 == len2 or len1 == 0 or len2 == 0 or
                a1.shape[0] != a2.shape[0]):
            return l
        elif len1 < len2:
            d = len1
            maxLength = len2
            i = 0
        else:
            d = len2
            maxLength = len1
            i = 1
        while d < maxLength:
            l[i] = numpy.expand_dims(l[i], d)
            d = d + 1
    return l
