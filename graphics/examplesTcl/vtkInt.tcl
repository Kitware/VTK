# a generic interactor for tcl and vtk
#
set vtkInteractBold "-background #43ce80 -relief raised -borderwidth 1"
set vtkInteractNormal "-background #c0c0c0 -relief flat"
set vtkInteractTagcount 1;

proc vtkInteract {} {

    proc dovtk {s w} {
	global vtkInteractBold vtkInteractNormal vtkInteractTagcount;
	set tag [append tagnum $vtkInteractTagcount];
	incr vtkInteractTagcount 1;
	.vtkInteract.text configure -state normal
	.vtkInteract.text insert end $s $tag
	eval .vtkInteract.text tag configure $tag $vtkInteractNormal
	.vtkInteract.text tag bind $tag <Any-Enter> \
	    ".vtkInteract.text tag configure $tag $vtkInteractBold"
	.vtkInteract.text tag bind $tag <Any-Leave> \
	    ".vtkInteract.text tag configure $tag $vtkInteractNormal"
	.vtkInteract.text tag bind $tag <1> "dovtk [list $s] .vtkInteract";
	.vtkInteract.text insert end \n; 
	.vtkInteract.text insert end [uplevel 1 $s]; 
	.vtkInteract.text insert end \n\n; 
	.vtkInteract.text configure -state disabled
	.vtkInteract.text yview end
    }

    catch {destroy .vtkInteract}
    toplevel .vtkInteract
    wm title .vtkInteract "vtk Interactor"
    wm iconname .vtkInteract "vtk"
    
    frame .vtkInteract.buttons
    pack  .vtkInteract.buttons -side bottom -fill x -pady 2m
    button .vtkInteract.buttons.dismiss -text Dismiss \
	-command "wm withdraw .vtkInteract"
    pack .vtkInteract.buttons.dismiss -side left -expand 1
    
    frame .vtkInteract.file
    label .vtkInteract.file.label -text "Command:" -width 10 -anchor w
    entry .vtkInteract.file.entry -width 40 
    bind .vtkInteract.file.entry <Return> {
	dovtk [%W get] .vtkInteract; %W delete 0 end}
    pack .vtkInteract.file.label -side left
    pack .vtkInteract.file.entry -side left -expand 1 -fill x
    
    text .vtkInteract.text -yscrollcommand ".vtkInteract.scroll set" \
	-setgrid true -width 60 -height 8 -wrap word
    scrollbar .vtkInteract.scroll -command ".vtkInteract.text yview"
    pack .vtkInteract.scroll -side right -fill y
    pack .vtkInteract.text -side bottom -expand 1 -fill both
    pack .vtkInteract.file -pady 3m -padx 2m -side bottom -fill x 
    
    .vtkInteract.text configure -state disabled
    wm withdraw .vtkInteract
}

vtkInteract;

