for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtk
vtkObject a
a GlobalWarningDisplayOff
a Delete


vtkPolyData emptyPD
vtkImageData emptyID
vtkStructuredGrid emptySG
vtkUnstructuredGrid emptyUG
vtkRectilinearGrid emptyRG

proc TestOne {cname} {

   $cname b 

    if {[b IsA "vtkSource"]} {
	catch {b SetInput emptyPD}
	catch {b Update}
	catch {b SetInput emptyID}
	catch {b Update}
	catch {b SetInput emptySG}
	catch {b Update}
	catch {b SetInput emptyUG}
	catch {b Update}
	catch {b SetInput emptyRG}
	catch {b Update}
    }

   b Delete
}

set classExceptions {
    vtkCommand
    vtkIndent
    vtkTimeStamp
    vtkTkImageViewerWidget
    vtkTkImageWindowWidget
    vtkTkRenderWidget
    vtkImageDataToTkPhoto
    vtkJPEGReader
    vtkWin32VideoSource
}

proc rtTestEmptyInputTest { fileid } { 
   global classExceptions
   # for every class
   set all [lsort [info command vtk*]]
   foreach a $all {
      if {[lsearch $classExceptions $a] == -1} {
         # test some set get methods
         puts  -nonewline "Testing -- $a - " 
         flush stdout
         TestOne $a
         puts "done"
      }
   }
   puts "All Passed"
}





rtTestEmptyInputTest stdout

emptyPD Delete
emptyID Delete
emptySG Delete
emptyUG Delete
emptyRG Delete

exit 


