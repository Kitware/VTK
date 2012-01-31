############################################################
from vtk import *
from vtk.util.misc import vtkGetDataRoot
############################################################

# Create renderer and add actors to it
renderer = vtkRenderer()
renderer.SetBackground( .8, .8 ,.8 )

# Create render window
window = vtkRenderWindow()
window.AddRenderer( renderer )
window.SetSize( 500, 500 )

# Create interactor
interactor = vtkRenderWindowInteractor()
interactor.SetRenderWindow( window )

# Retrieve data root
VTK_DATA_ROOT = vtkGetDataRoot()

# Read from AVS UCD data in binary form
reader = vtkAVSucdReader()
reader.SetFileName( VTK_DATA_ROOT + "/Data/UCD/UCD_00005.inp" )

# Update reader and get mesh cell data
reader.Update()
mesh = reader.GetOutput()
cellData = mesh.GetCellData()

# Create normal vectors
cellData.SetActiveScalars("norme[0]")
normX = cellData.GetScalars()
cellData.SetActiveScalars("norme[1]")
normY = cellData.GetScalars()
n = normX.GetNumberOfTuples()
norm = vtkDoubleArray()
norm.SetNumberOfComponents( 3 )
norm.SetNumberOfTuples( n )
norm.SetName( "norme" )
for i in range( 0, n ):
    norm.SetTuple3( i, normX.GetTuple1( i ), normY.GetTuple1( i ), 0.0 )
cellData.SetVectors( norm )

# Create mapper for wireframe rendering of material fraction #1
cellData.SetActiveScalars("frac_pres[1]")
fractionMapper = vtkDataSetMapper()
fractionMapper.SetInputConnection( reader.GetOutputPort() )
fractionMapper.SetScalarRange( cellData.GetScalars().GetRange() )
fractionMapper.SetScalarModeToUseCellData()
fractionMapper.SetColorModeToMapScalars()
fractionMapper.ScalarVisibilityOn()

# Create wireframe actor and add it to view
wireActor = vtkActor()
wireActor.SetMapper( fractionMapper )
wireActor.GetProperty().SetRepresentationToSurface()
renderer.AddViewProp( wireActor )

# Make multiblock from input mesh
meshMB =  vtkMultiBlockDataSet()
meshMB.SetNumberOfBlocks( 1 )
meshMB.GetMetaData( 0 ).Set( vtkCompositeDataSet.NAME(), "Mesh" )
meshMB.SetBlock( 0, mesh )

# Reconstruct material interface
interface = vtkYoungsMaterialInterface()
interface.SetInput( meshMB )
interface.SetNumberOfMaterials( 2 )
interface.SetMaterialVolumeFractionArray( 0, "frac_pres[1]" )
interface.SetMaterialVolumeFractionArray( 1, "frac_pres[2]" )
interface.SetMaterialNormalArray( 0, "norme" )
interface.SetMaterialNormalArray( 1, "norme" )
interface.UseAllBlocksOn()
interface.Update()

# Create mappers and actors for surface rendering of all reconstructed interfaces
interfaceMapper = vtkDataSetMapper()
interfaceIterator = vtkCompositeDataIterator()
interfaceIterator.SetDataSet( interface.GetOutput() )
interfaceIterator.VisitOnlyLeavesOn()
interfaceIterator.SkipEmptyNodesOn()
interfaceIterator.InitTraversal()
interfaceIterator.GoToFirstItem()
while ( interfaceIterator.IsDoneWithTraversal() == 0 ):
    # Create mapper for leaf node
    print "Creating mapper and actor for object with flat index", interfaceIterator.GetCurrentFlatIndex()
    interfaceMapper.SetInput( interfaceIterator.GetCurrentDataObject() )
    interfaceIterator.GoToNextItem()
    interfaceMapper.ScalarVisibilityOff()
    # Create surface actor and add it to view
    interfaceActor = vtkActor()
    interfaceActor.SetMapper( interfaceMapper )
    interfaceActor.GetProperty().SetColor( 0., 0., 0. )
    renderer.AddViewProp( interfaceActor )

# Start interaction
window.Render()
interactor.Start()
