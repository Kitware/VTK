for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtk
vtkTimerLog timer
vtkObject a
a GlobalWarningDisplayOff
a Delete

set exceptions {
vtkLODProp3D-GetPickLODID
vtkObject-GetSuperClassName
vtkPropAssembly-GetBounds
vtkRenderWindow-GetEventPending
vtkSQLiteDatabase-GetQueryInstance
vtkMySQLDatabase-GetQueryInstance
vtkPostgreSQLDatabase-GetQueryInstance
vtkODBCDatabase-GetQueryInstance
vtkUniformVariables-GetCurrentName
vtkXOpenGLRenderWindow-GetEventPending
vtkXMesaRenderWindow-GetEventPending
vtkMPICommunicator-GetWorldCommunicator
vtkMPICommunicator-GetLocalProcessId
vtkMPICommunicator-GetNumberOfProcesses
vtkMPICommunicator-GetMPIComm
vtkOpenGLScalarsToColorsPainter-GetTextureSizeLimit
vtkScalarsToColorsPainter-GetTextureSizeLimit
vtkStructuredGridConnectivity-GetNumberOfGrids
vtkPStructuredGridConnectivity-GetNumberOfGrids
vtkMesaScalarsToColorsPainter-GetTextureSizeLimit
vtkDataObjectTreeIterator-GetCurrentDataObject
vtkDataObjectTreeIterator-GetCurrentMetaData
vtkDataObjectTreeIterator-GetCurrentFlatIndex
}

proc TestOne {cname} {
   global exceptions
   $cname b
   puts "Test $cname"
   set methods [b ListMethods]
   # look for a Get Set pair
   set len [llength $methods]
   for {set i 0} {$i < $len} {incr i} {
      if {[regsub {^Get([A-za-z0-9]*)} [lindex $methods $i] {\1} name]} {
         if {($i == $len - 1) || ($i < $len - 1 && [lindex $methods [expr $i + 1]] != "with")} {
            if {[lsearch $exceptions "$cname-[lindex $methods $i]"] == -1} {
               # invoke the GetMethod
               set tmp [b Get$name]
               # find matching set method
               for {set j 0} {$j < $len} {incr j} {
                  if {[regexp "^Set$name" [lindex $methods $j]]} {
                     if {$j < $len - 3 && [lindex $methods [expr $j + 2]] == "1"} {
                        catch {b Set$name $tmp}
                     }
                     if {$j < $len - 3 && [lindex $methods [expr $j + 2]] > 1} {
                        catch {eval b Set$name $tmp}
                     }
                  }
               }
            }
         }
      }
   }
  # $object DescribeMethods with no arguments returns a list of methods for the object.
  # $object DescribeMethods <MethodName> returns a list containing the following:
  # MethodName {arglist} {description} {c++ signature} DefiningSuperclass
  set Methods [b DescribeMethods]
  # Find the Get methods
  foreach GetMethod [lsearch -inline -all -glob $Methods Get*] {
    # See how many arguments it requires, and only test get methods with 0 arguments
    if { [llength [lindex [b DescribeMethods $GetMethod] 1]] > 0 } { continue }
    # check the exceptions list
    if {[lsearch $exceptions "$cname-$GetMethod"] != -1} { continue }
    set tmp [b $GetMethod]
    set SetMethodSearch Set[string range $GetMethod 3 end]
    foreach SetMethod [lsearch -inline -all $Methods $SetMethodSearch] {
      catch { eval b $SetMethod $tmp }
      catch { b $SetMethod $tmp }
    }
  }

  b Delete
}

set classExceptions {
   vtkCommand
   vtkFileOutputWindow
   vtkIndent
   vtkOutputWindow
   vtkPlanes
   vtkProjectedPolyDataRayBounder
   vtkRayCaster
   vtkTimeStamp
   vtkTkImageViewerWidget
   vtkTkImageWindowWidget
   vtkTkRenderWidget
   vtkImageDataToTkPhoto
   vtkViewRays
   vtkWin32OutputWindow
   vtkWin32ProcessOutputWindow
   vtkXMLFileOutputWindow
   vtkHierarchicalBoxDataIterator
   vtkHierarchicalBoxDataSet
   vtkNonOverlappingAMR
   vtkOverlappingAMR
   vtkStructuredAMRGridConnectivity
   vtkUniformGridAMRDataIterator
   vtkMathTextUtilities
   vtkMatplotlibMathTextUtilities
   vtkTextRenderer
   vtkDataSetCellIterator
   vtkPointSetCellIterator
   vtkUnstructuredGridCellIterator
}

proc rtSetGetTest { fileid } {
   global classExceptions
   set totalTime 0.0
   # for every class
   set all [lsort [info command vtk*]]
   foreach a $all {
      if {[lsearch $classExceptions $a] == -1} {
         # test some set get methods
         timer StartTimer

         TestOne $a

         timer StopTimer
             set elapsedTime [timer GetElapsedTime]
             set totalTime [expr $totalTime + $elapsedTime]

             if { $elapsedTime > 1.0 } {
               puts "Elapsed Time: $elapsedTime and took longer than 1 second."
             }
      }
   }
}

# All tests should end with the following...

puts "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)"
rtSetGetTest stdout

timer Delete

exit
