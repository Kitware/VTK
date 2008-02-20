from vtk import *
from numeric import *

# Define script parameters 

numbvert = 20
edgeprob = 0.50

# Create a random graph

rgsource = vtkRandomGraphSource()
degree = vtkVertexDegree()

# Set number of vertices and the probability of an edge existing between any two vertices

rgsource.SetNumberOfVertices(numbvert)
rgsource.SetUseEdgeProbability(1)
rgsource.SetEdgeProbability(edgeprob)

# Calculate vertex degree of each vertex

degree = vtkVertexDegree()
degree.AddInputConnection(rgsource.GetOutputPort())

# Force pipeline to update output

degree.Update()

# Convert vtk "VertexDegree" array into a numpy data array

nda = getarray(degree.GetOutput().GetVertexData().GetArray("VertexDegree"))

# Compute vertex degree distance from vertex degree mean for each vertex

verdis = numpy.abs(nda[:] - nda.mean())

# Convert the Numpy array "verdis" to a vtkDataArray and display distance from the mean at each vertex

vtkarray = setarray(verdis)
vtkarray.SetName("DegDistance")
degree.GetOutput().GetVertexData().AddArray(vtkarray)

# Layout and display graph 

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(degree.GetOutputPort())
view.SetVertexLabelArrayName("DegDistance")
view.SetVertexLabelVisibility(True)

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)
window.GetInteractor().Start()


