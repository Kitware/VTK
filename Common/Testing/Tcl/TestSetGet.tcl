for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtktcl
vtkObject a
a GlobalWarningDisplayOff
a Delete

set exceptions {
vtkAssembly-GetNextPath
vtkImageExport-GetDataMemorySize
vtkImageExport-GetDataDimensions
vtkImageExport-GetDataNumberOfScalarComponents
vtkImageExport-GetDataExtent
vtkImageExport-GetDataSpacing
vtkImageExport-GetDataOrigin
vtkImageExport-GetDataScalarType
vtkLODProp3D-GetPickLODID
vtkObject-GetSuperClassName
vtkPropAssembly-GetNextPath
vtkPropAssembly-GetBounds
vtkRayCaster-GetParallelStartPosition
vtkRayCaster-GetParallelIncrements
vtkRenderWindow-GetEventPending
vtkSubPixelPositionEdgels-GetGradMaps
vtkViewRays-GetParallelStartPosition
vtkXOpenGLRenderWindow-GetEventPending
vtkMPICommunicator-GetWorldCommunicator
}

proc TestOne {cname} {
   global exceptions
   $cname b 
   set methods [b ListMethods]
   # look for a Get Set pair
   set len [llength $methods]
   for {set i 0} {$i < $len} {incr i} {
      if {[regsub {^Get([A-za-z0-9]*)} [lindex $methods $i] {\1} name]} {
         if {($i == $len - 1) || ($i < $len - 1 && [lindex $methods [expr $i + 1]] != "with")} {
            if {[lsearch $exceptions "$cname-[lindex $methods $i]"] == -1} {
               # invoke the GetMethod
	       #puts "  Invoking Get$name"
               set tmp [b Get$name]
               # find matching set method
               for {set j 0} {$j < $len} {incr j} {
                  if {[regexp "^Set$name" [lindex $methods $j]]} {
                     if {$j < $len - 3 && [lindex $methods [expr $j + 2]] == "1"} {
                        #puts "    Invoking Set$name"
                        catch {b Set$name $tmp}
                     }
                     if {$j < $len - 3 && [lindex $methods [expr $j + 2]] > 1} {
                        #puts "    Invoking Set$name"
                        catch {eval b Set$name $tmp}
                     }
                  }
               }
            }
         }
      }
   }
   b Delete
}

set classExceptions {
   vtkCommand
   vtkFileOutputWindow
   vtkIndent
   vtkOutputWindow
   vtkParallelFactory
   vtkPlanes
   vtkProjectedPolyDataRayBounder
   vtkTimeStamp
   vtkTkImageViewerWidget
   vtkTkImageWindowWidget
   vtkTkRenderWidget
   vtkWin32OutputWindow
   vtkXMLFileOutputWindow
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

exit
