# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Verify the GPU evaluation of the vtkDGTet "G" (HGRAD) basis functions.
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
    ('gauss_point_quintic', 5),
    ('gauss_point_quartic', 4),
    ('gauss_point_cubic', 3),
    ('gauss_point_quadratic', 2),
    ('gauss_point_linear', 1),
]

# (roll, azimuth, elevation) for the montage.
VIEWS = [(0, 0, 0), (0, 90, 0), (0, 180, 0), (0, 270, 0), (0, 0, 85), (0, 0, -85)]

EPS = sys.float_info.epsilon


def TetGaussBasis(order, rr_, ss, tt):
    """Reference for Filters/CellGrid/Basis/HGrad/TetGnBasis.h."""
    basis = []
    for ii in range(order + 1):
        for jj in range(order - ii + 1):
            for kk in range(order - ii - jj + 1):
                basis.append(
                    cc.vtkMath.JacobiPolynomial(ii, 0, 0, 2 * rr_ / (1 - ss - tt + EPS) - 1)
                    * (1 - ss - tt) ** ii
                    * cc.vtkMath.JacobiPolynomial(jj, 2 * ii + 1, 0, 2 * ss / (1 - tt + EPS) - 1)
                    * (1 - tt) ** jj
                    * cc.vtkMath.JacobiPolynomial(kk, 2 * (ii + jj) + 2, 0, 2 * tt - 1))
    return basis


class TestCellGridTetGaussBasis(Testing.vtkTest):

    dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgTetrahedra.dg')

    def _readCoefficients(self):
        """Per-cell degree-of-freedom vectors, straight out of the file."""
        reader = io.vtkCellGridReader(file_name=self.dataFile)
        reader.Update()
        grid = reader.output
        coeffs = {}
        for name, _ in FIELDS:
            array = grid.GetAttributes('vtkDGTet').GetArray(name)
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
            window_name="vtkDGTet HGRAD 'G' basis",
        )

        for row, (field, order) in enumerate(FIELDS):
            lo, hi = self._fieldRange(coeffs[field], order)
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
                value = sum(c * b for c, b in zip(coeffs, TetGaussBasis(order, *rst)))
                lo, hi = min(lo, value), max(hi, value)
        return lo, hi


if __name__ == "__main__":
    Testing.main([(TestCellGridTetGaussBasis, 'test')])
