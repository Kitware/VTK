import vtk
import numpy
import math

def readDataset(filePath):
    # load dataset
    reader = vtk.vtkXMLImageDataReader()
    reader.SetFileName(filePath)
    reader.Update()
    reader.GetOutput().GetPointData().RemoveArray("ProcessId")
    reader.GetOutput().GetPointData().RemoveArray("vtkValidPointMask")
    reader.GetOutput().GetPointData().RemoveArray("vtkGhostType")

    bounds = numpy.zeros(6);
    reader.GetOutput().GetBounds(bounds)

    dimension = 3
    if abs(bounds[5]-bounds[4]) < 1e-5:
        dimension = 2
        if reader.GetOutput().GetSpacing()[2] == 0:
          reader.GetOutput().SetSpacing(reader.GetOutput().GetSpacing()[0],reader.GetOutput().GetSpacing()[1],1)
    return(reader.GetOutput(), bounds, dimension)


def cutoutPattern(dataset, dimension, position, radius):
    pattern = vtk.vtkImageData()
    pattern.SetOrigin( position )
#    spacing = radius/25
#    pattern.SetSpacing( [spacing,spacing,spacing] )
#    if dimension == 2:
#        pattern.SetExtent( [-25,25,-25,25,0,0] )
#    else:
#        pattern.SetExtent( [-25,25,-25,25,-25,25] )
#    pattern.SetSpacing( [0.02,0.02,0.02] )
#    if dimension == 2:
#      pattern.SetExtent( [-50,50,-50,50,0,0] )
#    else:
#      pattern.SetExtent( [-50,50,-50,50,-50,50] )

    pattern.SetSpacing( dataset.GetSpacing() )
    extent = int(radius/dataset.GetSpacing()[0]+1e-10)

    if dimension == 2:
        pattern.SetExtent( [-extent,extent,-extent,extent,0,0] )
    else:
        pattern.SetExtent( [-extent,extent,-extent,extent,-extent,extent] )
    output = sample(dataset, pattern)
    output.GetPointData().RemoveArray("vtkGhostType")

    return output

def createCoarseDataset(bounds, nx, ny, nz):
    if( (nz == 0 and bounds[5]-bounds[4]>1e-10) or (nz > 0 and bounds[5]-bounds[4]<1e-10) ):
        print("ERROR: dimension of dataset and extent do not match")
        return

    nx = nx-1
    ny = ny-1
    datasetCoarse = vtk.vtkImageData()
    datasetCoarse.SetOrigin( bounds[0], bounds[2], bounds[4] )
    if( nz == 0 ):
        datasetCoarse.SetSpacing( 1./nx*(bounds[1]-bounds[0]), 1./ny*(bounds[3]-bounds[2]), 1 )
    else:
        nz = nz-1
        datasetCoarse.SetSpacing( 1./nx*(bounds[1]-bounds[0]), 1./ny*(bounds[3]-bounds[2]), 1./nz*(bounds[5]-bounds[4]) )
    datasetCoarse.SetExtent( 0, nx, 0, ny, 0, nz )
    return datasetCoarse


def sample(dataset, grid):
    probe = vtk.vtkProbeFilter()
    probe.SetInputData(grid)
    probe.SetSourceData(dataset)
    probe.Update()
    probe.GetOutput().GetPointData().RemoveArray("vtkValidPointMask")
    return probe.GetOutput()


def scaleDataset(dataset, s, nameOfPointData):
  if dataset.GetPointData().GetArray(nameOfPointData).GetNumberOfComponents() == 1:
    for i in range(dataset.GetNumberOfPoints()):
      dataset.GetPointData().GetArray(nameOfPointData).SetTuple1(i,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[0]*s)
  if dataset.GetPointData().GetArray(nameOfPointData).GetNumberOfComponents() == 3:
    for i in range(dataset.GetNumberOfPoints()):
      dataset.GetPointData().GetArray(nameOfPointData).SetTuple3(i,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[0]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[1]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[2]*s)
  if dataset.GetPointData().GetArray(nameOfPointData).GetNumberOfComponents() == 9:
    for i in range(dataset.GetNumberOfPoints()):
      dataset.GetPointData().GetArray(nameOfPointData).SetTuple9(i,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[0]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[1]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[2]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[3]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[4]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[5]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[6]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[7]*s,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[8]*s)
  return dataset


def shiftDataset(dataset, s, nameOfPointData):
  if dataset.GetPointData().GetArray(nameOfPointData).GetNumberOfComponents() == 1:
    for i in range(dataset.GetNumberOfPoints()):
      dataset.GetPointData().GetArray(nameOfPointData).SetTuple1(i,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[0]+s[0])
  if dataset.GetPointData().GetArray(nameOfPointData).GetNumberOfComponents() == 3:
    for i in range(dataset.GetNumberOfPoints()):
      dataset.GetPointData().GetArray(nameOfPointData).SetTuple3(i,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[0]+s[0],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[1]+s[1],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[2]+s[2])
  if dataset.GetPointData().GetArray(nameOfPointData).GetNumberOfComponents() == 9:
    for i in range(dataset.GetNumberOfPoints()):
      dataset.GetPointData().GetArray(nameOfPointData).SetTuple9(i,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[0]+s[0],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[1]+s[1],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[2]+s[2],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[3]+s[3],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[4]+s[4],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[5]+s[5],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[6]+s[6],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[7]+s[7],dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i)[8]+s[8])
  return dataset

# this uses the probe filter to map the rotated values to the original grid. That lets the similarity end up in around 10e8 for vectors. For the others the similarity is higher
def rotateDataset(dataset, angle, nameOfPointData):
    bounds=[0]*6
    dataset.GetBounds(bounds)
    center = [0.5*(bounds[1]+bounds[0]), 0.5*(bounds[3]+bounds[2]), 0.5*(bounds[5]+bounds[4])]

    dataset.SetOrigin(dataset.GetOrigin()[0]-center[0],dataset.GetOrigin()[1]-center[1],dataset.GetOrigin()[2]-center[2])

    trans = vtk.vtkTransform()
    trans.RotateZ(angle*180/math.pi)
    tf = vtk.vtkTransformFilter()
    tf.SetTransform(trans)
    tf.SetInputData(dataset);
    tf.Update()

    result = vtk.vtkImageData()
    result.DeepCopy(dataset)
    result.SetSpacing(dataset.GetSpacing()[0]*(1-1e-10),dataset.GetSpacing()[1]*(1-1e-10),dataset.GetSpacing()[2]*(1-1e-10))
    result = sample(tf.GetOutput(),result)
    result.SetSpacing(dataset.GetSpacing())
    result.SetOrigin(dataset.GetOrigin()[0]+center[0],dataset.GetOrigin()[1]+center[1],dataset.GetOrigin()[2]+center[2])

    if result.GetPointData().GetArray(nameOfPointData).GetNumberOfComponents() == 9:
      array = result.GetPointData().GetArray(nameOfPointData)
      rotMat = numpy.array([[numpy.cos(angle), -numpy.sin(angle),0], [numpy.sin(angle),  numpy.cos(angle),0], [0,0,1]])
      for i in range(dataset.GetNumberOfPoints()):
        value = numpy.array(array.GetTuple(i)).reshape(3,3)
        value = numpy.dot(rotMat,numpy.dot(value,rotMat.transpose()))
        value = value.reshape(9,1)
        array.SetTuple9( i, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], value[8] );

    return result


# if we know that we use only multiples of 90degree, we can make it more precise (10e13) by using the direct index correspondence, but we have to rotate the vectors by hand (like the matrices).
def rotateDatasetExact(dataset, angle, nameOfPointData):

  result = vtk.vtkImageData()
  result.DeepCopy(dataset)

  bounds = numpy.zeros(6)
  result.GetBounds(bounds)
  n = result.GetDimensions()[0]
  m = result.GetDimensions()[1]

  for l in range(result.GetNumberOfPoints()):
    if bounds[5]-bounds[4] < 1e-5:
      i = l % n
      j = l // n
      k = 0
    else:
      i = l % n
      j = (l // m) % n
      k = l // (n*m)
#    print l, i, j, k, i+j*n+k*n*m, (n-1-i)*n+j+k*n*m, (n-1-i)+(n-1-j)*n+k*n*m, i*n+(n-1-j)+k*n*m
    if abs(angle * 2 / math.pi - 0) < 1e-5:
      result.GetPointData().GetArray(nameOfPointData).SetTuple(l,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i+j*n+k*n*m))
    if abs(angle * 2 / math.pi - 1) < 1e-5:
      result.GetPointData().GetArray(nameOfPointData).SetTuple(l,dataset.GetPointData().GetArray(nameOfPointData).GetTuple((n-1-i)*n+j+k*n*m))
    if abs(angle * 2 / math.pi - 2) < 1e-5:
      result.GetPointData().GetArray(nameOfPointData).SetTuple(l,dataset.GetPointData().GetArray(nameOfPointData).GetTuple((n-1-i)+(n-1-j)*n+k*n*m))
    if abs(angle * 2 / math.pi - 3) < 1e-5:
      result.GetPointData().GetArray(nameOfPointData).SetTuple(l,dataset.GetPointData().GetArray(nameOfPointData).GetTuple(i*n+(n-1-j)+k*n*m))

  if result.GetPointData().GetArray(nameOfPointData).GetNumberOfComponents() == 3:
    array = result.GetPointData().GetArray(nameOfPointData)
    rotMat = numpy.array([[numpy.cos(angle), -numpy.sin(angle),0], [numpy.sin(angle),  numpy.cos(angle),0], [0,0,1]])
    for i in range(dataset.GetNumberOfPoints()):
      value = numpy.array(array.GetTuple(i)).reshape(3,1)
      value = numpy.dot(rotMat,value)
      result.GetPointData().GetArray(nameOfPointData).SetTuple3( i, value[0], value[1], value[2] );

  if result.GetPointData().GetArray(nameOfPointData).GetNumberOfComponents() == 9:
    array = result.GetPointData().GetArray(nameOfPointData)
    rotMat = numpy.array([[numpy.cos(angle), -numpy.sin(angle),0], [numpy.sin(angle),  numpy.cos(angle),0], [0,0,1]])
    for i in range(dataset.GetNumberOfPoints()):
        value = numpy.array(array.GetTuple(i)).reshape(3,3)
        value = numpy.dot(rotMat,numpy.dot(value,rotMat.transpose()))
        value = value.reshape(9,1)
        array.SetTuple9( i, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], value[8] );

  return result


def myNorm(tuple):
    norm = 0
    for i in range(len(tuple)):
        norm = norm + tuple[i] * tuple[i]
    return math.sqrt(norm)


def myAbst(x,y):
    z = [0]*3
    for i in range(3):
        z[i] = x[i]-y[i]
    return myNorm(z)
