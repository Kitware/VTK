# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkInteractionStyle as ii
from vtkmodules import vtkIOCellGrid as io
from vtkmodules import vtkFiltersCellGrid as fc
from vtkmodules import vtkRenderingAnnotation as ra
from vtkmodules import vtkRenderingCore as rr
from vtkmodules import vtkRenderingCellGrid as rg
from vtkmodules import vtkInteractionWidgets as iw

import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

from vtkmodules.test import Testing
import os
import sys

# Register render responder for DG cells:
rg.vtkRenderingCellGrid.RegisterCellsAndResponders()

class Shortcuts:
    TOGGLE_PARAM_COORD_VIZ = 'c'
    TOGGLE_HELP_MENU = 'h'
    RESET = 'r'
    TOGGLE_HOVER_PRINT = 'v'
    TOGGLE_TESSELLATION = 't'

class CellGridInteractorStyle(ii.vtkInteractorStyleTrackballCamera):
    """
    Used to toggle visibility of parametric coordinate, tessellated geometry and show cell information under mouse.
    This interactor style adds capabilities that aid in debugging cell grid rendering. It effectively upgrades the unit test
    into a mini application to view cell grids.
    """

    def __init__(self, rw):
        self._toggles = dict()
        self._actions = dict()

        self._setup_help_menu()

        ren2D = rr.vtkRenderer()
        ren2D.EraseOff()
        ren2D.InteractiveOff()
        rw.AddRenderer(ren2D)

        self.shortcutActor = iw.vtkTextRepresentation()

        self.shortcutActor.SetText(self.get_shortcuts_as_string())
        self.shortcutActor.SetRenderer(ren2D)
        self.shortcutActor.BuildRepresentation()
        self.shortcutActor.SetPosition(0, 0.5)
        self.shortcutActor.GetTextActor().SetTextScaleModeToNone()
        self.shortcutActor.GetTextActor().GetTextProperty().SetBackgroundRGBA(0.1, 0.1, 0.1, 0.2)
        self.shortcutActor.SetShowBorder(True)
        self.shortcutActor.SetBorderColor(0.84313725, 0.75686275, 0.90980392)
        self.shortcutActor.SetCornerRadiusStrength(0.1)
        self.shortcutActor.SetPadding(4)
        self.shortcutActor.GetTextActor().GetTextProperty().SetFontFamilyToCourier()
        self.shortcutActor.GetTextActor().GetTextProperty().SetFontSize(18)
        self.shortcutActor.GetTextActor().GetTextProperty().SetJustificationToLeft()
        self.shortcutActor.SetVisibility(True)
        ren2D.AddActor(self.shortcutActor)

        rg.vtkDGRenderResponder.SetScalarVisualizationOverrideType(self.scalar_vis_types[0][1])
        rg.vtkDGRenderResponder.SetVisualizeTessellation(False)

        self.AddObserver("KeyPressEvent", self.onKeyPress)
        self.AddObserver("MouseMoveEvent", self.onMouseMove)

        self.hoveredCellInfoActor = rr.vtkTextActor()
        self.hoveredCellInfoActor.SetTextScaleModeToNone()
        self.hoveredCellInfoActor.GetTextProperty().SetFontSize(24)
        self.hoveredCellInfoActor.GetTextProperty().SetColor(1, 1, 0)
        ren2D.AddActor(self.hoveredCellInfoActor)

    def _setup_help_menu(self):
        toggle_switch_stringifier = lambda v: "On/Off" if v else "Off/Off"
        self.add_toggle(Shortcuts.TOGGLE_PARAM_COORD_VIZ, "Color by parametric coord.", 0, value_to_str_fn=lambda i: self.scalar_vis_types[i][0])
        self.add_toggle(Shortcuts.TOGGLE_HELP_MENU, "Show/Hide this help menu", True, value_to_str_fn=toggle_switch_stringifier)
        self.add_action(Shortcuts.RESET, "Reset")
        self.add_toggle(Shortcuts.TOGGLE_HOVER_PRINT, "Show/Hide information on hover", False, value_to_str_fn=toggle_switch_stringifier)
        self.add_toggle(Shortcuts.TOGGLE_TESSELLATION, "Show/Hide tessellation", False, value_to_str_fn=toggle_switch_stringifier)

    def add_action(self, key: str, doc: str):
        self._actions.update({key: doc})

    def add_toggle(self, key: str, doc: str, current_value, value_to_str_fn = str):
        self._toggles.update({key: [doc, current_value, value_to_str_fn]})

    def get_toggle_value(self, key: str):
        return self._toggles.get(key)[1]

    def set_toggle_value(self, key: str, value):
        self._toggles.get(key)[1] = value

    def get_shortcuts_as_string(self):
        result = ""
        for key, doc in self._actions.items():
            result += f"{key}: {doc}\n"
        for key, (doc, current, value_to_str_fn) in self._toggles.items():
            result += f"{key}: {doc} [{value_to_str_fn(current)}] \n"
        return result

    @property
    def scalar_vis_types(self):
        return (
            ("NONE", rg.vtkDGRenderResponder.ScalarVisualizationOverrideType.NONE),
            ("R", rg.vtkDGRenderResponder.ScalarVisualizationOverrideType.R),
            ("S", rg.vtkDGRenderResponder.ScalarVisualizationOverrideType.S),
            ("T", rg.vtkDGRenderResponder.ScalarVisualizationOverrideType.T),
            ("L2_NORM_R_S", rg.vtkDGRenderResponder.ScalarVisualizationOverrideType.L2_NORM_R_S),
            ("L2_NORM_S_T", rg.vtkDGRenderResponder.ScalarVisualizationOverrideType.L2_NORM_S_T),
            ("L2_NORM_T_R", rg.vtkDGRenderResponder.ScalarVisualizationOverrideType.L2_NORM_T_R),
        )

    def onMouseMove(self, obj, event):
        interactor = obj.GetInteractor()
        if not self.get_toggle_value(Shortcuts.TOGGLE_HOVER_PRINT):
            self.hoveredCellInfoActor.SetVisibility(False)
            interactor.GetInteractorStyle().OnMouseMove()
            interactor.Render()
            return
        cursor = interactor.GetEventPosition()
        renderer = interactor.FindPokedRenderer(cursor[0], cursor[1])
        selection = dm.vtkSelection()
        renderer.PickProp(cursor[0], cursor[1], 0, selection)
        showInfoText = False
        for i in range(selection.GetNumberOfNodes()):
            node = selection.GetNode(i)
            properties = node.GetProperties()
            selectedIds = node.GetSelectionData().GetArray(0)
            mapper = properties.Get(dm.vtkSelectionNode.PROP()).GetMapper()
            if not isinstance(mapper, rr.vtkCellGridMapper):
                continue
            showInfoText = True
            cellTypes = mapper.GetInputDataObject(0, 0).GetCellTypes()
            cellTypeIdx = properties.Get(dm.vtkSelectionNode.CELLGRID_CELL_TYPE_INDEX())
            caption = f"CELL_TYPE={cellTypes[cellTypeIdx]}\n" +\
                f"CELLGRID_SOURCE_SPECIFICATION_INDEX={properties.Get(dm.vtkSelectionNode.CELLGRID_SOURCE_SPECIFICATION_INDEX())}\n" +\
                f"CELLGRID_CELL_TYPE_INDEX={properties.Get(dm.vtkSelectionNode.CELLGRID_CELL_TYPE_INDEX())}\n" +\
                f"selected_ids={[int(selectedIds.GetValue(i)) for i in range(selectedIds.GetNumberOfValues())]}"
            self.hoveredCellInfoActor.SetInput(caption)
            self.hoveredCellInfoActor.SetDisplayPosition(cursor[0], cursor[1])

        self.hoveredCellInfoActor.SetVisibility(showInfoText)
        interactor.Render()
        interactor.GetInteractorStyle().OnMouseMove()

    def onKeyPress(self, obj, event):
        interactor = obj.GetInteractor()
        key = interactor.GetKeySym().lower()
        if key == Shortcuts.TOGGLE_PARAM_COORD_VIZ:
            self.set_toggle_value(key, (self.get_toggle_value(key) + 1) % len(self.scalar_vis_types))
        elif key == Shortcuts.TOGGLE_HELP_MENU or key == Shortcuts.TOGGLE_HOVER_PRINT or key == Shortcuts.TOGGLE_TESSELLATION:
            self.set_toggle_value(key, not self.get_toggle_value(key))
        elif key == Shortcuts.RESET:
            self.set_toggle_value(Shortcuts.TOGGLE_PARAM_COORD_VIZ, 0)
            self.set_toggle_value(Shortcuts.TOGGLE_TESSELLATION, False)
        vis_type = self.scalar_vis_types[self.get_toggle_value(Shortcuts.TOGGLE_PARAM_COORD_VIZ)][1]
        rg.vtkDGRenderResponder.SetScalarVisualizationOverrideType(vis_type)
        rg.vtkDGRenderResponder.SetVisualizeTessellation(self.get_toggle_value(Shortcuts.TOGGLE_TESSELLATION))
        self.shortcutActor.SetVisibility(self.get_toggle_value(Shortcuts.TOGGLE_HELP_MENU))
        self.shortcutActor.SetText(self.get_shortcuts_as_string())
        interactor.Render()


class TestCellGridRendering(Testing.vtkTest):

    def runCase(self, dataFile, colorArray, imageFile, cell2D = False, angles = (0, 0, 0), colorArrayComponent=-2):
        rh = io.vtkCellGridReader()
        rh.SetFileName(dataFile)
        fh = fc.vtkCellGridComputeSides()
        fh.SetInputConnection(rh.GetOutputPort())
        fh.SetOutputDimensionControl(dm.vtkCellGridSidesQuery.NextLowestDimension)
        fh.PreserveRenderableInputsOff()
        fh.OmitSidesForRenderableInputsOff()
        fh.Update()
        surfOut = fh.GetOutputDataObject(0)
        # Note: For 2-d cells, we create 2 actor/mapper pairs:
        # + one (ai, mi) to show the original cells
        # + one (ah, mh) to show the edge-sides of the surface
        #   manifold (only those edge-sides on the boundary of the mesh).
        # We use separate mappers so the edges can be rendered without
        # scalar coloring.
        if cell2D:
            ah = rr.vtkActor()
            mh = rr.vtkCellGridMapper()
            mh.SetInputConnection(fh.GetOutputPort())
            ah.SetMapper(mh)
            ah.GetProperty().SetLineWidth(4.0)
        ai = rr.vtkActor()
        mi = rr.vtkCellGridMapper()
        if cell2D:
            mi.SetInputConnection(rh.GetOutputPort())
        else:
            mi.SetInputConnection(fh.GetOutputPort())
        if colorArray != None:
            #if cell2D:
            #    mh.ScalarVisibilityOn()
            #    mh.SetScalarMode(rr.VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
            #    mh.SetArrayName(colorArray)
            #    if colorArrayComponent != 0:
            #        mh.SetArrayComponent(colorArrayComponent)
            mi.ScalarVisibilityOn()
            mi.SetScalarMode(rr.VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
            mi.SetArrayName(colorArray)
            mi.SetArrayComponent(colorArrayComponent)
            gg = rh.GetOutputDataObject(0)
            arange = [0, 0]
            gg.GetCellAttributeRange(gg.GetCellAttributeByName(colorArray), colorArrayComponent, arange, True)
            print('test says color range', colorArray, colorArrayComponent, arange)
            # mi.SetScalarRange(*arange)
            mi.SetUseLookupTableScalarRange(0)
            # mi.UseLookupTableScalarRangeOff()
        ai.SetMapper(mi)
        rw = rr.vtkRenderWindow()
        rn = rr.vtkRenderer()
        ri = rw.MakeRenderWindowInteractor()
        ow = iw.vtkCameraOrientationWidget()
        rw.SetInteractor(ri)
        rw.AddRenderer(rn)
        rw.SetWindowName(imageFile.replace("TestCellGridRendering-", "").replace(".png", ""))
        rn.AddActor(ai)
        ow.SetParentRenderer(rn)
        ow.On()
        if cell2D:
            rn.AddActor(ah)
        rn.GetActiveCamera().Roll(angles[0])
        rn.GetActiveCamera().Azimuth(angles[1])
        rn.GetActiveCamera().Elevation(angles[2])
        rn.ResetCamera()
        rw.SetSize(300, 300)
        rw.Render()
        if "-I" in sys.argv:
            rw.SetSize(1920, 1080)
            rs = CellGridInteractorStyle(rw)
            ri.SetInteractorStyle(rs)

            # In interactive mode, it helps to visualize the point IDs for debugging purpose.
            if cell2D:
                inputData = rh.GetOutput()
            else:
                inputData = fh.GetOutput()
            points = inputData.FindAttributes("coordinates").GetArray(0)
            for i in range(points.GetNumberOfTuples()):
                point = points.GetTuple3(i)
                ca = ra.vtkCaptionActor2D()
                ca.SetAttachmentPoint(point)
                ca.SetCaption(f"{i}")
                ca.SetBorder(False)
                ca.GetTextActor().SetTextScaleModeToNone()
                ca.GetCaptionTextProperty().SetFontSize(12)
                ca.GetCaptionTextProperty().SetColor(1, 1, 0)
                rn.AddActor(ca)
            rw.Render()
            ri.Start()
        # else:
            # Testing.compareImage(rw, Testing.getAbsImagePath(imageFile))

    def testCurlVectorComponents(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg')
        testFile = 'TestCellGridRendering-CurlX.png'
        self.runCase(dataFile, 'curl1', testFile, False, angles=(0, 180, -20), colorArrayComponent=0)
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg')
        testFile = 'TestCellGridRendering-CurlY.png'
        self.runCase(dataFile, 'curl1', testFile, False, angles=(0, 180, -20), colorArrayComponent=1)
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg')
        testFile = 'TestCellGridRendering-CurlZ.png'
        self.runCase(dataFile, 'curl1', testFile, False, angles=(0, 180, -20), colorArrayComponent=2)

    def testDivVectorComponents(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg')
        testFile = 'TestCellGridRendering-DivX.png'
        self.runCase(dataFile, 'div1', testFile, False, angles=(0, 180, -20), colorArrayComponent=0)
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg')
        testFile = 'TestCellGridRendering-DivY.png'
        self.runCase(dataFile, 'div1', testFile, False, angles=(0, 180, -20), colorArrayComponent=1)
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg')
        testFile = 'TestCellGridRendering-DivZ.png'
        self.runCase(dataFile, 'div1', testFile, False, angles=(0, 180, -20), colorArrayComponent=2)

    def testDGWdgRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgWedges.dg')
        # Run once with cell coloring turned on:
        testFile = 'TestCellGridRendering-Wedges.png'
        self.runCase(dataFile, 'scalar1', testFile, False, angles=(15, -30, 10))

    def testDGPyrRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgPyramid19.dg')
        # Run once with cell coloring turned on:
        testFile = 'TestCellGridRendering-Pyramid19.png'
        self.runCase(dataFile, 'scalar3', testFile, False, angles=(-15, 20, -60))
        # Run once coloring by a solid color:
        testFile = 'TestCellGridRendering-Pyramid19-uncolored.png'
        self.runCase(dataFile, None, testFile, False, angles=(-10, 10, -30))

    def testDGHexRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg')
        # Run with HCurl coloring turned on:
        testFile = 'TestCellGridRendering-Hexahedra-curl1.png'
        self.runCase(dataFile, 'curl1', testFile, False, angles=(0, 180, -20), colorArrayComponent=2)
        # # Run once with cell coloring turned on:
        testFile = 'TestCellGridRendering-Hexahedra.png'
        self.runCase(dataFile, 'scalar1', testFile, False, angles=(10, 20, 30))
        # Run once coloring by a solid color:
        testFile = 'TestCellGridRendering-Hexahedra-uncolored.png'
        self.runCase(dataFile, None, testFile, False)
        # Run once with quadratic cell coloring turned on:
        testFile = 'TestCellGridRendering-Hexahedra-quadratic.png'
        self.runCase(dataFile, 'quadratic', testFile, False, angles=(10, 20, 30))

    def testDGTetRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgTetrahedra.dg')
        testFile = 'TestCellGridRendering-Tetrahedra.png'
        self.runCase(dataFile, 'scalar2', testFile, False, angles=(10, 20, 20))

    def testDGQuadRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgQuadrilateral.dg')
        testFile = 'TestCellGridRendering-Quadrilateral.png'
        self.runCase(dataFile, 'scalar1', testFile, True, angles=(10, 20, 30))
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgQuadraticQuadrilaterals.dg')
        testFile = 'TestCellGridRendering-Quadrilaterals-quadratic.png'
        self.runCase(dataFile, 'scalar2', testFile, True, angles=(10, -30, 0))

    def testDGTriRendering(self):
        dataFile = os.path.join(VTK_DATA_ROOT, 'Data', 'dgTriangle.dg')
        testFile = 'TestCellGridRendering-Triangles.png'
        self.runCase(dataFile, 'scalar1', testFile, True, angles=(10, 5, -30))

if __name__ == "__main__":
    Testing.main([(TestCellGridRendering, 'test')])
