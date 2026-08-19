# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Verify the GPU evaluation of the vtkDGPyr "G" (HGRAD) basis functions.
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


def PyrGaussBasis(order, rr_, ss, tt):
    """Reference for Filters/CellGrid/Basis/HGrad/PyrGnBasis.h.

    Uses VTK's canonical collapsed-coordinate reference pyramid (see
    vtkDGPyr::Parameters): rr_, ss in [-1, 1], tt in [0, 1], base at
    tt == 0, apex at tt == 1.
    """
    basis = []
    tt_tmp = 1.0 - tt + EPS
    for ii in range(order + 1):
        for jj in range(order - ii + 1):
            for kk in range(order - ii + 1):
                maxjjkk = max(jj, kk)
                basis.append(
                    cc.vtkMath.JacobiPolynomial(kk, 0, 0, rr_ / tt_tmp)
                    * cc.vtkMath.JacobiPolynomial(jj, 0, 0, ss / tt_tmp)
                    * tt_tmp ** maxjjkk
                    * cc.vtkMath.JacobiPolynomial(ii, 2 * maxjjkk + 2, 0, tt))
    return basis


class TestCellGridPyrGaussBasis(Testing.vtkTest):

    dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgPyramids-GBasis.dg')

    def _readCoefficients(self):
        """Per-cell degree-of-freedom vectors, straight out of the file."""
        reader = io.vtkCellGridReader(file_name=self.dataFile)
        reader.Update()
        grid = reader.output
        coeffs = {}
        for name, _ in FIELDS:
            array = grid.GetAttributes('vtkDGPyr').GetArray(name)
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
        """Draw every field from every angle; exercises both pyramids' faces and shading."""
        coeffs = self._readCoefficients()
        tile = 160
        window = rr.vtkRenderWindow(
            size=(tile * len(VIEWS), tile * len(FIELDS)),
            window_name="vtkDGPyr HGRAD 'G' basis",
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
        for k in range(steps + 1):
            tt = k / steps
            # Cross-section of the collapsed-coordinate reference pyramid
            # shrinks from the full [-1, 1] square at the base (tt == 0)
            # to a point at the apex (tt == 1).
            halfwidth = 1.0 - tt
            for i in range(steps + 1):
                rst_r = -halfwidth + 2.0 * halfwidth * i / steps
                for j in range(steps + 1):
                    rst_s = -halfwidth + 2.0 * halfwidth * j / steps
                    samples.append((rst_r, rst_s, tt))
        lo, hi = float('inf'), float('-inf')
        for coeffs in cellCoefficients:
            for rst in samples:
                value = sum(c * b for c, b in zip(coeffs, PyrGaussBasis(order, *rst)))
                lo, hi = min(lo, value), max(hi, value)
        return lo, hi


if __name__ == "__main__":
    Testing.main([(TestCellGridPyrGaussBasis, 'test')])
