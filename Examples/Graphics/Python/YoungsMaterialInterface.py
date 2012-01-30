############################################################
from vtk import *
from vtk.util.misc import vtkGetDataRoot
############################################################

# Retrieve data root
VTK_DATA_ROOT = vtkGetDataRoot()

# Read from AVS UCD data in binary form
reader = vtkAVSucdReader()
reader.SetFileName( VTK_DATA_ROOT + "/Data/UCD/UCD_00005.inp" )

# Update reader and get cell data
reader.Update()
cellData = reader.GetOutput().GetCellData()
cellData.SetActiveScalars("frac_pres[1]")

# Create mapper for wireframe rendering of volume fraction #1
mapper = vtkDataSetMapper()
mapper.SetInputConnection( reader.GetOutputPort() )
mapper.SetScalarRange( cellData.GetScalars().GetRange() )
mapper.SetScalarModeToUseCellData()
mapper.SetColorModeToMapScalars()
mapper.ScalarVisibilityOn()

# Create actor
actor = vtkActor()
actor.SetMapper( mapper )
actor.GetProperty().SetRepresentationToWireframe()

# Create renderer
renderer = vtkRenderer()
renderer.AddViewProp( actor )
renderer.SetBackground( .3, .4 ,.5 )

# Create render window
window = vtkRenderWindow()
window.AddRenderer( renderer )
window.SetSize( 500, 500 )

# Create interactor
interactor = vtkRenderWindowInteractor()
interactor.SetRenderWindow( window )

# Start interaction
window.Render()
interactor.Start()
