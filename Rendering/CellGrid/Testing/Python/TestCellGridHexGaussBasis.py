# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Verify the GPU evaluation of the vtkDGHex "G" (HGRAD) basis functions.
"""

import math
import os
import sys

from pathlib import Path

from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkIOCellGrid as io
from vtkmodules import vtkFiltersCellGrid as fc
from vtkmodules import vtkRenderingCore as rr
from vtkmodules import vtkRenderingCellGrid as rg

import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
from vtkmodules.test import Testing

VTK_DATA_ROOT = vtkGetDataRoot()

# Register render responder for DG cells:
rg.vtkRenderingCellGrid.RegisterCellsAndResponders()

# Cell-attribute name -> nominal polynomial order.
FIELDS = [
    ('G5', 5),
    ('G4', 4),
    ('G3', 3),
    ('G2', 2),
    ('G1', 1),
]

# (roll, azimuth, elevation) for the montage.
VIEWS = [(0, 0, 0), (0, 90, 0), (0, 180, 0), (0, 270, 0), (0, 0, 85), (0, 0, -85)]

EPS = sys.float_info.epsilon

# Gauss-Legendre points per order (order -> order+1 points), same table as
# the `gpts` array in vtkDGHGradOperators.cxx.
GPTS = [
    [0.0],
    [-0.577350269189625731, 0.577350269189625731],
    [-0.774596669241483404, 0.0, 0.774596669241483404],
    [-0.861136311594052462, -0.339981043584856257, 0.339981043584856257,
     0.861136311594052462],
    [-0.906179845938663853, -0.538469310105682997, 0.0,
     0.538469310105682997, 0.906179845938663853],
    [-0.932469514203152050, -0.661209386466264482, -0.238619186083196932,
     0.238619186083196932, 0.661209386466264482, 0.932469514203152050],
]


def HexGaussBasis(order, rr_, ss, tt):
    """Reference for Filters/CellGrid/Basis/HGrad/HexGnBasis.h."""
    pts = GPTS[order]
    rterm, sterm, tterm = [], [], []
    for ii in range(order + 1):
        rt = st = tv = 1.
        for jj in range(order + 1):
            if ii == jj:
                continue
            rt *= (rr_ - pts[jj]) / (pts[ii] - pts[jj])
            st *= (ss - pts[jj]) / (pts[ii] - pts[jj])
            tv *= (tt - pts[jj]) / (pts[ii] - pts[jj])
        rterm.append(rt)
        sterm.append(st)
        tterm.append(tv)

    basis = []
    for kk in range(order + 1):
        for jj in range(order + 1):
            for ii in range(order + 1):
                basis.append(rterm[ii] * sterm[jj] * tterm[kk])
    return basis


class TestCellGridHexGaussBasis(Testing.vtkTest):

    dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra-GBasis.dg')

    def _readCoefficients(self):
        """Per-cell degree-of-freedom vectors, straight out of the file."""
        reader = io.vtkCellGridReader(file_name=self.dataFile)
        reader.Update()
        grid = reader.output
        coeffs = {}
        for name, _ in FIELDS:
            array = grid.GetAttributes('vtkDGHex').GetArray(name)
            self.assertIsNotNone(
                array, '%s is missing from %s' % (name, self.dataFile))
            nc = array.GetNumberOfComponents()
            coeffs[name] = [[array.GetComponent(cell, comp) for comp in range(nc)]
                            for cell in range(array.GetNumberOfTuples())]
        return coeffs

    def _make_sides(self):
        reader = io.vtkCellGridReader(file_name=self.dataFile)
        sides = fc.vtkCellGridComputeSides(
            output_dimension_control=dm.vtkCellGridSidesQuery.NextLowestDimension,
            preserve_renderable_inputs=False,
            omit_sides_for_renderable_inputs=False,
        )
        reader >> sides
        return sides

    def _make_mapper(self, sides, field, scalar_range):

        # mimics vtkCellGridMapper::PrepareColorMap but with correct scalar_range
        ctf = rr.vtkColorTransferFunction(color_space=rr.VTK_CTF_DIVERGING)
        lo, hi = scalar_range
        span = hi - lo
        ctf.AddRGBPoint(lo, 59. / 255., 76. / 255., 192. / 255.)
        ctf.AddRGBPoint(lo + 0.5 * span, 221. / 255., 221. / 255., 221. / 255.)
        ctf.AddRGBPoint(lo + span, 180. / 255., 4. / 255., 38. / 255.)
        ctf.Build()
        mapper = rr.vtkCellGridMapper(
            scalar_visibility=True,
            scalar_mode=rr.VTK_SCALAR_MODE_USE_CELL_FIELD_DATA,
            array_name=field,
            array_component=-2,
            lookup_table=ctf,
            use_lookup_table_scalar_range=True,
            scalar_range=scalar_range,
        )
        sides >> mapper
        return mapper

    def testMontageRendering(self):
        """Draw every field from every angle; exercises all faces and shading."""
        coeffs = self._readCoefficients()
        tile = 160
        window = rr.vtkRenderWindow(
            size=(tile * len(VIEWS), tile * len(FIELDS)),
            window_name="vtkDGHex HGRAD 'G' basis",
        )

        for row, (field, order) in enumerate(FIELDS):
            lo, hi = self._fieldRange(coeffs[field], order)
            print(field, lo, hi)
            for col, angles in enumerate(VIEWS):
                sides = self._make_sides()
                actor = rr.vtkActor(mapper=self._make_mapper(sides, field, (lo, hi)))
                actor.property.lighting = False
                renderer = rr.vtkRenderer()
                renderer.SetBackground(0.12, 0.12, 0.16)
                renderer.SetViewport(col / len(VIEWS),
                                     row / len(FIELDS),
                                     (col + 1) / len(VIEWS),
                                     (row + 1) / len(FIELDS))
                renderer.AddActor(actor)
                camera = renderer.GetActiveCamera()
                camera.Roll(angles[0])
                camera.Azimuth(angles[1])
                camera.Elevation(angles[2])
                renderer.ResetCamera()
                window.AddRenderer(renderer)
        window.Render()

        if '-I' in sys.argv:
            import vtkmodules.vtkInteractionStyle
            interactor = window.MakeRenderWindowInteractor()
            window.SetInteractor(interactor)
            interactor.Start()
        else:
            Testing.compareImage(window, Path(Testing.getAbsImagePath(f"{__class__.__name__}.png")).as_posix())

    @staticmethod
    def _fieldRange(cellCoefficients, order):
        """Actual min/max of the field.

        Note this is deliberately not vtkCellGrid::GetCellAttributeRange(),
        which reports the range of the degree-of-freedom array. For a modal
        basis the coefficients are not field values, so that range is far too
        narrow and would clamp most of the color map.
        """
        samples = []
        steps = 12
        for i in range(steps + 1):
            for j in range(steps + 1 - i):
                for k in range(steps + 1 - i - j):
                    samples.append((i / steps, j / steps, k / steps))
        lo, hi = float('inf'), float('-inf')
        for coeffs in cellCoefficients:
            for rst in samples:
                value = sum(c * b for c, b in zip(coeffs, HexGaussBasis(order, *rst)))
                lo, hi = min(lo, value), max(hi, value)
        return lo, hi


if __name__ == "__main__":
    Testing.main([(TestCellGridHexGaussBasis, 'test')])
