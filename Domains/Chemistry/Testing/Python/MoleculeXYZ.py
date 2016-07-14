""" Tests if vtkXYZMolReader works properly. """
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestXYZMolReader(Testing.vtkTest):
  timerange = (0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0)

  def createMolecule(self):
    reader = vtk.vtkXYZMolReader2()
    reader.SetFileName(VTK_DATA_ROOT + "/Data/nanowireTB23K298.xyz")
    reader.Update()
    return reader

  def test1(self):
    reader = self.createMolecule()
    mol = reader.GetOutput()
    bonder = vtk.vtkSimpleBondPerceiver()
    bonder.SetInputData(mol)
    bonder.SetTolerance(.3)
    bonder.Update()
    mol = bonder.GetOutput()
    self.assertEqual(mol.GetNumberOfAtoms(), 3254)
    self.assertEqual(mol.GetNumberOfBonds(), 16948)
    if(reader.GetOutputInformation(0).Has(vtk.vtkCompositeDataPipeline.TIME_STEPS())):
      self.assertEqual(self.timerange, reader.GetOutputInformation(0).Get(reader.GetExecutive().TIME_STEPS()))

if __name__ == "__main__":
  Testing.main([(TestXYZMolReader, 'test')])
