# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkFiltersCore as ff
from vtkmodules import vtkFiltersCellGrid as fc
from vtkmodules import vtkIOCellGrid as io
from vtkmodules import vtkIOXML as ix
from vtkmodules import vtkIOIOSS as ii
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

from vtkmodules.vtkRenderingCore import *
from vtkmodules.vtkRenderingOpenGL2 import *
from vtkmodules.vtkRenderingCellGrid import *
from vtkmodules.vtkInteractionStyle import *
from vtkmodules.vtkInteractionWidgets import vtkCameraOrientationWidget

# We need vtkIOExodus as otherwise an information key will be missing, generating
# the following warning:
#   Could not locate key vtkExodusIIReader::GLOBAL_TEMPORAL_VARIABLE.
try:
    from vtkmodules import vtkIOExodus as ie
except:
    pass

from vtkmodules.test import Testing
import os

class TestCellGridCopy(Testing.vtkTest):

    def setUp(self):
        reader = io.vtkCellGridReader()
        # reader.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg'))
        reader.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgMixed.dg'))
        reader.UpdateInformation()
        reader.Update()
        self.source = reader.GetOutputDataObject(0)
        # Fetch all the attribute names (which we know are unique for our test data)
        self.allAttributes = [x.GetName().Data() for x in self.source.GetCellAttributeList()]
        # print(self.source)

    def checkNumCells(self, query):
        if query.GetCopyCells():
            print(query.GetTarget().GetNumberOfCells(), 'cells, expected',
                  self.source.GetNumberOfCells(), 'cells')
            ok = query.GetTarget().GetNumberOfCells() == self.source.GetNumberOfCells()
        else:
            print(query.GetTarget().GetNumberOfCells(), 'cells, expected 0 cells')
            ok = query.GetTarget().GetNumberOfCells() == 0
        if not ok:
            raise RuntimeError('Wrong number of cells in copy.')
        return ok

    def checkAttribute(self, query, attName):
        """Check whether two attributes have corresponding arrays for their cell types."""
        sourceAtt = query.GetSource().GetCellAttributeByName(attName)
        targetAtt = query.GetTarget().GetCellAttributeByName(attName)
        if sourceAtt == None or targetAtt == None:
            print('ERROR: Missing attribute', type(sourceAtt), type(targetAtt))
            raise RuntimeError('Missing expected attribute %s' % attName)
            return False
        print('  Check', attName)
        roles = [cc.vtkStringToken(x) for x in ['values', 'connectivity']]
        ok = True
        for cellTypeString in query.GetSource().GetCellTypes():
            cellType = cc.vtkStringToken(cellTypeString)
            print('    Cell', cellTypeString)
            for role in roles:
                sourceArr = sourceAtt.GetArrayForCellTypeAndRole(cellType, role)
                targetArr = targetAtt.GetArrayForCellTypeAndRole(cellType, role)
                sourcePtr = sourceArr.GetVoidPointer(0)
                targetPtr = targetArr.GetVoidPointer(0) if targetArr != None else None
                if not query.GetCopyArrays():
                    # Even when not copying arrays, we do copy the connectivity
                    accept = targetPtr == None or (query.GetCopyCells() and role.Data() == 'connectivity')
                    print('      Role', role.Data(), 'OK' if accept else 'FAIL')
                    ok = ok and accept
                elif query.GetDeepCopyArrays() or not query.GetCopyCells():
                    print('      Role', role.Data(), 'OK' if sourcePtr != targetPtr else 'FAIL')
                    ok = ok and sourcePtr != targetPtr
                else:
                    print('      Role', role.Data(), 'OK' if sourcePtr == targetPtr else 'FAIL')
                    ok = ok and sourcePtr == targetPtr
        return ok

    def checkAttributes(self, query, attributeNames, unexpected):
        print('Checking attributes in copy')
        ok = True
        for attributeName in attributeNames:
            ok = ok and self.checkAttribute(query, attributeName)
        for attributeName in unexpected:
            if query.GetTarget().GetCellAttributeByName(attributeName) != None:
                ok = False
                print('ERROR: Unexpected attribute %s was present in output' % attributeName)
        if not ok:
            raise RuntimeError('Attribute arrays were not copied properly.')

    def testCopyNoArrays(self):
        print('\n## Testing cell-grid copy without arrays.\n')
        qq = dm.vtkCellGridCopyQuery()
        qq.SetSource(self.source)
        qq.SetTarget(dm.vtkCellGrid())
        qq.CopyOnlyShapeOff()
        qq.CopyCellsOn()
        qq.CopyArraysOff()
        qq.DeepCopyArraysOn()
        qq.AddSourceCellAttributeId(self.source.GetShapeAttribute().GetId())
        qq.AddSourceCellAttributeId(self.source.GetCellAttributeByName('scalar1').GetId())
        print('Query', qq)
        ok = self.source.Query(qq)
        print('Query result', ok)
        if not ok:
            print(qq.GetTarget())
            raise RuntimeError('Copy query reported failure.')
        self.checkNumCells(qq)
        self.checkAttributes(qq, ['shape', 'scalar1'], ['scalar0'])

    def testCopyNoArraysAndNoCells(self):
        print('\n## Testing cell-grid copy without arrays or cells.\n')
        qq = dm.vtkCellGridCopyQuery()
        qq.SetSource(self.source)
        qq.SetTarget(dm.vtkCellGrid())
        qq.CopyOnlyShapeOff()
        qq.CopyCellsOff()
        qq.CopyArraysOff()
        qq.DeepCopyArraysOn()
        qq.AddSourceCellAttributeId(self.source.GetShapeAttribute().GetId())
        qq.AddSourceCellAttributeId(self.source.GetCellAttributeByName('scalar1').GetId())
        print('Query', qq)
        ok = self.source.Query(qq)
        print('Query result', ok)
        if not ok:
            print(qq.GetTarget())
            raise RuntimeError('Copy query reported failure.')
        self.checkNumCells(qq)
        self.checkAttributes(qq, ['shape', 'scalar1'], ['scalar0'])

    def testShallowCopy(self):
        print('\n## Testing cell-grid shallow copy.\n')
        target = dm.vtkCellGrid()
        target.ShallowCopy(self.source)
        # Create a query object just to test results.
        qq = dm.vtkCellGridCopyQuery()
        qq.SetSource(self.source)
        qq.SetTarget(target)
        qq.CopyOnlyShapeOff()
        qq.CopyCellsOn()
        qq.DeepCopyArraysOff()
        qq.AddAllSourceCellAttributeIds()
        self.checkNumCells(qq)
        self.checkAttributes(qq, self.allAttributes, [])

    def testDeepCopy(self):
        print('\n## Testing cell-grid deep copy.\n')
        target = dm.vtkCellGrid()
        target.DeepCopy(self.source)
        # Create a query object just to test results.
        qq = dm.vtkCellGridCopyQuery()
        qq.SetSource(self.source)
        qq.SetTarget(target)
        qq.CopyOnlyShapeOff()
        qq.CopyCellsOn()
        qq.DeepCopyArraysOn()
        qq.AddAllSourceCellAttributeIds()
        self.checkNumCells(qq)
        self.checkAttributes(qq, self.allAttributes, [])

    def testPartialDeepCopy(self):
        print('\n## Testing partial cell-grid deep copy.\n')
        # Only copy 2 attributes using the query explicitly.
        qq = dm.vtkCellGridCopyQuery()
        qq.SetSource(self.source)
        qq.SetTarget(dm.vtkCellGrid())
        qq.CopyOnlyShapeOff()
        qq.CopyCellsOff()
        qq.DeepCopyArraysOn()
        qq.AddSourceCellAttributeId(self.source.GetShapeAttribute().GetId())
        qq.AddSourceCellAttributeId(self.source.GetCellAttributeByName('scalar1').GetId())
        print('Query', qq)
        ok = self.source.Query(qq)
        print('Query result', ok)
        if not ok:
            print(qq.GetTarget())
            raise RuntimeError('Copy query reported failure.')
        self.checkNumCells(qq)
        self.checkAttributes(qq, ['shape', 'scalar1'], ['scalar0'])

    def testShapeOnlyCopy(self):
        print('\n## Testing cell-grid shape-only copy.\n')
        qq = dm.vtkCellGridCopyQuery()
        qq.SetSource(self.source)
        qq.SetTarget(dm.vtkCellGrid())
        qq.CopyOnlyShapeOn()
        qq.CopyCellsOn()
        qq.DeepCopyArraysOn()
        qq.AddSourceCellAttributeId(self.source.GetShapeAttribute().GetId())
        qq.AddSourceCellAttributeId(self.source.GetCellAttributeByName('scalar1').GetId())
        print('Query', qq)
        ok = self.source.Query(qq)
        print('Query result', ok)
        if not ok:
            print(qq.GetTarget())
            raise RuntimeError('Copy query reported failure.')
        self.checkNumCells(qq)
        self.checkAttributes(qq, ['shape'], ['scalar1', 'scalar0'])

    def testCopyStructure(self):
        print('\n## Testing cell-grid copy-structure.\n')
        target = dm.vtkCellGrid()
        target.CopyStructure(self.source, True) # copy by reference (shallow)
        # Create a query object just to test results.
        qq = dm.vtkCellGridCopyQuery()
        qq.SetSource(self.source)
        qq.SetTarget(target)
        qq.ResetCellAttributeIds()
        qq.CopyOnlyShapeOn()
        qq.CopyCellsOn()
        qq.DeepCopyArraysOff()
        self.checkNumCells(qq)
        someAttributes = self.allAttributes.copy()
        shapeName = target.GetShapeAttribute().GetName().Data()
        someAttributes.remove(shapeName)
        self.checkAttributes(qq, [shapeName,], someAttributes)

if __name__ == '__main__':
    Testing.main([(TestCellGridCopy, 'test')])
