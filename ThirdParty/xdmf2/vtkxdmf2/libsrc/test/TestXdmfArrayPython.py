### Author: Kenneth Leiter
### E-mail: kenneth.leiter@us.army.mil
###
### A simple python test that writes ints and floats to XdmfArrays and then tries to read them.  Compares the
### values for equality and tests the accuracy of some convenience functions


import Xdmf
from Xdmf import *

if __name__ == '__main__':   
    
##
## First Part = XDMF_INT64_TYPE Array
##
    intArray = Xdmf.XdmfArray()

    intArray.SetNumberType(Xdmf.XDMF_INT64_TYPE)
    assert(intArray.GetNumberType() == Xdmf.XDMF_INT64_TYPE)
    intArray.SetShapeFromString("3 3")
    assert(intArray.GetShapeAsString() == "3 3")
    assert(intArray.GetNumberOfElements() == 9)
    toWrite = [0, 1, 3, 4, 500, -5000, 500000, 9223372036854775807, -9223372036854775807]
    i = 0;
    for element in toWrite:
        intArray.SetValueFromInt64(i,element)
        i += 1
    
    i = 0;
    for element in toWrite:
        assert(intArray.GetValueAsInt64(i) == element)
        i += 1
    
    assert (intArray.GetMaxAsInt64() == 9223372036854775807)
    assert (intArray.GetMinAsInt64() == -9223372036854775807)

##
## Second Part = XDMF_FLOAT64_TYPE Array
##
    floatArray = Xdmf.XdmfArray()

    floatArray.SetNumberType(Xdmf.XDMF_FLOAT64_TYPE)
    assert(floatArray.GetNumberType() == Xdmf.XDMF_FLOAT64_TYPE)
    floatArray.SetShapeFromString("2 2 2")
    assert(floatArray.GetShapeAsString() == "2 2 2")
    assert(floatArray.GetNumberOfElements() == 8)
    toWrite = [0, -1, 1100.256, 1.1, 1000.50 , 5.6234567, -60.2, 60.25659]
    i = 0;
    for element in toWrite:
        floatArray.SetValueFromFloat64(i,element)
        i += 1
    
    i = 0;
    for element in toWrite:
        assert(floatArray.GetValueAsFloat64(i) == element)
        i += 1

    assert (floatArray.GetMaxAsFloat64() == 1100.256)
    assert (floatArray.GetMinAsFloat64() == -60.2)