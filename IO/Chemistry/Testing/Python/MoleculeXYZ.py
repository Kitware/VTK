""" Tests if vtkXYZMolReader works properly. """
from vtkmodules.vtkCommonExecutionModel import vtkCompositeDataPipeline
from vtkmodules.vtkDomainsChemistry import vtkSimpleBondPerceiver
from vtkmodules.vtkIOChemistry import vtkXYZMolReader2
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestXYZMolReader(Testing.vtkTest):
  timerange = (0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0)

  def createMolecule(self):
    reader = vtkXYZMolReader2()
    reader.SetFileName(VTK_DATA_ROOT + "/Data/nanowireTB23K298.xyz")
    reader.Update()
    return reader

  def testBond(self):
    reader = self.createMolecule()
    mol = reader.GetOutput()
    bonder = vtkSimpleBondPerceiver()
    bonder.SetInputData(mol)
    bonder.SetTolerance(.7)
    bonder.Update()
    mol = bonder.GetOutput()
    self.assertEqual(mol.GetNumberOfAtoms(), 3254)
    self.assertEqual(mol.GetNumberOfBonds(), 16948)
    if(reader.GetOutputInformation(0).Has(vtkCompositeDataPipeline.TIME_STEPS())):
      self.assertEqual(self.timerange, reader.GetOutputInformation(0).Get(reader.GetExecutive().TIME_STEPS()))

  def testChangeTimeStep(self):
    reader = self.createMolecule()
    mol = reader.GetOutput()
    reader.Update()
    reader.UpdateTimeStep(10.0)
    mol = reader.GetOutput()
    self.assertEqual(mol.GetNumberOfAtoms(), 3200)

if __name__ == "__main__":
  Testing.main([(TestXYZMolReader, 'test')])
