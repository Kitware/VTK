"""
vtkImageImportFromArray: a NumPy front-end to vtkImageImport

Load a python array into a vtk image.
To use this class,you must have NumPy installed (http://numpy.scipy.org/)

Methods:

  GetOutput() -- connect to VTK image pipeline
  SetArray()  -- set the array to load in

Convert python 'Int' to VTK_UNSIGNED_SHORT:
(python doesn't support unsigned short, so this might be necessary)

  SetConvertIntToUnsignedShort(yesno)
  ConvertIntToUnsignedShortOn()
  ConvertIntToUnsignedShortOff()

Methods from vtkImageImport:
(if you don't set these, sensible defaults will be used)

  SetDataExtent()
  SetDataSpacing()
  SetDataOrigin()
"""

from vtk import vtkImageImport
from vtk import VTK_SIGNED_CHAR
from vtk import VTK_UNSIGNED_CHAR
from vtk import VTK_SHORT
from vtk import VTK_UNSIGNED_SHORT
from vtk import VTK_INT
from vtk import VTK_UNSIGNED_INT
from vtk import VTK_LONG
from vtk import VTK_UNSIGNED_LONG
from vtk import VTK_FLOAT
from vtk import VTK_DOUBLE

class vtkImageImportFromArray:
    def __init__(self):
        self.__import = vtkImageImport()
        self.__ConvertIntToUnsignedShort = False
        self.__Array = None

    # type dictionary: note that python doesn't support
    # unsigned integers properly!
    __typeDict = {'b':VTK_SIGNED_CHAR,     # int8
                  'B':VTK_UNSIGNED_CHAR,   # uint8
                  'h':VTK_SHORT,           # int16
                  'H':VTK_UNSIGNED_SHORT,  # uint16
                  'i':VTK_INT,             # int32
                  'I':VTK_UNSIGNED_INT,    # uint32
                  'l':VTK_LONG,            # int64
                  'L':VTK_UNSIGNED_LONG,   # uint64
                  'f':VTK_FLOAT,           # float32
                  'd':VTK_DOUBLE,          # float64
                  }

    # convert 'Int32' to 'unsigned short'
    def SetConvertIntToUnsignedShort(self,yesno):
        self.__ConvertIntToUnsignedShort = yesno

    def GetConvertIntToUnsignedShort(self):
        return self.__ConvertIntToUnsignedShort

    def ConvertIntToUnsignedShortOn(self):
        self.__ConvertIntToUnsignedShort = True

    def ConvertIntToUnsignedShortOff(self):
        self.__ConvertIntToUnsignedShort = False

    def Update(self):
        self.__import.Update()

    # get the output
    def GetOutputPort(self):
        return self.__import.GetOutputPort()

    # get the output
    def GetOutput(self):
        return self.__import.GetOutput()

    # import an array
    def SetArray(self,imArray):
        self.__Array = imArray
        imTmpArr = imArray.flat
        numComponents = 1
        dim = imArray.shape
        if len(dim) == 0:
            dim = (1,1,1)
        elif len(dim) == 1:
            dim = (1, 1, dim[0])
        elif len(dim) == 2:
            dim = (1, dim[0], dim[1])
        elif len(dim) == 4:
            numComponents = dim[3]
            dim = (dim[0],dim[1],dim[2])

        typecode = imArray.dtype.char

        ar_type = self.__typeDict[typecode]

        if (typecode == 'F' or typecode == 'D'):
            numComponents = numComponents * 2

        if (self.__ConvertIntToUnsignedShort and typecode == 'i'):
            imTmpArr = imArray.astype('h').flat
            ar_type = VTK_UNSIGNED_SHORT
        else:
            imTmpArr = imArray.flat

        size = len(imTmpArr)*self.__sizeDict[type]*numComponents
        self.__import.CopyImportVoidPointer(imTmpArr,size)
        self.__import.SetDataScalarType(ar_type)
        self.__import.SetNumberOfScalarComponents(numComponents)
        extent = self.__import.GetDataExtent()
        self.__import.SetDataExtent(extent[0],extent[0]+dim[2]-1,
                                    extent[2],extent[2]+dim[1]-1,
                                    extent[4],extent[4]+dim[0]-1)
        self.__import.SetWholeExtent(extent[0],extent[0]+dim[2]-1,
                                     extent[2],extent[2]+dim[1]-1,
                                     extent[4],extent[4]+dim[0]-1)

    def GetArray(self):
        return self.__Array

    # a whole bunch of methods copied from vtkImageImport

    def SetDataExtent(self,extent):
        self.__import.SetDataExtent(extent)

    def GetDataExtent(self):
        return self.__import.GetDataExtent()

    def SetDataSpacing(self,spacing):
        self.__import.SetDataSpacing(spacing)

    def GetDataSpacing(self):
        return self.__import.GetDataSpacing()

    def SetDataOrigin(self,origin):
        self.__import.SetDataOrigin(origin)

    def GetDataOrigin(self):
        return self.__import.GetDataOrigin()
