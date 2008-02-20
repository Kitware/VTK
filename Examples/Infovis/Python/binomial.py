from vtk import *
from numeric import *
import pylab

# Define script parameters 

numbvert = 2000
edgeprob = 0.10
histbins = 100

rgsource = vtkRandomGraphSource()

# Set number of vertices and the probability of an edge existing between any two vertices

rgsource.SetNumberOfVertices(numbvert)
rgsource.SetUseEdgeProbability(1)
rgsource.SetEdgeProbability(edgeprob)

# Calculate vertex degree, result is stored in an array named "VertexDegree"

degree = vtkVertexDegree()
degree.AddInputConnection(rgsource.GetOutputPort())

# Force pipeline to update output

degree.Update()

# Convert "VertexDegree" array into a numpy data array

nda = getarray(degree.GetOutput().GetVertexData().GetArray("VertexDegree"))

bnda = numpy.random.binomial(n = numbvert, p = edgeprob, size = nda.size)

# Create histograms from random graph vertex degree and binomial distribution

(bn, bbins) = numpy.histogram(bnda, bins = histbins, normed=1)
(n, bins) = numpy.histogram(nda, bins = histbins,normed=1)

p1 = pylab.bar(bins, n, color = 'r')
p2 = pylab.bar(bbins, bn, color = 'b')

pylab.legend((p1[0],p2[0]), ('graph vertex degree', 'binomial distribution'))

pylab.ylabel('Normalized Histogram Count')
pylab.xlabel('Vertex Degree Bin')
pylab.title('Normalized Histograms')

# Layout and display graph 

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(degree.GetOutputPort())

window = vtkRenderWindow()
window.SetSize(600, 600)
view.SetupRenderWindow(window)

pylab.show()
window.GetInteractor().Start()

