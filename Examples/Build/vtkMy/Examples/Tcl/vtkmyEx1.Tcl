# Do not forget to add the path to the Wrapping/Tcl directory to your 
# TCLLIBPATH environment variable. Use forward slash / instead of \. 

package require vtkmy

puts "Create vtkBar object and print it."

vtkBar bar
puts [bar Print]
  
puts "Create vtkBar2 object and print it."
  
vtkBar2 bar2
puts [bar2 Print]
  
puts "Create vtkImageFoo object and print it."

vtkImageFoo imagefoo
puts [imagefoo Print]
  
puts "Looks good ?"
