This package includes information about changes in VTK since last release
and utilities for upgrading to VTK 4.0.

Two pdf files and two perl scripts are included:

FieldDataChanges.pdf:
 This document discusses changes to vtkDataSetAttributes, vtkPointData,
 vtkCellData and vtkFieldData.

AttributeChanges.pdf:
 This document discusses changes to the way VTK handles attributes.
 It focuses on the removal of vtkAttributeData and it's subclasses
 (vtkScalars, vtkVectors, vtkNormals, vtkTCoords, vtkTensors).

DiagAttribute.pl : 
 This script tries to find deprecated attribute data classes and 
 methods and warns the user whenever it finds them. It also suggests
 possible modification to bring code up to date.

UpgradeFrom32.pl:
 This script tries to find deprecated classes and methods and replace 
 them with new classes/methods. Please note that it can not fix all 
 possible problems. However, it should be relatively easy to trace 
 those problems from compilation errors.


Here is the related entry from VTK FAQ at 
http://public.kitware.com/cgi-bin/vtkfaq :

6.7. Changes in VTK since 3.2 

* Changes to vtkDataSetAttributes, vtkFieldData and vtkDataArray: All attributes (scalars, vectors...) are now stored in the field data as vtkDataArray's. vtkDataSetAttributes became a sub-class of vtkFieldData. For backwards compatibility, the interface which allows setting/getting the attributes the old way (by passing in a sub-class of vtkAttributeData such as vtkScalars) is still supported but it will be removed in the future. Therefore, the developers should use the new interface which requires passing in a vtkDataArray to set an attribute. vtkAttributeData and it's sub-classes (vtkScalars, vtkVectors...) will be deprectated in the near future; developers should use vtkDataArray and it's sub-classes instead. We are in the process of removing the use of these classes from vtk filters.

* Subclasses of vtkAttributeData (vtkScalars, vtkVectors, vtkNormals, vtkTCoords, vtkTensors) were removed. As of VTK 4.0, vtkDataArray and it's sub-classes should be used to represent attributes and fields. Detailed description of the changes and utilities for upgrading from 3.2 to 4.0 can be found in the package http://public.kitware.com/VTK/files/Upgrading.zip.

* Improved support for parallel visualization: vtkMultiProcessController and it's sub-classes have been re-structured and mostly re-written. The functionality of vtkMultiProcessController have been re-distributed between vtkMultiProcessController and vtkCommunicator. vtkCommunicator is responsible of sending/receiving messages whereas vtkMultiProcessController (and it's subclasses) is responsible of program flow/control (for example processing rmi's). New classes have been added to the Parallel directory. These include vtkCommunicator, vtkMPIGroup, vtkMPICommunicator, vtkSharedMemoryCommunicator, vtkMPIEventLog... There is now a tcl interpreter which supports parallel scripts. It is called pvtk and can be build on Windows and Unix. Examples for both Tcl and C++ can be found in the examples directories.

* vtkSocketCommunicator and vtkSocketController have been added. These support message passing via BSD sockets. Best used together with input-output ports.

* vtkIterativeClosestPointTransform has been added. This class is an implementation of the ICP algorithm. It matches two surfaces using the iterative closest point (ICP) algorithm. The core of the algorithm is to match each vertex in one surface with the closest surface point on the other, then apply the transformation that modify one surface to best match the other (in a least square sense).

* The SetFileName, SaveImageAsPPM and related methods in vtkRenderWindow have been removed. vtkWindowToImageFilter combined with any of the image writers provides greater functionality.

* Support for reading and writing PGM and JPEG images has been included.



