#
# This example creates a couple of class instances and print them to
# the standard output. No rendering window is created.
#

#
# Do not forget to add the path to the Wrapping/Tcl directory to your 
# TCLLIBPATH environment variable. Use forward slash / instead of \ 
# and quote (") path containing spaces. 
# Also check that the path to your DLL (i.e. your build dir) is also
# in your PATH environment variable.
# 

#
# First we include the Tcl packages which will make available 
# all of the vtk commands to Tcl
#
package require vtkmy

#
# Next we create an instance of vtkBar
#
puts "Create vtkBar object and print it."

vtkBar bar
puts [bar Print]
  
#
# Then we create an instance of vtkBar2
#
puts "Create vtkBar2 object and print it."
  
vtkBar2 bar2
puts [bar2 Print]
  
#
# And we create an instance of vtkImageFoo
#
puts "Create vtkImageFoo object and print it."

vtkImageFoo imagefoo
puts [imagefoo Print]
  
puts "Looks good ?"

#
# Free up any objects we created
#
vtkCommand DeleteAllObjects

#
# Exit the application
#
exit
