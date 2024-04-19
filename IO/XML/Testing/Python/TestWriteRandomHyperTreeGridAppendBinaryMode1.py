from vtkmodules.vtkFiltersSources import vtkRandomHyperTreeGridSource
from vtkmodules.vtkIOXML import vtkXMLHyperTreeGridWriter

# Creating pseudo-random HTG source
source = vtkRandomHyperTreeGridSource()
source.SetDimensions(5, 5, 2)
source.SetSeed(3713971)
source.SetSplitFraction(0.75);

name = "output_TestWriteRandomHTGAppendBinaryMode1.htg"

# Creating HTG Writer
writer = vtkXMLHyperTreeGridWriter()
writer.SetInputConnection(source.GetOutputPort())
writer.SetDataSetMajorVersion(1)
writer.SetDataModeToAppended()
writer.SetFileName( name )
writer.Update()
