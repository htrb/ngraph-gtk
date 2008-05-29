#!/usr/bin/wish

#
# fitrslt.tcl written by S. ISHIZAKA 1999/04
#

#
# Color selection dialogbox. 
#
# global: red green blue
#
proc colordialog { w title } {

  proc newColor value {
    global red green blue
    set color [format #%02x%02x%02x [.c.top.f.red get] [.c.top.f.green get]\
              [.c.top.f.blue get]]
    .c.top.f.sample config -background $color
  }

  global dlgbutton red green blue
  toplevel $w -class Dialog
  wm title $w $title
  wm iconname $w Dialog
  wm geometry $w [wm geometry [winfo toplevel [winfo parent $w]]]
  wm geometry $w {}
  wm transient $w [winfo toplevel [winfo parent $w]]
  frame $w.top -relief raised -bd 1
  pack $w.top -side top -fill both
  frame $w.bot -relief raised -bd 1
  pack $w.bot -side bottom -fill both

  frame $w.top.f
  pack $w.top.f -side top -fill x -pady 1m -padx 5m -anchor w

  scale $w.top.f.red -label Red -from 0 -to 255 -length 10c \
             -orient horizontal -variable red -command newColor 
  scale $w.top.f.green -label Green -from 0 -to 255 -length 10c \
             -orient horizontal -variable green -command newColor 
  scale $w.top.f.blue -label Blue -from 0 -to 255 -length 10c \
             -orient horizontal -variable blue -command newColor
  frame $w.top.f.sample -height 1c -width 10c
  pack $w.top.f.red $w.top.f.green $w.top.f.blue $w.top.f.sample -side top

  button $w.bot.ok -text OK -command "set dlgbutton 1"
  frame $w.bot.default -relief sunken -bd 1
  raise $w.bot.ok
  pack $w.bot.default -side left -expand 1 -padx 2m -pady 2m
  pack $w.bot.ok -in $w.bot.default -side left -padx 1m -pady 1m \
       -ipadx 2m -ipady 1m
  button $w.bot.cancel -text Cancel -command "set dlgbutton 0"
  pack $w.bot.cancel -side left -expand 1 -padx 2m -pady 2m \
       -ipadx 2m -ipady 1m

  bind $w <Return> "$w.bot.ok flash; set dlgbutton ok"

  set oldFocus [focus]
  grab set $w
  focus $w
  tkwait variable dlgbutton
  destroy $w
  focus $oldFocus
  return $dlgbutton  
}

#
# Font selection dialogbox.
#
# global: textred textgreen textblue textpt textspc textsc textfont textjfont
#
proc fontdialog { w title } {
  proc setbuttoncolor { w } {
    global red green blue
    set color [format #%02x%02x%02x $red $green $blue]
    $w config -background $color -foreground $color
  }
  global dlgbutton2
  global fontlist jfontlist textfont textjfont textpt textspc textsc
  global textred textgreen textblue
  global pt spc sc
  global red green blue
  toplevel $w -class Dialog
  wm title $w $title
  wm iconname $w Dialog
  wm geometry $w [wm geometry [winfo toplevel [winfo parent $w]]]
  wm geometry $w {}
  wm transient $w [winfo toplevel [winfo parent $w]]
  frame $w.top -relief raised -bd 1
  pack $w.top -side top -fill both
  frame $w.bot -relief raised -bd 1
  pack $w.bot -side bottom -fill both

  frame $w.top.ptf
  frame $w.top.ff
  frame $w.top.ff.fontf 
  frame $w.top.ff.jfontf 
  frame $w.top.colorf
  pack $w.top.ptf $w.top.ff -side top -fill x -pady 1m -padx 5m -anchor w
  pack $w.top.ff.fontf $w.top.ff.jfontf -side left -padx 2m -anchor w
  pack $w.top.colorf -side top -fill x -pady 1m -padx 5m -anchor w

  label $w.top.ptf.textptl -text " Pt:"
  entry $w.top.ptf.textpt -width 7 -relief sunken -bd 2 -textvariable pt
  label $w.top.ptf.textspcl -text " Space:"
  entry $w.top.ptf.textspc -width 7 -relief sunken -bd 2 -textvariable spc
  label $w.top.ptf.textscl -text " Script:"
  entry $w.top.ptf.textsc -width 7 -relief sunken -bd 2 -textvariable sc
  pack $w.top.ptf.textptl $w.top.ptf.textpt $w.top.ptf.textspcl \
       $w.top.ptf.textspc $w.top.ptf.textscl $w.top.ptf.textsc \
       -side left -pady 2m -anchor w

  label $w.top.ff.fontf.fontl 
  frame $w.top.ff.fontf.f
  pack $w.top.ff.fontf.fontl $w.top.ff.fontf.f -side top -anchor w
  listbox $w.top.ff.fontf.f.font -width 25 -height 4 -relief raised -bd 2 \
         -selectmode single \
         -yscrollcommand "$w.top.ff.fontf.f.scrollf set"
  pack $w.top.ff.fontf.f.font -side left -anchor w
  scrollbar $w.top.ff.fontf.f.scrollf \
         -command "$w.top.ff.fontf.f.font yview"
  pack $w.top.ff.fontf.f.scrollf -side right -fill y
  foreach i $fontlist {
    $w.top.ff.fontf.f.font insert end $i
  }
  bind $w.top.ff.fontf.f.font <Double-1> \
    {.f.top.ff.fontf.fontl configure \
     -text [format "Font:%%s" [%W get active]]}

  label $w.top.ff.jfontf.jfontl
  frame $w.top.ff.jfontf.j
  pack $w.top.ff.jfontf.jfontl $w.top.ff.jfontf.j -side top -anchor w
  listbox $w.top.ff.jfontf.j.jfont -width 10 -height 4 -relief raised -bd 2 \
         -selectmode single \
         -yscrollcommand "$w.top.ff.jfontf.j.scrollj set"
  pack $w.top.ff.jfontf.j.jfont -side left -anchor w
  scrollbar $w.top.ff.jfontf.j.scrollj \
         -command "$w.top.ff.jfontf.j.jfont yview"
  pack $w.top.ff.jfontf.j.scrollj -side right -fill y
  foreach i $jfontlist {
    $w.top.ff.jfontf.j.jfont insert end $i
  }
  bind $w.top.ff.jfontf.j.jfont <Double-1> \
    {.f.top.ff.jfontf.jfontl configure \
     -text [format "Kanji:%%s" [%W get active]]}

  label $w.top.colorf.textcolf -text "Color:"
  button $w.top.colorf.textcol \
         -command "colordialog .c Color; \
                   setbuttoncolor $w.top.colorf.textcol"

  pack $w.top.colorf.textcolf $w.top.colorf.textcol \
       -side left -pady 2m -anchor w

  button $w.bot.ok -text OK -command "set dlgbutton2 1"
  frame $w.bot.default -relief sunken -bd 1
  raise $w.bot.ok
  pack $w.bot.default -side left -expand 1 -padx 2m -pady 2m
  pack $w.bot.ok -in $w.bot.default -side left -padx 1m -pady 1m \
       -ipadx 2m -ipady 1m
  button $w.bot.cancel -text Cancel -command "set dlgbutton2 0"
  pack $w.bot.cancel -side left -expand 1 -padx 2m -pady 2m \
       -ipadx 2m -ipady 1m

  set red $textred
  set green $textgreen
  set blue $textblue
  setbuttoncolor $w.top.colorf.textcol
  set pt $textpt
  set spc $textspc
  set sc $textsc
  $w.top.ff.fontf.f.font activate [lsearch -exact $fontlist $textfont]
  $w.top.ff.fontf.f.font yview [lsearch -exact $fontlist $textfont]
  $w.top.ff.jfontf.j.jfont activate [lsearch -exact $jfontlist $textjfont]
  $w.top.ff.jfontf.j.jfont yview [lsearch -exact $jfontlist $textjfont]
  $w.top.ff.fontf.fontl configure \
     -text [format "Font:%s" [$w.top.ff.fontf.f.font get active]]
  $w.top.ff.jfontf.jfontl configure \
     -text [format "Kanji:%s" [$w.top.ff.jfontf.j.jfont get active]]

  bind $w <Return> "$w.bot.ok flash; set dlgbutton2 ok"

  set oldFocus [focus]
  grab set $w
  focus $w
  tkwait variable dlgbutton2

  if { $dlgbutton2 } {
    set textred $red
    set textgreen $green
    set textblue $blue
    set textpt $pt
    set textspc $spc
    set textsc $sc
    set textfont [$w.top.ff.fontf.f.font get active]
    set textjfont [$w.top.ff.jfontf.j.jfont get active]
  }

  destroy $w
  focus $oldFocus
  return $dlgbutton2
}

proc loaddatalist {} {
  global datalist fitnum
  global filelist typelist userfunclist fileidlist idlist polylist
  global p0list p1list p2list p3list p4list p5list p6list p7list p8list p9list
  if { [file exist $datalist] } {
    set f [ open $datalist r ]
    gets $f fitnum
    for {set i 0} {$i < $fitnum} {incr i 1} {
      set fileidlist [concat $fileidlist [list [gets $f]]]
      set filelist [concat $filelist [list [gets $f]]]
      set idlist [concat $idlist [list [gets $f]]]
      set typelist [concat $typelist [list [gets $f]]]
      set polylist [concat $polylist [list [gets $f]]]
      set userfunclist [concat $userfunclist [list [gets $f]]]
      set p0list [concat $p0list [list [gets $f]]]
      set p1list [concat $p1list [list [gets $f]]]
      set p2list [concat $p2list [list [gets $f]]]
      set p3list [concat $p3list [list [gets $f]]]
      set p4list [concat $p4list [list [gets $f]]]
      set p5list [concat $p5list [list [gets $f]]]
      set p6list [concat $p6list [list [gets $f]]]
      set p7list [concat $p7list [list [gets $f]]]
      set p8list [concat $p8list [list [gets $f]]]
      set p9list [concat $p9list [list [gets $f]]]
    }
    close $f
  }
}

proc makescript { f gx gy height cap val} {
  global textfont textjfont textpt textspc textsc textred textgreen textblue
  global script frame
  puts $f "new text"
  puts $f [format "text::text=\'%s %s'" $cap $val]
  puts $f [format "text::x=%d" $gx]
  puts $f [format "text::y=%d" [expr $gy+$height]]
  puts $f [format "text::pt=%d" [expr $textpt]]
  puts $f "text::font=$textfont"
  puts $f "text::jfont=$textjfont"
  puts $f [format "text::space=%d" $textspc]
  puts $f [format "text::script_size=%d" $textsc]
  puts $f [format "text::R=%d" $textred]
  puts $f [format "text::G=%d" $textgreen]
  puts $f [format "text::B=%d" $textblue]
  if { $frame } {
    puts $f "iarray:textbbox:@=\${text::bbox}"
    puts $f "int:textlen:@=\"\${iarray:textbbox:get:2}-\${iarray:textbbox:get:0}\""
    puts $f "if \[ \"\${int:texttot:@}\" -lt \"\${int:textlen:@}\" \]; then"
    puts $f "int:texttot:@=\${int:textlen:@}"
    puts $f "fi"
  }
}

proc savescript {} {
  global fit fitnum filelist fileidlist
  global plus figure expand frame posx posy
  global p0 p1 p2 p3 p4 p5 p6 p7 p8 p9
  global cap0 cap1 cap2 cap3 cap4 cap5 cap6 cap7 cap8 cap9
  global val0 val1 val2 val3 val4 val5 val6 val7 val8 val9
  global textfont textjfont textpt textspc textsc textred textgreen textblue
  global script

  if { $script == "" } return
  set f [ open $script w ]
  if { $frame } {
    puts $f "new int name:textlen"
    puts $f "new int name:texttot"
    puts $f "new iarray name:textbbox"
  }
  set height [expr int($textpt*25.4/72.0)]
  set gy $posy

  if { $p0 } {
    set gx $posx
    makescript $f $gx $gy $height $cap0 $val0
    set gy [expr int($gy+$height*1.2)]
  }
  if { $p1 } {
    set gx $posx
    makescript $f $gx $gy $height $cap1 $val1
    set gy [expr int($gy+$height*1.2)]
  }
  if { $p2 } {
    set gx $posx
    makescript $f $gx $gy $height $cap2 $val2
    set gy [expr int($gy+$height*1.2)]
  }
  if { $p3 } {
    set gx $posx
    makescript $f $gx $gy $height $cap3 $val3
    set gy [expr int($gy+$height*1.2)]
  }
  if { $p4 } {
    set gx $posx
    makescript $f $gx $gy $height $cap4 $val4
    set gy [expr int($gy+$height*1.2)]
  }
  if { $p5 } {
    set gx $posx
    makescript $f $gx $gy $height $cap5 $val5
    set gy [expr int($gy+$height*1.2)]
  }
  if { $p6 } {
    set gx $posx
    makescript $f $gx $gy $height $cap6 $val6
    set gy [expr int($gy+$height*1.2)]
  }
  if { $p7 } {
    set gx $posx
    makescript $f $gx $gy $height $cap7 $val7
    set gy [expr int($gy+$height*1.2)]
  }
  if { $p8 } {
    set gx $posx
    makescript $f $gx $gy $height $cap8 $val8
    set gy [expr int($gy+$height*1.2)]
  }
  if { $p9 } {
    set gx $posx
    makescript $f $gx $gy $height $cap9 $val9
    set gy [expr int($gy+$height*1.2)]
  }


  if { $frame } {
    puts $f "new rectangle"
    puts $f [format "rectangle::x1=%d" [expr int($posx-$height/4)]]
    puts $f [format "rectangle::y1=%d" $posy]
    puts $f [format "rectangle::x2=%d+\${int:texttot:@}" \
            [expr int($posx+3*$height/4)]]
    puts $f [format "rectangle::y2=%d" [expr int($gy+$height/2)]]
    puts $f "rectangle::R=0"
    puts $f "rectangle::G=0"
    puts $f "rectangle::B=0"
    puts $f "rectangle::fill=true"
    puts $f "new rectangle"
    puts $f [format "rectangle::x1=%d" [expr int($posx-$height/2)]]
    puts $f [format "rectangle::y1=%d" [expr int($posy-$height/4)]]
    puts $f [format "rectangle::x2=%d+\${int:texttot:@}" \
            [expr int($posx+$height/2)]]
    puts $f [format "rectangle::y2=%d" [expr int($gy+$height/4)]]
    puts $f "rectangle::R=255"
    puts $f "rectangle::G=255"
    puts $f "rectangle::B=255"
    puts $f "rectangle::R2=0"
    puts $f "rectangle::G2=0"
    puts $f "rectangle::B2=0"
    puts $f "rectangle::fill=true"
    puts $f "rectangle::frame=true"
    puts $f "del int:textlen"
    puts $f "del int:texttot"
    puts $f "del iarray:textbbox"
  }
  close $f
}

proc setvalue { dummy } {
  global fit idlist
  global p0list p1list p2list p3list p4list p5list p6list p7list p8list p9list
  global plus figure expand
  global val0 val1 val2 val3 val4 val5 val6 val7 val8 val9

  set fmt ""
  if { $plus } {
    set fmt [format "%%#+.%dg" $figure]
  } else {
    set fmt [format "%%#.%dg" $figure]
  }

  if { $expand } {
    set val0 [format $fmt [lindex $p0list $fit]]
    set val1 [format $fmt [lindex $p1list $fit]]
    set val2 [format $fmt [lindex $p2list $fit]]
    set val3 [format $fmt [lindex $p3list $fit]]
    set val4 [format $fmt [lindex $p4list $fit]]
    set val5 [format $fmt [lindex $p5list $fit]]
    set val6 [format $fmt [lindex $p6list $fit]]
    set val7 [format $fmt [lindex $p7list $fit]]
    set val8 [format $fmt [lindex $p8list $fit]]
    set val9 [format $fmt [lindex $p9list $fit]]
  } else {
    set val0 [format "%%pf{%s %%{fit:%d:%%00}}" $fmt \
             [lindex $idlist $fit]]
    set val1 [format "%%pf{%s %%{fit:%d:%%01}}" $fmt \
             [lindex $idlist $fit]]
    set val2 [format "%%pf{%s %%{fit:%d:%%02}}" $fmt \
             [lindex $idlist $fit]]
    set val3 [format "%%pf{%s %%{fit:%d:%%03}}" $fmt \
             [lindex $idlist $fit]]
    set val4 [format "%%pf{%s %%{fit:%d:%%04}}" $fmt \
             [lindex $idlist $fit]]
    set val5 [format "%%pf{%s %%{fit:%d:%%05}}" $fmt \
             [lindex $idlist $fit]]
    set val6 [format "%%pf{%s %%{fit:%d:%%06}}" $fmt \
             [lindex $idlist $fit]]
    set val7 [format "%%pf{%s %%{fit:%d:%%07}}" $fmt \
             [lindex $idlist $fit]]
    set val8 [format "%%pf{%s %%{fit:%d:%%08}}" $fmt \
             [lindex $idlist $fit]]
    set val9 [format "%%pf{%s %%{fit:%d:%%09}}" $fmt \
             [lindex $idlist $fit]]
  }
}

proc selchange { w w2 } {
  global fit filelist fileidlist
  global figure typelist polylist userfunclist
  global p0 p1 p2 p3 p4 p5 p6 p7 p8 p9

  set fit [eval $w curselection]
  if { $fit=="" } {set fit 0}
  $w2 configure -text [format "Fitting: #%d %s" \
     [lindex $fileidlist $fit] [lindex $filelist $fit]]
  set type [lindex $typelist $fit]
  if { $type=="poly" } {
    set dim [lindex $polylist $fit]
    if { $dim==1 } {
      set p0 1; set p1 1; set p2 0; set p3 0; set p4 0
      set p5 0; set p6 0; set p7 0; set p8 0;set p9 0
    } elseif { $dim==2 } {
      set p0 1; set p1 1; set p2 1; set p3 0; set p4 0
      set p5 0; set p6 0; set p7 0; set p8 0;set p9 0
    } elseif { $dim==3 } {
      set p0 1; set p1 1; set p2 1; set p3 1; set p4 0
      set p5 0; set p6 0; set p7 0; set p8 0;set p9 0
    } elseif { $dim==4 } {
      set p0 1; set p1 1; set p2 1; set p3 1; set p4 1
      set p5 0; set p6 0; set p7 0; set p8 0;set p9 0
    } elseif { $dim==5 } {
      set p0 1; set p1 1; set p2 1; set p3 1; set p4 1
      set p5 1; set p6 0; set p7 0; set p8 0;set p9 0
    } elseif { $dim==6 } {
      set p0 1; set p1 1; set p2 1; set p3 1; set p4 1
      set p5 1; set p6 1; set p7 0; set p8 0;set p9 0
    } elseif { $dim==7 } {
      set p0 1; set p1 1; set p2 1; set p3 1; set p4 1
      set p5 1; set p6 1; set p7 1; set p8 0;set p9 0
    } elseif { $dim==8 } {
      set p0 1; set p1 1; set p2 1; set p3 1; set p4 1
      set p5 1; set p6 1; set p7 1; set p8 1;set p9 0
    } elseif { $dim==9 } {
      set p0 1; set p1 1; set p2 1; set p3 1; set p4 1
      set p5 1; set p6 1; set p7 1; set p8 1;set p9 1
    }
  } elseif { ($type=="exp") || ($type=="log") || ($type=="pow") } {
    set p0 1; set p1 1; set p2 0; set p3 0; set p4 0
    set p5 0; set p6 0; set p7 0; set p8 0;set p9 0
  } else {
    set func [lindex $userfunclist $fit]
    set p0 0; set p1 0; set p2 0; set p3 0; set p4 0
    set p5 0; set p6 0; set p7 0; set p8 0;set p9 0
    if {[string first "%00" $func]!=-1} {set p0 1}
    if {[string first "%01" $func]!=-1} {set p1 1}
    if {[string first "%02" $func]!=-1} {set p2 1}
    if {[string first "%03" $func]!=-1} {set p3 1}
    if {[string first "%04" $func]!=-1} {set p4 1}
    if {[string first "%05" $func]!=-1} {set p5 1}
    if {[string first "%06" $func]!=-1} {set p6 1}
    if {[string first "%07" $func]!=-1} {set p7 1}
    if {[string first "%08" $func]!=-1} {set p8 1}
    if {[string first "%09" $func]!=-1} {set p9 1}
  }
  setvalue $figure
}

proc setupwindow {} {
  global fit fitnum filelist fileidlist
  global plus figure expand frame posx posy
  global p0 p1 p2 p3 p4 p5 p6 p7 p8 p9
  global cap0 cap1 cap2 cap3 cap4 cap5 cap6 cap7 cap8 cap9
  global val0 val1 val2 val3 val4 val5 val6 val7 val8 val9

  frame .top -relief raised -bd 1
  frame .bot -relief raised -bd 1
  pack .top -side top -fill both
  pack .bot -side bottom -fill both 

  frame .titlef -relief sunken -bd 1
  frame .formatf -relief groove -bd 2
  frame .ff -bd 1
  frame .ff.filef -bd 1
  frame .ff.filef.labelf -bd 1
  frame .ff.filef.listf -bd 1
  frame .ff.positionf -relief groove -bd 2
  frame .ff.positionf.posxf -bd 1
  frame .ff.positionf.posyf -bd 1
  frame .captionf -bd 1
  frame .captionf.f0 -bd 1
  frame .captionf.f1 -bd 1
  frame .captionf.f2 -bd 1
  frame .captionf.f3 -bd 1
  frame .captionf.f4 -bd 1
  frame .captionf.f5 -bd 1
  frame .captionf.f6 -bd 1
  frame .captionf.f7 -bd 1
  frame .captionf.f8 -bd 1
  frame .captionf.f9 -bd 1

  pack .titlef -in .top -side top -fill x -pady 5m -padx 5m -anchor center
  pack .formatf .ff .captionf -in .top -side top \
        -fill x -pady 1m -padx 5m -anchor w
  pack .ff.filef .ff.positionf -side left -padx 5m -anchor w
  pack .ff.filef.labelf .ff.filef.listf -side top
  pack .ff.positionf.posxf .ff.positionf.posyf -padx 3m -side top
  pack .captionf.f0 .captionf.f1 .captionf.f2 .captionf.f3 .captionf.f4 \
       .captionf.f5 .captionf.f6 .captionf.f7 .captionf.f8 .captionf.f9 \
       -side top

  label .title -text "Fitrslt version 1.00.00" 
  pack .title -in .titlef -side top -pady 1m -padx 5m -anchor center

  label .title2 -text "fitting results ---> legend-text" 
  pack .title2 -in .titlef -side top -pady 1m -padx 5m -anchor center

  checkbutton .plus -text "Add +" -variable plus \
    -command "setvalue $figure"
  pack .plus -in .formatf -side left -pady 2m -anchor w

  scale .figure -label accuracy -from 1 -to 15 -length 5c \
    -orient horizontal -variable figure -command "setvalue"
  pack .figure -in .formatf -side left -anchor w

  checkbutton .expand -text "Expand" -variable expand \
    -command "setvalue $figure"
  checkbutton .frame -text "Frame" -variable frame
  pack .expand .frame -in .formatf -side left -pady 2m -anchor w

  label .file -width 25
  pack .file -in .ff.filef.labelf -side top -anchor w
  listbox .files -width 20 -height 3 -relief raised -bd 2 -selectmode single \
          -yscrollcommand ".scroll set"
  pack .files -in .ff.filef.listf -side left -anchor center 
  scrollbar .scroll -command ".files yview"
  pack .scroll -in .ff.filef.listf -side left -fill y
  .files delete 0 end
  for {set i 0} {$i < $fitnum} {incr i 1} {
    .files insert end [format "#%d %s" \
              [lindex $fileidlist $i] [lindex $filelist $i]]
  }
  bind .files <Double-1> {selchange .files .file}
  selchange .files .file

  label .posxl -text "X:"
  entry .posx -width 8 -relief sunken -bd 2 -textvariable posx
  pack .posxl .posx -in .ff.positionf.posxf \
  -side left -pady 2m -anchor w

  label .posyl -text "Y:"
  entry .posy -width 8 -relief sunken -bd 2 -textvariable posy
  pack .posyl .posy -in .ff.positionf.posyf \
  -side left -pady 2m -anchor w

  checkbutton .p0 -text "%00" -variable p0
  label .p0l -text "Caption:"
  entry .ce0 -width 14 -relief sunken -bd 2 -textvariable cap0
  entry .ve0 -width 14 -relief sunken -bd 2 -textvariable val0
  pack .p0 .p0l .ce0 .ve0 -in .captionf.f0 -side left
  checkbutton .p1 -text "%01" -variable p1
  label .p1l -text "Caption:"
  entry .ce1 -width 14 -relief sunken -bd 2 -textvariable cap1
  entry .ve1 -width 14 -relief sunken -bd 2 -textvariable val1
  pack .p1 .p1l .ce1 .ve1 -in .captionf.f1 -side left
  checkbutton .p2 -text "%02" -variable p2
  label .p2l -text "Caption:"
  entry .ce2 -width 14 -relief sunken -bd 2 -textvariable cap2
  entry .ve2 -width 14 -relief sunken -bd 2 -textvariable val2
  pack .p2 .p2l .ce2 .ve2 -in .captionf.f2 -side left
  checkbutton .p3 -text "%03" -variable p3
  label .p3l -text "Caption:"
  entry .ce3 -width 14 -relief sunken -bd 2 -textvariable cap3
  entry .ve3 -width 14 -relief sunken -bd 2 -textvariable val3
  pack .p3 .p3l .ce3 .ve3 -in .captionf.f3 -side left
  checkbutton .p4 -text "%04" -variable p4
  label .p4l -text "Caption:"
  entry .ce4 -width 14 -relief sunken -bd 2 -textvariable cap4
  entry .ve4 -width 14 -relief sunken -bd 2 -textvariable val4
  pack .p4 .p4l .ce4 .ve4 -in .captionf.f4 -side left
  checkbutton .p5 -text "%05" -variable p5
  label .p5l -text "Caption:"
  entry .ce5 -width 14 -relief sunken -bd 2 -textvariable cap5
  entry .ve5 -width 14 -relief sunken -bd 2 -textvariable val5
  pack .p5 .p5l .ce5 .ve5 -in .captionf.f5 -side left
  checkbutton .p6 -text "%06" -variable p6
  label .p6l -text "Caption:"
  entry .ce6 -width 14 -relief sunken -bd 2 -textvariable cap6
  entry .ve6 -width 14 -relief sunken -bd 2 -textvariable val6
  pack .p6 .p6l .ce6 .ve6 -in .captionf.f6 -side left
  checkbutton .p7 -text "%07" -variable p7
  label .p7l -text "Caption:"
  entry .ce7 -width 14 -relief sunken -bd 2 -textvariable cap7
  entry .ve7 -width 14 -relief sunken -bd 2 -textvariable val7
  pack .p7 .p7l .ce7 .ve7 -in .captionf.f7 -side left
  checkbutton .p8 -text "%08" -variable p8
  label .p8l -text "Caption:"
  entry .ce8 -width 14 -relief sunken -bd 2 -textvariable cap8
  entry .ve8 -width 14 -relief sunken -bd 2 -textvariable val8
  pack .p8 .p8l .ce8 .ve8 -in .captionf.f8 -side left
  checkbutton .p9 -text "%09" -variable p9
  label .p9l -text "Caption:"
  entry .ce9 -width 14 -relief sunken -bd 2 -textvariable cap9
  entry .ve9 -width 14 -relief sunken -bd 2 -textvariable val9
  pack .p9 .p9l .ce9 .ve9 -in .captionf.f9 -side left

  button .ok -text "OK" -command "savescript; exit"
  frame .default -relief sunken -bd 1
  raise .ok
  pack .default -side left -expand 1 -in .bot -padx 2m -pady 2m
  pack .ok -in .default -side left -padx 1m -pady 1m -ipadx 2m -ipady 1m

  button .cancel -text "Cancel" -command exit
  pack .cancel -in .bot -side left -expand 1 -padx 2m -pady 2m -ipadx 2m \
  -ipady 1m

  button .font -text "Font" -command "fontdialog .f Font"
  pack .font -in .bot -side left -expand 1 -padx 2m -pady 2m -ipadx 2m \
  -ipady 1m 
}

set fontlist {Times TimesBold TimesItalic TimesBoldItalic \
              Helvetica HelveticaBold HelveticaOblique HelveticaItalic \
              HelveticaBoldOblique HelveticaBoldItalic \
              Courier CourierBold CourierOblique CourierItalic \
              CourierBoldOblique CourierBoldItalic Symbol SymbolItalic \
              Tim TimB TimI TimBI Helv HelvB HelvO HelvI HelvBO HelvBI \
              Cour CourB CourO CourI CourBO CourBI Sym SymI}

set jfontlist {Mincho Gothic Min Goth}

set textred 0
set textgreen 0
set textblue 0
set textpt 2000
set textspc 0
set textsc 7000
set posx 5000
set posy 5000
set textfont Helvetica
set textjfont Gothic

set plus 0
set figure 5
set expand 1
set frame 1

set fit 0
set fitnum 0
set filelist ""
set typelist ""
set userfunclist ""
set fileidlist ""
set idlist ""
set polylist ""
set p0list ""
set p1list ""
set p2list ""
set p3list ""
set p4list ""
set p5list ""
set p6list ""
set p7list ""
set p8list ""
set p9list ""

set p0 1
set p1 1
set p2 1
set p3 1
set p4 1
set p5 1
set p6 1
set p7 1
set p8 1
set p9 1
set cap0 "\\%00:"
set cap1 "\\%01:"
set cap2 "\\%02:"
set cap3 "\\%03:"
set cap4 "\\%04:"
set cap5 "\\%05:"
set cap6 "\\%06:"
set cap7 "\\%07:"
set cap8 "\\%08:"
set cap9 "\\%09:"
set val0 0
set val1 0
set val2 0
set val3 0
set val4 0
set val5 0
set val6 0
set val7 0
set val8 0
set val9 0

set i 0
while {1} {
  set a [lindex $argv $i]
  if {[string length $a] == 0} break;
  if { [string compare $a "-x"] == 0 } {
    set i [expr $i + 1]
    set posx [lindex $argv $i]
  } elseif { [string compare $a "-y"] == 0 } {
    set i [expr $i + 1]
    set posy [lindex $argv $i]
  } else {
    break
  }
  set i [expr $i + 1]
}
set datalist [lindex $argv $i]
set i [expr $i + 1]
set script [lindex $argv $i]

loaddatalist
setvalue $figure
setupwindow

