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
cellData.SetActiveScalars( "norme[0]" )
normX = cellData.GetScalars()
cellData.SetActiveScalars( "norme[1]" )
normY = cellData.GetScalars()
n = normX.GetNumberOfTuples()
norm = vtkDoubleArray()
norm.SetNumberOfComponents( 3 )
norm.SetNumberOfTuples( n )
norm.SetName( "norme" )
for i in range( 0, n ):
    norm.SetTuple3( i, normX.GetTuple1( i ), normY.GetTuple1( i ), 0.0 )
cellData.SetVectors( norm )

# Extract submeshes corresponding to 2 different material Ids
cellData.SetActiveScalars( "Material Id" )
threshold2 = vtkThreshold()
threshold2.SetInput( mesh )
threshold2.SetInputArrayToProcess(0, 0, 0, vtkDataObject.FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes.SCALARS );
threshold2.ThresholdByLower( 2 )
threshold2.Update()
meshMat2 = threshold2.GetOutput()
threshold3 = vtkThreshold()
threshold3.SetInput( mesh )
threshold3.SetInputArrayToProcess(0, 0, 0, vtkDataObject.FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes.SCALARS );
threshold3.ThresholdByUpper( 3 )
threshold3.Update()
meshMat3 = threshold3.GetOutput()

# Make multiblock from extracted submeshes
meshMB =  vtkMultiBlockDataSet()
meshMB.SetNumberOfBlocks( 2 )
meshMB.GetMetaData( 0 ).Set( vtkCompositeDataSet.NAME(), "Material 2" )
meshMB.SetBlock( 0, meshMat2 )
meshMB.GetMetaData( 1 ).Set( vtkCompositeDataSet.NAME(), "Material 3" )
meshMB.SetBlock( 1, meshMat3 )

# Create mapper for both submeshes
matRange = cellData.GetScalars().GetRange()
mat2Mapper = vtkDataSetMapper()
mat2Mapper.SetInput( meshMat2 )
mat2Mapper.SetScalarRange( matRange )
mat2Mapper.SetScalarModeToUseCellData()
mat2Mapper.SetColorModeToMapScalars()
mat2Mapper.ScalarVisibilityOn()
mat2Mapper.SetResolveCoincidentTopologyPolygonOffsetParameters( 0, 1 )
mat2Mapper.SetResolveCoincidentTopologyToPolygonOffset()
mat3Mapper = vtkDataSetMapper()
mat3Mapper.SetInput( meshMat3 )
mat3Mapper.SetScalarRange( matRange )
mat3Mapper.SetScalarModeToUseCellData()
mat3Mapper.SetColorModeToMapScalars()
mat3Mapper.ScalarVisibilityOn()
mat3Mapper.SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 )
mat3Mapper.SetResolveCoincidentTopologyToPolygonOffset()

# Create wireframe actor for one, surface actor for other, and add both to view
wireActor = vtkActor()
wireActor.SetMapper( mat2Mapper )
wireActor.GetProperty().SetRepresentationToWireframe()
renderer.AddViewProp( wireActor )
surfActor = vtkActor()
surfActor.SetMapper( mat3Mapper )
surfActor.GetProperty().SetRepresentationToSurface()
renderer.AddViewProp( surfActor )

cellData.SetActiveScalars("frac_pres[1]")
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
interfaceIterator = vtkCompositeDataIterator()
interfaceIterator.SetDataSet( interface.GetOutput() )
interfaceIterator.VisitOnlyLeavesOn()
interfaceIterator.SkipEmptyNodesOn()
interfaceIterator.InitTraversal()
interfaceIterator.GoToFirstItem()
while ( interfaceIterator.IsDoneWithTraversal() == 0 ):
    idx = interfaceIterator.GetCurrentFlatIndex()
    # Create mapper for leaf node
    print "Creating mapper and actor for object with flat index", idx
    interfaceMapper = vtkDataSetMapper()
    interfaceMapper.SetInput( interfaceIterator.GetCurrentDataObject() )
    interfaceIterator.GoToNextItem()
    interfaceMapper.ScalarVisibilityOff()
    # Create surface actor and add it to view
    interfaceActor = vtkActor()
    interfaceActor.SetMapper( interfaceMapper )
    if ( idx == 2 ):
        interfaceActor.GetProperty().SetColor( 0, 1, 0 )
    else:
        interfaceActor.GetProperty().SetColor( 0, 0, 0 )
    renderer.AddViewProp( interfaceActor )

# Start interaction
window.Render()
interactor.Start()
