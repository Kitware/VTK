#!/usr/bin/env python

# This example was written by Philippe Pebay, 2016
# This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

############################################################
from vtk import *
############################################################

# Number of plane cuts
nCuts = 6

# Contour parameters
nContours = 4
firstvalue = .3
resolution = .6

# Create quadric for implicit function definition
quadric = vtkQuadric()
quadric.SetCoefficients( 36., 64., 144.,
                         0, 0., 0.,
                         -288., -384., -576.,
                         1152. )

# Create hyper tree grid
htGrid = vtkHyperTreeGridSource()
htGrid.SetMaximumLevel( 4 )
htGrid.SetGridSize( 4, 4, 4 )
htGrid.SetGridScale( 2., 1.5, 1. )
htGrid.SetDimension( 3 )
htGrid.SetBranchFactor( 3 )
htGrid.UseDescriptorOff()
htGrid.UseMaterialMaskOn()
htGrid.SetQuadric( quadric )

# Geometry
geometry = vtkHyperTreeGridGeometry()
geometry.SetInputConnection( htGrid.GetOutputPort() )
geometry.Update()
pd = geometry.GetPolyDataOutput()

# Cut planes
cut = [vtkHyperTreeGridPlaneCutter() for _ in xrange( nCuts )]
cut[0].SetPlane( 1., 0., 0., 4. )
cut[1].SetPlane( .866025403784, .5, -.5, 3.96410161514 )
cut[2].SetPlane( .5, .866025403784, -.866025403784, 2.86602540378 )
cut[3].SetPlane( 0., 1., -1., 1. )
cut[4].SetPlane( -.5, 0.866025403784, -.866025403784, -1.13397459622 )
cut[5].SetPlane( -.866025403784, .5, -.5, -2.96410161514 )

# Contour cuts
cd2pd = [vtkCellDataToPointData() for _ in xrange( nCuts )]
contour = [vtkContourFilter() for _ in xrange( nCuts )]
for i in xrange( 0, nCuts ):
    # Plane cutters
    cut[i].SetInputConnection( htGrid.GetOutputPort() )

    # Cell data to point data
    cd2pd[i].SetInputConnection( cut[i].GetOutputPort() )

    # Contours
    contour[i].SetNumberOfContours( nContours )
    contour[i].SetInputConnection( cd2pd[i].GetOutputPort() )
    isovalue = firstvalue
    for j in xrange( 0, nContours ):
        contour[i].SetValue( j, isovalue )
        isovalue += resolution

# Retrieve scalar range on first cut
cut[0].Update()
range = cut[0].GetPolyDataOutput().GetCellData().GetScalars().GetRange()

# Camera
bd = [ 0. ] * 6
pd.GetBounds( bd )
camera = vtkCamera()
camera.SetClippingRange( 1., 100. )
camera.SetFocalPoint( pd.GetCenter() )
camera.SetPosition( -.3 * bd[1], 1.3 * bd[3], -2.9 * bd[5] )

# Rendering pipeline
vtkMapper.SetResolveCoincidentTopologyToPolygonOffset()
mapper = [vtkPolyDataMapper() for _ in xrange( nCuts )]
mapper1 = [vtkPolyDataMapper() for _ in xrange( nCuts )]
mapper2 = [vtkPolyDataMapper() for _ in xrange( nCuts )]
actor = [vtkActor() for _ in xrange( nCuts )]
actor1 = [vtkActor() for _ in xrange( nCuts )]
actor2 = [vtkActor() for _ in xrange( nCuts )]
renderer = vtkRenderer()
renderer.SetActiveCamera( camera )
renderer.SetBackground( 1., 1. ,1. )
for i in xrange( 0, nCuts ):
    # Mappers
    mapper[i].SetInputConnection( contour[i].GetOutputPort() )
    mapper[i].SetScalarRange( range )
    mapper1[i].SetInputConnection( cut[i].GetOutputPort() )
    mapper1[i].SetScalarRange( range )
    mapper2[i].SetInputConnection( cut[i].GetOutputPort() )
    mapper2[i].ScalarVisibilityOff()

    # Actors
    actor[i].SetMapper( mapper[i] )
    actor[i].GetProperty().SetLineWidth( 2 )
    actor1[i].SetMapper( mapper1[i] )
    actor2[i].SetMapper( mapper2[i] )
    actor2[i].GetProperty().SetRepresentationToWireframe()
    actor2[i].GetProperty().SetColor( .7, .7, .7 )

    # Add actors to renderer
    renderer.AddActor( actor[i] )
    renderer.AddActor( actor1[i] )
    renderer.AddActor( actor2[i] )

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
