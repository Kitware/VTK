from vtkmodules.vtkFiltersSources import vtkRandomHyperTreeGridSource
from vtkmodules.vtkIOXML import (
    vtkXMLHyperTreeGridReader,
    vtkXMLHyperTreeGridWriter,
)

# Creating pseudo-random HTG source
source = vtkRandomHyperTreeGridSource()
source.SetDimensions(5, 5, 2)
source.SetSeed(3713971)
source.SetSplitFraction(0.75);

name = "output_TestWriteReadRandomHTGAppendBinaryMode1.htg"

# Creating HTG Writer
writer = vtkXMLHyperTreeGridWriter()
writer.SetInputConnection(source.GetOutputPort())
writer.SetDataSetMajorVersion(1)
writer.SetDataModeToAppended()
writer.SetFileName( name )
print("Writing HTG to {}".format(name))
writer.Update()

# Creating HTG Reader
reader = vtkXMLHyperTreeGridReader()
reader.SetFileName( name )
print("Reading HTG from {}".format(name))
reader.Update()
