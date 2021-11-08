## Repeatable image mathematics

vtkImageMathematics can now be used to perform mathematical operations over more than two images.
Previously, the filter accepted two input connections over two different ports. Changes in this
version remove the need for connecting a second input connection to port 1. Instead, all connections
are now made to input port 0, similar to other repeatable VTK image filters like vtkImageAppend.
Without breaking API, the first input connection is copied to the output and the specific operation
is performed when processing the remaining connections.
