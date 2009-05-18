#!/bin/sh
sed -i -e "s/#include \"stdint.h\"/#include \"vtkType.h\"/" *.h *.cpp
sed -i -e "s/uint8_t/vtkTypeUInt8/g" *.h *.cpp
sed -i -e "s/int8_t/vtkTypeInt8/g" *.h *.cpp
sed -i -e "s/uint16_t/vtkTypeUInt16/g" *.h *.cpp
sed -i -e "s/int16_t/vtkTypeInt16/g" *.h *.cpp
sed -i -e "s/uint32_t/vtkTypeUInt32/g" *.h *.cpp
sed -i -e "s/int32_t/vtkTypeInt32/g" *.h *.cpp
sed -i -e "s/uint64_t/vtkTypeUInt64/g" *.h *.cpp
sed -i -e "s/int64_t/vtkTypeInt64/g" *.h *.cpp
