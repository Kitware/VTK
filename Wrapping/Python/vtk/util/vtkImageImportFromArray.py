"""
vtkImageImportFromArray: a NumPy front-end to vtkImageImport

Load a Numeric Python array into a VTK image.
To use this class, you must have the LLNL Numeric Python distribution
(http://numpy.sf.net)

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

import Numeric
from vtk import vtkImageImport
from vtkConstants import *

_NEW_NUMERIC = 0
try:
    val = float(Numeric.__version__)
except ValueError:
    _NEW_NUMERIC = 0
else:
    if val > 20.0:
        _NEW_NUMERIC = 1
    else:
        _NEW_NUMERIC = 0


class vtkImageImportFromArray:
    def __init__(self):
        self.__import = vtkImageImport()
        self.__ConvertIntToUnsignedShort = 0
        self.__Array = None

    # type dictionary: note that python doesn't support
    # unsigned integers properly!
    __typeDict = {'c':VTK_UNSIGNED_CHAR,
                  'b':VTK_UNSIGNED_CHAR,
                  '1':VTK_CHAR,
                  's':VTK_SHORT,
                  'i':VTK_INT,
                  'l':VTK_LONG,
                  'f':VTK_FLOAT,
                  'd':VTK_DOUBLE,
                  'F':VTK_FLOAT,
                  'D':VTK_DOUBLE }

    __sizeDict = { VTK_CHAR:1,
                   VTK_UNSIGNED_CHAR:1,
                   VTK_SHORT:2,
                   VTK_UNSIGNED_SHORT:2,
                   VTK_INT:4,
                   VTK_FLOAT:4,
                   VTK_DOUBLE:8 }

    # convert 'Int32' to 'unsigned short'
    def SetConvertIntToUnsignedShort(self,yesno):
        self.__ConvertIntToUnsignedShort = yesno

    def GetConvertIntToUnsignedShort(self):
        return self.__ConvertIntToUnsignedShort
    
    def ConvertIntToUnsignedShortOn(self):
        self.__ConvertIntToUnsignedShort = 1

    def ConvertIntToUnsignedShortOff(self):
        self.__ConvertIntToUnsignedShort = 0

    # get the output
    def GetOutput(self):
        return self.__import.GetOutput()

    # import an array
    def SetArray(self,imArray):
        self.__Array = imArray
        numComponents = 1
        dim = imArray.shape

        if (len(dim) == 4):
            numComponents = dim[3]
            dim = (dim[0],dim[1],dim[2])
            
        type = self.__typeDict[imArray.typecode()]

        if (imArray.typecode() == 'F' or imArray.typecode == 'D'):
            numComponents = numComponents * 2

        if (self.__ConvertIntToUnsignedShort and imArray.typecode() == 'i'):
            if _NEW_NUMERIC:
                imTmpArr = imArray.astype(Numeric.Int16).flat
            else:
                imTmpArr = imArray.astype(Numeric.Int16).tostring()
            type = VTK_UNSIGNED_SHORT
        else:
            if _NEW_NUMERIC:
                imTmpArr = imArray.flat
            else:
                imTmpArr = imArray.tostring()

        size = len(imTmpArr)*self.__sizeDict[type]
        self.__import.CopyImportVoidPointer(imTmpArr, size)
        self.__import.SetDataScalarType(type)
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
    
