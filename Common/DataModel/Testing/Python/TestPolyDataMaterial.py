from vtkmodules.vtkCommonDataModel import vtkPolyDataMaterial
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.test import Testing
import os

class TestPolyDataMaterial(Testing.vtkTest):

    def testAPI(self):
        data = vtkPolyData()
        print(vtkPolyDataMaterial.GetTextureURI())
        imagePath = "/my/path/image.png"
        vtkPolyDataMaterial.SetField(data, vtkPolyDataMaterial.GetTextureURI(), imagePath)
        value = vtkPolyDataMaterial.GetField(data, vtkPolyDataMaterial.GetTextureURI())
        self.assertEqual(imagePath, value[0])
        color = (1., 0., 0.)
        defaultColor = (0., 1., 0.)
        value = vtkPolyDataMaterial.GetField(
            data, vtkPolyDataMaterial.GetDiffuseColor(),
            defaultColor)
        self.assertEqual(value, defaultColor)
        vtkPolyDataMaterial.SetField(data, vtkPolyDataMaterial.GetDiffuseColor(),
                                     color, 3)
        value = vtkPolyDataMaterial.GetField(data, vtkPolyDataMaterial.GetDiffuseColor(),
                                             defaultColor)
        self.assertEqual(value, color)


if __name__ == "__main__":
    Testing.main([(TestPolyDataMaterial, 'test')])
