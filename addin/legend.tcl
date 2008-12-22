#!/usr/bin/wish

#
# legend.tcl written by S. ISHIZAKA 2000/01
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
  global datalist filenum
  global filelist xlist ylist typelist marklist marksizelist
  global linewidthlist linestylelist
  global redlist greenlist bluelist red2list green2list blue2list
  global mathxlist mathylist
  global mixlist showlist captionlist
  if { [file exist $datalist] } {
    set f [ open $datalist r ]
    gets $f filenum
    for {set i 0} {$i < $filenum} {incr i 1} {
      set file [gets $f]
      set filelist [concat $filelist [list $file]]
      set xlist [concat $xlist [list [gets $f]]]
      set ylist [concat $ylist [list [gets $f]]]
      set typelist [concat $typelist [list [gets $f]]]
      set marklist [concat $marklist [list [gets $f]]]
      set marksizelist [concat $marksizelist [list [gets $f]]]
      set linewidthlist [concat $linewidthlist [list [gets $f]]]
      set linestylelist [concat $linestylelist [list [gets $f]]]
      set redlist [concat $redlist [list [gets $f]]]
      set greenlist [concat $greenlist [list [gets $f]]]
      set bluelist [concat $bluelist [list [gets $f]]]
      set red2list [concat $red2list [list [gets $f]]]
      set green2list [concat $green2list [list [gets $f]]]
      set blue2list [concat $blue2list [list [gets $f]]]
      set mathxlist [concat $mathxlist [list [gets $f]]]
      set mathylist [concat $mathylist [list [gets $f]]]
      set showlist [concat $showlist {1}]
      set captionlist [concat $captionlist [list [file tail $file]]]
      for {set j 0} {$j < $i} {incr j 1} {
        set a [string compare [lindex $filelist $i] [lindex $filelist $j]]
        set b [string compare [lindex $xlist $i] [lindex $xlist $j]]
        set c [string compare [lindex $ylist $i] [lindex $ylist $j]]
        set d [string compare [lindex $mathxlist $i] [lindex $mathxlist $j]]
        set e [string compare [lindex $mathylist $i] [lindex $mathylist $j]]
        if [ expr ($a==0) && ($b==0) && ($c==0) && ($d==0) && ($e==0) ] {
          set mixlist [concat $mixlist [list $j ]]
          break
        }
      }
      if { $j == $i } {
        set mixlist [concat $mixlist {-1}]
      }
    }
    close $f
  }
}

proc makescript { f i gx gy height } {
  global typelist marklist marksizelist
  global linewidthlist linestylelist
  global redlist greenlist bluelist red2list green2list blue2list
  global width

  set type [lindex $typelist $i]
  set mark [lindex $marklist $i]
  set marksize [lindex $marksizelist $i]
  set linewidth [lindex $linewidthlist $i]
  set linestyle [lindex $linestylelist $i]
  set R [lindex $redlist $i]
  set G [lindex $greenlist $i]
  set B [lindex $bluelist $i]
  set R2 [lindex $red2list $i]
  set G2 [lindex $green2list $i]
  set B2 [lindex $blue2list $i]

  if { ($type=="line") || ($type=="polygon") || ($type=="curve") \
    || ($type=="diagonal") || ($type=="errorbar_x") || ($type=="errorbar_y") \
    || ($type=="staircase_x") || ($type=="staircase_y") || ($type=="fit") } {
    puts $f "new line"
    puts $f [format "line::points=\'%d %d %d %d\'" \
            $gx [expr int($gy+$height*2/3)] \
            [expr $gx+$width] [expr int($gy+$height*2/3)]]
    puts $f [format "line::width=%d" $linewidth]
    if { $linestyle != "" } {
      puts $f [format "line::style=\'%s\'" $linestyle]
    }
    puts $f [format "line::R=%d" $R]
    puts $f [format "line::G=%d" $G]
    puts $f [format "line::B=%d" $B]
  } elseif { ($type=="arrow") } {
    puts $f "new line"
    puts $f [format "line::points=\'%d %d %d %d\'" \
            $gx [expr int($gy+$height*2/3)] \
            [expr $gx+$width] [expr int($gy+$height*2/3)]]
    puts $f [format "line::width=%d" $linewidth]
    if { $linestyle != "" } { 
      puts $f [format "line::style=\'%s\'" $linestyle]
    }
    puts $f [format "line::R=%d" $R]
    puts $f [format "line::G=%d" $G]
    puts $f [format "line::B=%d" $B]
    puts $f "line::arrow=end"
  } elseif { ($type=="rectangle") || ($type=="rectangle_fill") \
   || ($type=="rectangle_solid_fill") || ($type=="bar_x") || ($type=="bar_y") \
   || ($type=="bar_fill_x") || ($type=="bar_fill_y") \
   || ($type=="bar_solid_fill_x") || ($type=="bar_solid_fill_y") } {
    puts $f "new rectangle"
    puts $f [format "rectangle::x1=%d" $gx]
    puts $f [format "rectangle::y1=%d" [expr int($gy+$height*2/3-$height/2)]]
    puts $f [format "rectangle::x2=%d" [expr $gx+$width]]
    puts $f [format "rectangle::y2=%d" [expr in($gy+$height*2/3+$height/2)]]
    if { ($type=="rectangle") || ($type=="bar_x") || ($type=="bar_y") } {
      puts $f "rectangle::fill=false"
      puts $f [format "rectangle::R=%d" $R]
      puts $f [format "rectangle::G=%d" $G]
      puts $f [format "rectangle::B=%d" $B]
    } elseif { ($type=="rectangle_fill") || ($type=="bar_fill_x") \
     || ($type=="bar_fill_y") } {
      puts $f "rectangle::fill=true"
      puts $f "rectangle::frame=true"
      puts $f [format "rectangle::R=%d" $R2]
      puts $f [format "rectangle::G=%d" $G2]
      puts $f [format "rectangle::B=%d" $B2]
      puts $f [format "rectangle::R2=%d" $R]
      puts $f [format "rectangle::G2=%d" $G]
      puts $f [format "rectangle::B2=%d" $B]
    } elseif { ($type=="rectangle_solid_fill") \
     || ($type=="bar_solid_fill_x") || ($type=="bar_solid_fill_y") } {
      puts $f "rectangle::fill=true"
      puts $f [format "rectangle::R=%d" $R]
      puts $f [format "rectangle::G=%d" $G]
      puts $f [format "rectangle::B=%d" $B]
    }
    puts $f [format "rectangle::width=%d" $linewidth]
    if { $linestyle!="" } {
      puts $f [format "rectangle::style=\'%s\'" $linestyle]
    }
  } elseif { $type=="mark" } {
    puts $f "new mark"
    puts $f [format "mark::x=%d" [expr int($gx+$width/2)]]
    puts $f [format "mark::y=%d" [expr int($gy+$height*2/3)]]
    puts $f [format "mark::size=%d" $marksize]
    puts $f [format "mark::type=%d" $mark]
    puts $f [format "mark::width=%d" $linewidth]
    if { $linestyle!= "" } {
      puts $f [format "mark::style=\'%s\'" $linestyle]
    }
    puts $f [format "mark::R=%d" $R]
    puts $f [format "mark::G=%d" $G]
    puts $f [format "mark::B=%d" $B]
    puts $f [format "mark::R2=%d" $R2]
    puts $f [format "mark::G2=%d" $G2]
    puts $f [format "mark::B2=%d" $B2]
  }
}

proc savescript {} {
  global datalist filenum
  global typelist marklist marksizelist
  global linewidthlist linestylelist
  global redlist greenlist bluelist red2list green2list blue2list
  global mixlist showlist captionlist
  global mix type caption frame posx posy width
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
  set len 0
  set gy $posy
  for {set i 0} {$i < $filenum} {incr i 1} {
    if { (( [lindex $mixlist $i] == -1 ) || ( ! $mix )) \
         && [lindex $showlist $i] } {
      set gx $posx
      if { $type } {
        makescript $f $i $gx $gy $height
        for {set j [expr $i+1]} {$j < $filenum} {incr j 1} {
          if { ([lindex $mixlist $j] == $i) && $mix } {
            makescript $f $j $gx $gy $height
          }
        }
        set len [expr int($width+$height/2)]
      }
      if { $caption } {
        puts $f "new text"
        puts $f [format "text::text=\'%s'" [lindex $captionlist $i]]
        puts $f [format "text::x=%d" [expr $gx+$len]]
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
      set gy [expr int($gy+$height*1.2)]
    }
  }
  if { $frame } {
    puts $f "new rectangle"
    puts $f [format "rectangle::x1=%d" [expr int($posx-$height/4)]]
    puts $f [format "rectangle::y1=%d" $posy]
    puts $f [format "rectangle::x2=%d+\${int:texttot:@}" \
            [expr int($posx+$len+3*$height/4)]]
    puts $f [format "rectangle::y2=%d" [expr int($gy+$height/2)]]
    puts $f "rectangle::R=0"
    puts $f "rectangle::G=0"
    puts $f "rectangle::B=0"
    puts $f "rectangle::fill=true"
    puts $f "new rectangle"
    puts $f [format "rectangle::x1=%d" [expr int($posx-$height/2)]]
    puts $f [format "rectangle::y1=%d" [expr int($posy-$height/4)]]
    puts $f [format "rectangle::x2=%d+\${int:texttot:@}" \
            [expr int($posx+$len+$height/2)]]
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
  puts $f "menu::modified=TRUE"
  close $f
}

proc getcaption { w } {
  global filenum mixlist showlist captionlist 
  global mix show captiontext cursel

  if { $cursel >= 0 } {
    set j -1
    for {set i 0} {$i < $filenum} {incr i 1} {
      if { ( [lindex $mixlist $i] == -1 ) || ( ! $mix ) } {
        incr j 1
        if { $cursel == $j } {
          set captionlist [lreplace $captionlist $i $i $captiontext]
          set showlist [lreplace $showlist $i $i $show]
          break;
        }
      }
    }
  }
  set cursel -1
}

proc getcaption2 { w } {
  global filenum mixlist showlist captionlist 
  global mix show captiontext cursel

  if { $cursel >= 0 } {
    set j -1
    for {set i 0} {$i < $filenum} {incr i 1} {
      if { ( [lindex $mixlist $i] == -1 ) || ( $mix ) } {
        incr j 1
        if { $cursel == $j } {
          set captionlist [lreplace $captionlist $i $i $captiontext]
          set showlist [lreplace $showlist $i $i $show]
          break;
        }
      }
    }
  }
  set cursel -1
}

proc setcaption { w } {
  global filenum mixlist showlist captionlist 
  global mix show captiontext cursel

  set cursel [eval $w curselection]
  set j -1
  for {set i 0} {$i < $filenum} {incr i 1} {
    if { ( [lindex $mixlist $i] == -1 ) || ( ! $mix ) } {
      incr j 1
      if { $cursel == $j } {
        set captiontext [concat [lindex $captionlist $i]]
        set show [lindex $showlist $i]
        break;
      }
    }
  }
}

proc setupwindow {} {
  proc setlistbox {} {
    global mix filenum filelist xlist ylist mixlist sel
    .files delete 0 end
    for {set i 0} {$i < $filenum} {incr i 1} {
      if { ( [lindex $mixlist $i] == -1 ) || ( ! $mix ) } {
        .files insert end [format "%d %d %s" \
                  [lindex $xlist $i] [lindex $ylist $i] [file tail [lindex $filelist $i]]]
      }
    }
  }
  global filenum filelist xlist ylist mixlist captionlist showlist
  global mix type caption frame posx posy width show captiontext
  frame .top -relief raised -bd 1
  frame .bot -relief raised -bd 1
  pack .top -side top -fill both
  pack .bot -side bottom -fill both 

  frame .titlef -relief sunken -bd 1
  frame .formatf -relief groove -bd 2
  frame .positionf -relief groove -bd 2
  frame .captionf -bd 1
  frame .inputf
  frame .listf 
  pack .titlef -in .top -side top -fill x -pady 5m -padx 5m -anchor center
  pack .formatf .positionf .captionf -in .top -side top \
        -fill x -pady 1m -padx 5m -anchor w
  pack .inputf .listf -in .captionf -side left -padx 1m

  label .title -text "Legend version 1.01.00" 
  pack .title -in .titlef -side top -pady 1m -padx 5m -anchor center

  label .title2 -text "automatic legend generator" 
  pack .title2 -in .titlef -side top -pady 1m -padx 5m -anchor center

  checkbutton .mix -text "Mix" -variable mix
  checkbutton .type -text "Type" -variable type
  checkbutton .caption -text "Caption" -variable caption
  checkbutton .frame -text "Frame" -variable frame
  button .font -text "FONT" -command "fontdialog .f Font"
  pack .mix .type .caption .frame -in .formatf -side left -pady 2m -anchor w
  pack .font -in .formatf -side right -padx 5m -anchor e

  label .posxl -text " X:"
  entry .posx -width 8 -relief sunken -bd 2 -textvariable posx
  label .posyl -text " Y:"
  entry .posy -width 8 -relief sunken -bd 2 -textvariable posy
  label .widthl -text " Width:"
  entry .width -width 8 -relief sunken -bd 2 -textvariable width
  pack .posxl .posx .posyl .posy .widthl .width -in .positionf -side left \
       -pady 2m -anchor w

  checkbutton .show -text "Show" -variable show
  label .textl -text " Caption:"
  entry .text -width 20 -relief sunken -bd 2 -textvariable captiontext
  pack .show -in .inputf -side top -pady 5m -anchor w
  pack .textl .text -in .inputf -side top -pady 1m -anchor w

  listbox .files -width 20 -height 6 -relief raised -bd 2 -selectmode single \
          -yscrollcommand ".scroll set" 
  pack .files -in .listf -side left -anchor center 
  scrollbar .scroll -command ".files yview"
  pack .scroll -in .listf -side left -fill y

  bind .files <ButtonRelease> {getcaption .files; setcaption .files}
  .mix configure -command "getcaption2 .files; setlistbox"

  setlistbox

  button .ok -text "OK" -command "getcaption .files; savescript; exit"
  frame .default -relief sunken -bd 1
  raise .ok
  pack .default -side left -expand 1 -in .bot -padx 2m -pady 2m
  pack .ok -in .default -side left -padx 1m -pady 1m -ipadx 2m -ipady 1m

  button .cancel -text "Cancel" -command exit
  pack .cancel -in .bot -side left -expand 1 -padx 2m -pady 2m -ipadx 2m \
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
set textfont Helvetica
set textjfont Gothic

set posx 5000
set posy 5000
set width 3000
set mix 1
set type 1
set caption 1
set frame 1
set cursel -1

set filenum 0
set filelist ""
set xlist ""
set ylist ""
set typelist ""
set marklist ""
set marksizelist ""
set linewidthlist ""
set linestylelist ""
set redlist ""
set greenlist ""
set bluelist ""
set red2list ""
set green2list ""
set blue2list ""
set mathxlist ""
set mathylist ""
set showlist ""
set captionlist ""
set mixlist ""

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
  } elseif { [string compare $a "-w"] == 0 } {
    set i [expr $i + 1]
    set width [lindex $argv $i]
  } else {
    break
  }
  set i [expr $i + 1]
}
set datalist [lindex $argv $i]
set i [expr $i + 1]
set script [lindex $argv $i]

loaddatalist
setupwindow


