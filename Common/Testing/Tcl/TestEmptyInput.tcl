for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtktcl
vtkObject a
a GlobalWarningDisplayOff
a Delete


vtkMultiThreader t
t SetGlobalDefaultNumberOfThreads 1
t Delete

# Create empty data sets of each type.
vtkImageData         d1
vtkRectilinearGrid   d2
vtkStructuredGrid    d3
vtkPolyData          d4
vtkUnstructuredGrid  d5



proc TestOne {cname} {
   $cname b 
   if {[catch {b SetInput ""}] == 0} {
      # One is bound to work.
      catch {b SetInput d1}
      catch {b SetInput d2}
      catch {b SetInput d3}
      catch {b SetInput d4}
      catch {b SetInput d5}
      catch {b Update}
   }
   b Delete
}

# Any filter with multiple inputs. ...
set classExceptions {
   vtkCommand
   vtkIndent
   vtkTimeStamp
   vtkTkImageViewerWidget
   vtkTkImageWindowWidget
   vtkTkRenderWidget
   vtkDataSetToDataObjectFilter
   vtkImageToPolyDataFilter
   vtkProbeFilter
   vtkTensorGlyph
}

proc rtSetGetTest { fileid } { 
   global classExceptions
   # for every class
   set all [lsort [info command vtk*]]
   foreach a $all {
      if {[lsearch $classExceptions $a] == -1} {
         # test some set get methods
         #puts "Testing -- $a"
         TestOne $a
      }
   }
}

# All tests should end with the following...

rtSetGetTest stdout

d1 Delete
d2 Delete
d3 Delete
d4 Delete
d5 Delete


exit
