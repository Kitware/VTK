############################################################
from vtk import *
from vtk.util.misc import vtkGetDataRoot
############################################################

# Retrieve data root
VTK_DATA_ROOT = vtkGetDataRoot()

# Read from AVS UCD data in binary form
reader = vtkAVSucdReader()
reader.SetFileName( VTK_DATA_ROOT + "/Data/UCD/UCD_00005.inp" )

# Update reader and get mesh and cell data
reader.Update()
mesh = reader.GetOutput()
cellData = mesh.GetCellData()
cellData.SetActiveScalars("frac_pres[1]")

# Create mapper for wireframe rendering of material fraction #1
fractionMapper = vtkDataSetMapper()
fractionMapper.SetInputConnection( reader.GetOutputPort() )
fractionMapper.SetScalarRange( cellData.GetScalars().GetRange() )
fractionMapper.SetScalarModeToUseCellData()
fractionMapper.SetColorModeToMapScalars()
fractionMapper.ScalarVisibilityOn()

# Create wireframe actor
wireActor = vtkActor()
wireActor.SetMapper( fractionMapper )
wireActor.GetProperty().SetRepresentationToWireframe()

# Male multiblock from input mesh
meshMB =  vtkMultiBlockDataSet()
meshMB.SetNumberOfBlocks( 1 )
meshMB.GetMetaData( 0 ).Set( vtkCompositeDataSet.NAME(), "Mesh" )
meshMB.SetBlock( 0, mesh )

# Reconstruct material interface
interface = vtkYoungsMaterialInterface()
interface.SetInput( meshMB )

# Create mappeyr for surface rendering of reconstructed interface
interfaceMapper = vtkDataSetMapper()
interfaceMapper.SetInputConnection( interface.GetOutputPort() )

# Create surface actor
surfActor = vtkActor()
surfActor.SetMapper( fractionMapper )
surfActor.GetProperty().SetRepresentationToSurface()

# Create renderer and add actors to it
renderer = vtkRenderer()
renderer.AddViewProp( wireActor )
renderer.AddViewProp( surfActor )
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
