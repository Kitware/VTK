catch {load vtktcl}
# Check all the set methods to see if resetting a variable
# changes the modified time.

# Use List methods to find SetMethods.
# This script records an error when the mtime changes when it should not.
# i.e. "SetIVarValue 1" followed by "SetIVarValue 1"
# It does not find errors of MTime not changing (alot of exceptions)
# It does not change IVars of sub objects (PointLocator Resolution ...)
# It does not check methods with no arguments

# get the interactor ui
source ../examplesTcl/vtkInt.tcl



# A global variable similar to "DebugOn"
# When this is set to 1, the method calls will be echoed, and
# the mtime results displayed.  When an error is encountered,
# an interactor will popup.
set DEBUG 0






proc debug {} {
   global CONTINUE

   set CONTINUE 0

   wm deiconify .vtkInteract
   while { $CONTINUE == 0 } {
      update
   }
   wm withdraw .vtkInteract
}





# finds all objects by doing an "ls"
proc TestKit {kit} {
   global testObject

   #puts "Kit: $kit"
    set inFiles [lsort [glob -nocomplain ../$kit/vtk*.h]]
   #set inFiles {imaging/vtkImageResample.h}

   foreach f $inFiles {
      set objectClass [file rootname [file tail $f]]
      TestObject $kit $objectClass
   }
}

proc TestObject {kit objectClass} {
   global DEBUG

   if {$objectClass == "vtkOutputPort" || $objectClass == "vtkAssemblyNode"} {
     return
   }
   if {$objectClass == "vtkMatrixToHomogeneousTransform" || $objectClass == "vtkGhostLevels"} {
     return
   }
   if {$objectClass == "vtkIndent" || $objectClass == "vtkTimeStamp"} {
     return
   }
   if {$objectClass == "vtkMatrixToLinearTransform" || $objectClass == "vtkOutputWindow"} {
     return
   }

   if {$DEBUG == 1} {puts "    ----------------Object: $objectClass"}

   # just return if this object is not in the kit
   if {[catch {set pathList [glob "../$kit/$objectClass*"]}] != 0} {
      if {$DEBUG} {puts "could not find! $objectClass in $kit"}
      return
   }

   # This checks all the objects (not just sources)
   # (unlike CheckModifyTime3.tcl)

   # make sure we can actualy create the object
   set objectName [new $objectClass]
   if { $objectName == ""} {
      #puts "    --- Could not create object in class $objectClass !!!"
      return
   }

   set methodList [$objectName ListMethods]
   set idx 0
   set str [lindex $methodList $idx]
   while {$str != ""} {
      if { $str == "Methods"} {
	 # remember which superclass defined this method
	 set idx [expr $idx + 2]
	 set methodClass [string trim [lindex $methodList $idx] ":"]
      }
      if {$DEBUG && 0} {puts "look at: $str"}
      if {[string range $str 0 2] == "Set"} {
	 regsub "\{" $str " " str
	 # we found a Set method
	 set methodName [lindex $str 0]
	 # check to see if the Method has arguments
	 if { [lindex $methodList [expr $idx + 1]] == "with" } {
	    # skip to the word with the number of arguments
	    set idx [expr $idx + 2]
	    set numberOfArgs [lindex $methodList $idx]
	    # place idx at end of line so next step will get a new line.
	    incr idx
	 } else {
	    set numberOfArgs 0
	 }
	 # Test this set method
	 TestMethod $methodName $numberOfArgs $methodClass $kit $objectName
      }
      incr idx
      set str [lindex $methodList $idx]
   }

   if {$DEBUG} {puts "$objectName Delete"}
   $objectName Delete
}




proc TestMethod {methodName numberOfArgs methodClass kit objectName} {
   global ERROR_LOG_FD DEBUG

   #puts "        Method: $methodName with $numberOfArgs args"

   if {[CheckException $methodName]} {
      return
   }

   if {$DEBUG} {
      puts "     Method: $methodName"
   }

   if { $numberOfArgs == 0} {
      set argTypes ""
      return
   } else {
      set argTypes [GetArgTypes $methodName $numberOfArgs $methodClass $kit]
      # error checking
      if { $argTypes == ""} {
	 return
      }
   }

   # sanity check
   if { [llength $argTypes] != $numberOfArgs} {
      #puts "-- Could not find arg types for method $methodName"
      #debug
      return
   }

   #puts "                 args [llength $argTypes] : $argTypes"

   if {[catch {set modifyTime0 [$objectName GetMTime]}] != 0} {
      return
   }

   # third call
   set argValues3 [GetArgValues $argTypes 2 $kit]
   set call3 "$objectName $methodName $argValues3"
   if {$DEBUG} { puts "  $call3; $objectName GetMTime"}
   if { [catch {eval $call3}] != 0} {
      return
   }
   if {[catch {set modifyTime3 [$objectName GetMTime]}] != 0} {
      return
   }
   if {$DEBUG} { puts "        = $modifyTime3"}

   # forth call (reset test)
   if {$DEBUG} { puts "  $call3; $objectName GetMTime"}
   if { [catch {eval $call3}] != 0} {
      return
   }
   if {[catch {set modifyTime4 [$objectName GetMTime]}] != 0} {
      return
   }
   if {$DEBUG} { puts "        = $modifyTime4"}

   if { $modifyTime3 != $modifyTime4} {
      puts "Reset Modify Time Error in $methodClass $methodName"
      if {$DEBUG} { 
	 puts "------------------ reset error -------------------------------"
	 puts "MTime changed : ------------------------"
	 puts "MTime: $modifyTime3, $modifyTime4"  
	 puts " method class: $methodClass"
	 debug
      }
   }

   DeleteArgValues $argValues3
}


# I am not going to worry about deleting old objects created as arguments.
proc GetArgValues {argTypes val kit} {

   # create an empty list
   if { [llength $argTypes] == 0} {
      return {}
   }

   set count -1
   foreach type $argTypes {
      incr count
      switch $type {
	 "int" {lappend argValues [expr $val + $count]} 
	 "float" {lappend argValues [expr $val + $count]} 
	 "short" {lappend argValues [expr $val + $count]} 
	 "unsigned char" {lappend argValues [expr $val + $count]} 
	 "char *" {
	    if {$val == 2} {
	       lappend argValues "c$val"
	    } else {
	       lappend argValues "a$val"
	    } 
	 }
	 "default" {
	    if { [string range $type 0 2] != "vtk"} {
	       # puts "--- Cannot handle type:$type"
	       # debug
	       return ""
	    }
	    # this must be an object
	    set argName [ConcreteNew $type $kit]
	    if { $argName == ""} {
	       #puts "--- Could not create arg of type $type !!!"
	       # debug
	       lappend argValues {}
	    } else {
	       # puts "            -Create concrete object ($argName) of class ($type)"
	       lappend argValues $argName
	    }
	 }
      }
   }
   return $argValues
}

proc DeleteArgValues {argValues} {
   global DEBUG

   foreach arg $argValues {
      if { [string range $arg 0 2] == "vtk"} {
	 # It is a vtk object. Delete it..
	 if {$DEBUG} {puts "$arg Delete"}
	 $arg Delete
      }
   }
}

# This is going to be a complex procedure.  It has to look in the header file
# to see what type of arguments a method takes.  Specifically if the 
# set method takes another object as an argument.  
proc GetArgTypes {methodName numberOfArgs methodClass kit} {
   # first look in the hints file.
   # (why aren't there any set methods in the hint file?)
   set fd [open ../wrap/hints]
   set str [getline $fd]
   while {$str != ""} {
      if {$methodClass == [lindex $str 0] &&
	  $methodName == [lindex $str 1] &&
	  $numberOfArgs == [lindex $str 3]} {
	 # found the method in the hints file
	 switch [lindex $str 2] {
	    "301" {set type "float"}
	    "304" {set type "int"}
	    "313" {set type "unsigned char"}
	    "default" {
	       # puts "Can not find type for [lindex $str 2] in hints file"
	       close $fd
	       return ""
	    }
	 }
	 for {set idx 0} { $idx < $numberOfArgs} {incr idx} {
	    lappend argTypes $type
	 }
	 close $fd
	 return $argTypes
      }
      set str [getline $fd]
   }
   close $fd

   # now look in the header file.
   set fileName "../$kit/$methodClass.h"
   if { ! [file exists $fileName] } {
      # could not find header in kit.  Check Common.
      set fileName "common/$methodClass.h"
      if { ! [file exists $fileName] } {
      # could not find header in common.  Check graphics.
	 set fileName "graphics/$methodClass.h"
	 if { ! [file exists $fileName] } {
	    # puts "--Could not find $methodClass.h."
	    return ""
	 }
      }
   }

   set fd [open $fileName]
   set str [getline $fd]
   while { $str != ""} {
      set firstIdx [string first $methodName $str]
      if { $firstIdx != -1} {
	 set idx [expr [string length $methodName] + $firstIdx]
	 if {[string index $str $idx] == "("} {
	    # we found a prototype with the method name in it.
	    # extract the argument types
	    set argTypes [GetArgTypesFromPrototype $str]
	    # if the prototype has the correct number of arguments
	    if { [llength $argTypes] == $numberOfArgs} {
	       close $fd
	       return $argTypes
	    }
	 }
      } 
      # check for macros ...

      set str [getline $fd]
   }
   close $fd

   # puts "            -- Could not find method $methodName $numberOfArgs args in $fileName"

   return ""
}


proc getline { fd } {
    # read and return the next non-blank line from fd
    set s [string trim [gets $fd]]
    while {$s == "" && ![eof $fd]} {
	set s [string trim [gets $fd]]
    }
    ## puts "<$s"
    return $s
}

# this procedure creates a list of arguement types from
# a method prototype.
proc GetArgTypesFromPrototype {prototype} {

   # puts "           GetArgTypesFromPrototype: $prototype"

   # inline bodies can match (so many darn special cases)
   if {[string index $prototype 0] == "\{" } {
      return {}
   }

   set idx [string first "(" $prototype]
   if { $idx == -1} {
      # puts "------- Could not parse prototype: $prototype"
      return {}
   }

   # remove everything but the arguments
   set str [string range $prototype [expr $idx + 1] end]
   set idx [string first ")" $str]
   set str [string range $str 0 [expr $idx - 1]]
  
   # special case: no args
   if { [string length $str] == 0} {
      return {}
   }

   # Ok, I will write my own split, that behaves exactly the way I want!!!!
   set idx [string first "," $str]
   while { $idx != -1} {
      set argDef [string range $str 0 [expr $idx - 1]]
      set str [string range $str [expr $idx + 1] end]
      set type [GetTypeFromArgDef $argDef] 
      # error handling
      if { $type == ""} {
	 return {}
      }
      lappend argTypes $type
      # move to the next arg
      set idx [string first "," $str]
   }

   set type [GetTypeFromArgDef $str] 
   # error handling
   if { $type == ""} {
      return ""
   }
   lappend argTypes $type
   
   return $argTypes
}



# This procedure is given an argument definition (the text the defines
# one argument in a method prototype), and extracts the type.
# It could be more intelligent because I know most types, and objects
# have * in front ...
proc GetTypeFromArgDef {argDef} {
   # prune off varible name (I assume all have one!)
   set argDef [string trim $argDef]

   set word1 [lindex $argDef 0]
   set word2 [lindex $argDef 1]

   # get rid of & if one exists
   set idx [expr [string length $word1] - 1]
   if {[string index $word1 $idx] == "&"} {
      # just abort
      return ""
   }
   # get rid of & if one exists
   set idx [expr [string length $word2] - 1]
   if {[string index $word2 $idx] == "&"} {
      # just abort
      return ""
   }

   # vtk object
   if { [string range $word1 0 2] == "vtk"} {return $word1}

   if { $word1 == "char"} {
      if { [string index $word2 0] == "*"} {
	 return "char *"
      } else {
	 return "char"
      }
   }

   # vtk objects and strings are the only allowed pointer argument types
   if { [string index $word2 0] == "*" || 
	[string index $word2 [expr [string length $word2] - 1]] == "]"} {
      # we have an array.  What do we do???
      return ""
   }   

   if { $word1 == "void"} {return ""}
   if { $word1 == "float"} {return $word1}
   if { $word1 == "int"} {return $word1}
   if { $word1 == "short"} {return $word1}
   if { $word1 == "long"} {return $word1}
   if { $word1 == "double"} {return $word1}



   if { $word1 == "unsigned"} {

      set word3 [lindex $argDef 2]
      # vtk objects and strings are the only allowed pointer argument types
      if { [string index $word3 0] == "*" || 
	   [string index $word3 [expr [string length $word2] - 1]] == "]"} {
	 # we have an array.  What do we do???
	 return ""
      }   
      
      if { $word2 == "int"} {return "$word1 $word2"}
      if { $word2 == "char"} {return "$word1 $word2"}
   }

   return ""
}


# do I have to define everything?
proc MyStringLastSpace {argDef} {
   set idx [string length $argDef]
   while { $idx > 0 } {
      set idx [expr $idx - 1]
      if { [string index $argDef $idx] == " "} {
	 return $idx
      }
   }

   return -1
}



# ---------------------------------------------------------------------------
# concrete stuff
# improve this to remember previously found concrete classes




# this procedure tries to create a concrete instance of a class.
# it has to look through *.h files to find concreate subclasses.
proc ConcreteNew {class kit} {
   global CONCRETE_HACK
   global CONCRETE_ARRAY

   # try creating this object
   set objectName [new $class]
   if { $objectName != "" } {
      set CONCRETE_HACK $class
      return $objectName
   }

   # have we found a concrete subclass before?
   if {[catch {set concrete_class $CONCRETE_ARRAY($class)}] == 0} {
      #puts "                 concete for $class: $concrete_class found before"
      set objectName [new $concrete_class]
      if { $objectName == "" } {
	 #puts "---Cannot create concrete class $concrete_class !!!!"
	 # debug
	 return ""
      }
      return $objectName
   }
   

   # look through all the objects in the kit to find a sub class
   set inFiles [lsort [glob -nocomplain ../$kit/*.h]]
   foreach f $inFiles {
      set subClass [file rootname [file tail $f]]
      if { [CheckSubclassRelationship $class $subClass $kit] == 1} {
	 set objectName [ConcreteNew $subClass $kit]
	 if { $objectName != "" } {
	    set CONCRETE_ARRAY($class) $CONCRETE_HACK
	    return $objectName
	 }
      }
   }   

   # look through all the objects in common a sub class
   if { $kit != "common"} {
      set inFiles [lsort [glob -nocomplain ../common/*.h]]
      foreach f $inFiles {
	 set subClass [file rootname [file tail $f]]
	 if { [CheckSubclassRelationship $class $subClass "common"] == 1} {
	    set objectName [ConcreteNew $subClass "common"]
	    if { $objectName != "" } {
	       set CONCRETE_ARRAY($class) $CONCRETE_HACK
	       return $objectName
	    }
	 }
      }   
   }

   # look through all the objects in graphics a sub class
   if { $kit != "graphics"} {
      set inFiles [lsort [glob -nocomplain ../graphics/*.h]]
      foreach f $inFiles {
	 set subClass [file rootname [file tail $f]]
	 if { [CheckSubclassRelationship $class $subClass "graphics"] == 1} {
	    set objectName [ConcreteNew $subClass "graphics"]
	    if { $objectName != "" } {
	       set CONCRETE_ARRAY($class) $CONCRETE_HACK
	       return $objectName
	    }
	 }
      }   
   }

   # look through all the objects in imaging a sub class
   if { $kit != "imaging"} {
      set inFiles [lsort [glob -nocomplain ../imaging/*.h]]
      foreach f $inFiles {
	 set subClass [file rootname [file tail $f]]
	 if { [CheckSubclassRelationship $class $subClass "imaging"] == 1} {
	    set objectName [ConcreteNew $subClass "imaging"]
	    if { $objectName != "" } {
	       set CONCRETE_ARRAY($class) $CONCRETE_HACK
	       return $objectName
	    }
	 }
      }   
   }


   #puts "could not find a concrete subclass !!!"

   return ""
}

proc CheckSubclassRelationship {class subClass subClassKit} {
   #puts "    CheckSubclassRelationship $class $subClass"
   # look in the header file.
   set fileName "../$subClassKit/$subClass.h"
   if { ! [file exists $fileName] } {
      # ok look in common
      set fileName "../common/$subClass.h"
      if { ! [file exists $fileName] } {
	 # ok look in graphics
	 set fileName "../graphics/$subClass.h"
	 if { ! [file exists $fileName] } {
	 # ok look in imaging
	    set fileName "../imaging/$subClass.h"
	    if { ! [file exists $fileName] } {
	       #puts "Could not find .h file"
	       return 0
	    }
	 }
      }
   }
   # scan file
   set fd [open $fileName]
   set str [getline $fd]
   while { $str != ""} {
      if { [string first "class VTK_EXPORT" $str] != -1} {
	 regsub "\{" $str " " str
	 # we have the class definition line.
	 close $fd
	 # get the class name
	 set idx [string first "public" $str]
	 # no superclass
	 if { $idx == -1} {
	    return 0
	    #puts "No Supper class"
	 }
	 set str [string trim [string range $str [expr $idx + 7] end]]
	 set str [lindex $str 0]
	 if { $str == $class} {
	    return 1
	    #puts "return 1"
	 } else {
	    return [CheckSubclassRelationship $class $str $subClassKit]
	 }
      }
      set str [getline $fd]
   }
   close $fd
   
   #puts "Could not find superclass"
   return 0
}


# A method to make an instance of a class with a unique name.
proc new {className} {
   global DEBUG

   set counterName [format {%sCounter} $className]
   global $counterName
   if {[info exists $counterName] == 0} {
      set $counterName 0
   }
   # Choose a name that is not being used
   set instanceName [format {%s%d} $className [incr $counterName]]
   while {[info commands $instanceName] != ""} {
      set instanceName [format {%s%d} $className [incr $counterName]]
   }
   # make the vtk object
   if {[catch "$className $instanceName"] != 0} {
      return ""
   }
 
   if {$DEBUG} {puts "$className $instanceName"}

   return $instanceName
}


# do not test certain methods (with no error checking)
proc CheckException {methodName} {

   # I give up on this one!
   if {$methodName == "SetRoll"} {
      return 1
   }

   if {$methodName == "SetPromptUser"} {
      return 1
   }

   if {$methodName == "SetPoint"} {
      return 1
   }

   if {$methodName == "SetVector"} {
      return 1
   }

   if {$methodName == "SetScalar"} {
      return 1
   }

   if {$methodName == "SetTensor"} {
      return 1
   }

   if {$methodName == "SetNormal"} {
      # ( vtkPlaneSource:: Error with these args)
      return 1
   }

   if {$methodName == "SetBackgroundColor"} {
      # ( vtkXImageWindow:: Error with these args)
      return 1
   }

   if {$methodName == "SetSize"} {
      # window size 0 0 gives problems
      return 1
   }

   if {$methodName == "SetDebug"} {
      return 1
   }
   if {$methodName == "SetGlobalWarningDisplay"} {
      return 1
   }
   if {$methodName == "SetReleaseDataFlag"} {
      return 1
   }
   if {$methodName == "SetDataReleased"} {
      return 1
   }
   if {$methodName == "SetTuple"} {
      return 1
   }
   if {$methodName == "SetTCoord"} {
      return 1
   }

   if {$methodName == "SetTag"} {
      return 1
   }

   if {$methodName == "SetFilteredAxes"} {
      # 0 0 and 1 1 are not valid
      return 1
   }

   return 0
}


wm withdraw .


# show no warnings
vtkImageViewer viewer
viewer GlobalWarningDisplayOff



# Check to see if  classes was specified.
if { $argv != ""} {
   foreach file $argv {
      # we do not know what kit it is in, so try them all
      TestObject contrib $file
      TestObject patented $file
      TestObject common $file
      TestObject imaging $file
      TestObject graphics $file
   }
} else {
   # Still Reference counting problems in graphics. (seg faults)
   # next: Exporter has pointer to deleted window (GetMTime)
   #TestObject graphics vtkIVExporter
   TestKit contrib
   TestKit patented
   TestKit common
   TestKit imaging
   # (seg faults becasue objects are not ref counted)
   #TestKit graphics
}

viewer GlobalWarningDisplayOn



exit

