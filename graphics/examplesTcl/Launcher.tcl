#
# some functions for look-and-feel
#

set the_font "-*-helvetica-bold-r-normal--12-*-*-*-p-70-iso8859-1"

proc SetFrameProperties { w } {
    $w configure -bg #224488 -bd 0
}


proc SetButtonProperties { w } {
    global the_font 
    $w configure -bg #bbbbbb -fg #224488 -highlightthickness 0 \
	    -activebackground #dddddd -activeforeground #112244 \
	    -font $the_font
}

proc SetLabelProperties { w } {
    global the_font 
    $w configure -bg #224488 -fg #bbbbbb \
	    -font $the_font
}

proc SetMenuProperties { w } {
    global the_font 

    $w configure -fg #224488 -bg #bbbbbb \
	    -activebackground #dddddd -activeforeground #112244 \
	    -font $the_font  -tearoff 0
}

proc SetMenuButtonProperties { w } {
    global the_font 

    $w configure -bg #bbbbbb -fg #224488 \
	    -activebackground #dddddd -activeforeground #112244 \
	    -font $the_font -highlightthickness 0 -bd 4 -relief raised
}

proc SetScrolledListboxProperties { w1 w2 } {
    global the_font
 
    $w1 configure -bg #bbbbbb -fg #000000 \
	    -font $the_font \
	    -selectforeground #000000 -selectbackground #cccccc \
	    -bd 4 -relief sunken -highlightthickness 0 \
	    -yscrollcommand " $w2 set "
    $w2 configure -bg #bbbbbb \
	    -activebackground #dddddd -troughcolor #aaaaaa \
	    -highlightthickness 0 -bd 4 -relief sunken \
	    -command " $w1 yview " -orient vertical
}

proc SetScrolledTextProperties { w1 w2 } {
    global the_font 

    $w1 configure -bg #bbbbbb -fg #000000 \
	    -font $the_font \
	    -selectforeground #000000 -selectbackground #cccccc \
	    -bd 4 -relief sunken -highlightthickness 0 \
	    -yscrollcommand " $w2 set "
    $w2 configure -bg #bbbbbb \
	    -activebackground #dddddd -troughcolor #aaaaaa \
	    -highlightthickness 0 -bd 4 -relief sunken \
	    -command " $w1 yview " -orient vertical
}

#
# Process the data file
#

set DataFile Launcher.dat

set fd [open $DataFile]
set ActiveDemo none

set OneLine [gets $fd]

while { ![eof $fd] } {
    if { $OneLine != "" } {
	set count [scan $OneLine "%s" FirstWord]
	
	if { $count > 0 && $FirstWord != "" } {

	    if { $FirstWord == "Demo" } {
		scan $OneLine "%s %s" FirstWord SecondWord
		set index [expr [string first $SecondWord $OneLine] + \
			[string length $SecondWord] + 1]
		set DemoName [string range $OneLine $index end]
		set Demos($SecondWord) $DemoName
		set ActiveDemo $SecondWord
	    }
	    
	    if { $FirstWord == "Chapter" } {
		scan $OneLine "%s %s" FirstWord SecondWord
		set CurrentList [lindex [array get Chapters $SecondWord] 1]
		set CurrentList [lappend CurrentList $ActiveDemo]
		set Chapters($SecondWord) $CurrentList
	    }

	    if { $FirstWord == "Topic" } {
		set index [expr [string first $FirstWord $OneLine] + \
			[string length $FirstWord] + 1]
		set TopicName [string range $OneLine $index end]
		set CurrentList [lindex [array get Topics $TopicName] 1]
		set CurrentList [lappend CurrentList $ActiveDemo]
		set Topics($TopicName) $CurrentList
	    }

	    if { $FirstWord == "BeginDescription" } {
		set OneLine [gets $fd]
		set Message ""
		while { $OneLine != "EndDescription" } {
		    set Message "$Message $OneLine"
		    set OneLine [gets $fd]
		}
		set Descriptions($ActiveDemo) $Message
	    }
	}
    }

    set OneLine [gets $fd]
}


#
# Create the user interface
#

set ViewBy Topic

wm withdraw .

toplevel .win -visual best -bg #224488

frame .win.viewby
SetFrameProperties .win.viewby
.win.viewby configure -bd 2 -relief ridge

label .win.viewby.label -text "View Demos By: "
SetLabelProperties .win.viewby.label

menubutton .win.viewby.menubutton -text $ViewBy \
	-menu .win.viewby.menubutton.menu
SetMenuButtonProperties .win.viewby.menubutton

menu .win.viewby.menubutton.menu
SetMenuProperties .win.viewby.menubutton.menu
.win.viewby.menubutton.menu add command -label "Topic" \
	-command { .win.viewby.menubutton configure -text Topic; \
	set ViewBy Topic; \
	.win.type.label configure -text {                Topic: }; \
	catch { pack forget .win.type.topicmenu }; \
	catch { pack forget .win.type.chaptermenu }; \
	pack .win.type.topicmenu -side left -expand 0 -fill none \
	-padx 4 -pady 4; \
	ChangeViewBy }
.win.viewby.menubutton.menu add command -label "Chapter" \
	-command { .win.viewby.menubutton configure -text {Chapter}; \
	set ViewBy Chapter; \
	.win.type.label configure -text {            Chapter: }; \
	catch { pack forget .win.type.topicmenu }; \
	catch { pack forget .win.type.chaptermenu }; \
	pack .win.type.chaptermenu -side left -expand 0 -fill none \
	-padx 4 -pady 4; \
	ChangeViewBy }

pack .win.viewby.label .win.viewby.menubutton -side left -expand 0 -fill none \
	-padx 4 -pady 4 -anchor w


set Topic [lindex [array names Topics] 1]
set Chapter [lindex [array names Chapters] 1]

frame .win.type
SetFrameProperties .win.type
.win.type configure -bd 2 -relief ridge

label .win.type.label -text {                Topic: }
SetLabelProperties .win.type.label

menubutton .win.type.topicmenu -text $Topic \
	-menu .win.type.topicmenu.menu
SetMenuButtonProperties .win.type.topicmenu

menu .win.type.topicmenu.menu
SetMenuProperties .win.type.topicmenu.menu
foreach name [array names Topics] {
  .win.type.topicmenu.menu add command -label $name \
	  -command [subst { .win.type.topicmenu configure -text "$name"; \
	  set Topic "$name"; ChangeTopic }]
}

menubutton .win.type.chaptermenu -text $Chapter \
	-menu .win.type.chaptermenu.menu
SetMenuButtonProperties .win.type.chaptermenu

menu .win.type.chaptermenu.menu
SetMenuProperties .win.type.chaptermenu.menu
foreach name [array names Chapters] {
  .win.type.chaptermenu.menu add command -label $name \
	  -command [subst { .win.type.chaptermenu configure -text "$name"; \
	  set Chapter "$name"; ChangeChapter }]
}

pack .win.type.label .win.type.topicmenu -side left -expand 0 -fill none \
	-padx 4 -pady 4 -anchor w

frame .win.listframe
SetFrameProperties .win.listframe
.win.listframe configure -bd 2 -relief ridge

frame .win.listframe.listmsg
SetFrameProperties .win.listframe.listmsg

label .win.listframe.listmsg.msg -text "Demo List

Single Click To View Description
Double Click To Run Demo"
SetLabelProperties .win.listframe.listmsg.msg
pack .win.listframe.listmsg.msg -side top -anchor n -expand 1 -fill both

frame .win.listframe.list
SetFrameProperties .win.listframe.list

listbox .win.listframe.list.box -width 50 -height 10
scrollbar .win.listframe.list.scroll
SetScrolledListboxProperties .win.listframe.list.box .win.listframe.list.scroll

pack .win.listframe.list.box .win.listframe.list.scroll \
	-side left -expand 1 -fill both \
	-padx 4 -pady 4

bind .win.listframe.list.box <ButtonRelease> { DisplayDescription }
bind .win.listframe.list.box <Double-Button-1> { RunDemo }

pack .win.listframe.listmsg .win.listframe.list -side top \
	-expand 1 -fill both -padx 4 -pady 4

frame .win.descframe
SetFrameProperties .win.descframe
.win.descframe configure -bd 2 -relief ridge

frame .win.descframe.descmsg

label .win.descframe.descmsg.msg -text "Demo Description"
SetLabelProperties .win.descframe.descmsg.msg
pack .win.descframe.descmsg.msg -side top -anchor n -expand 1 -fill both

frame .win.descframe.desc
SetFrameProperties .win.descframe.desc

pack .win.descframe.descmsg .win.descframe.desc -side top \
	-expand 1 -fill both -padx 4 -pady 4

text .win.descframe.desc.text -width 50 -height 10 -state disabled -wrap word
scrollbar .win.descframe.desc.scroll
SetScrolledTextProperties .win.descframe.desc.text .win.descframe.desc.scroll
pack .win.descframe.desc.text .win.descframe.desc.scroll \
	-side left -expand 1 -fill both -padx 4 -pady 4

frame .win.msg
SetFrameProperties .win.msg

label .win.msg.running -text ""
SetLabelProperties .win.msg.running
pack .win.msg.running -side top -anchor n -expand 1 -fill both

frame .win.controls
SetFrameProperties .win.controls

button .win.controls.exit -text Exit -command { exit }
SetButtonProperties .win.controls.exit
pack .win.controls.exit -side top -expand 1 -fill both -padx 4 -pady 4


pack .win.viewby .win.type .win.listframe \
	.win.descframe .win.msg .win.controls \
	-side top -padx 4 -pady 4 -expand 1 -fill both -anchor w


proc ChangeViewBy { } {
    global ViewBy
    
    if { $ViewBy == "Topic" } {
	ChangeTopic
    } elseif { $ViewBy == "Chapter" } {
	ChangeChapter
    }
}

proc ChangeTopic { } {
    global Topic Topics Demos
    
    .win.listframe.list.box delete 0 end
    set demo_list [lindex [array get Topics $Topic] 1]
    foreach demo $demo_list {
	set demo_name [lindex [array get Demos $demo] 1]
	.win.listframe.list.box insert end $demo_name
    }

    .win.descframe.desc.text configure -state normal
    .win.descframe.desc.text delete 0.0 end
    .win.descframe.desc.text configure -state disabled

}

proc ChangeChapter { } {
    global Chapter Chapters Demos
    
    .win.listframe.list.box delete 0 end
    set demo_list [lindex [array get Chapters $Chapter] 1]
    foreach demo $demo_list {
	set demo_name [lindex [array get Demos $demo] 1]
      .win.listframe.list.box insert end $demo_name
    }

    .win.descframe.desc.text configure -state normal
    .win.descframe.desc.text delete 0.0 end
    .win.descframe.desc.text configure -state disabled
}

proc RunDemo { } {
    global Demos

    set index [.win.listframe.list.box curselection]

    if { $index >= 0 } {
	set demo_name [.win.listframe.list.box get $index]
	set demo_list [array names Demos]
	foreach demo $demo_list {
	    if { [lindex [array get Demos $demo] 1] == $demo_name } {
		set demo_script $demo
	    }
	}
	.win.msg.running configure -text "Starting demo: $demo_script"
	update
	catch { exec /home/homer/u0/sobie/vtk-sgi-irix6/tcl/vtk $demo_script &}
	after 1000
    }
}

proc DisplayDescription { } {
    global Demos Descriptions

    update 

    set index [.win.listframe.list.box curselection]

    if { $index >= 0 } {
	set demo_name [.win.listframe.list.box get $index]
	set demo_list [array names Demos]
	foreach demo $demo_list {
	    if { [lindex [array get Demos $demo] 1] == $demo_name } {
		set demo_script $demo
	    }
	}
	.win.descframe.desc.text configure -state normal
	.win.descframe.desc.text delete 0.0 end
	set description [lindex [array get Descriptions $demo_script] 1]
	.win.descframe.desc.text insert end $description
	.win.descframe.desc.text configure -state disabled
    }
}

ChangeViewBy


