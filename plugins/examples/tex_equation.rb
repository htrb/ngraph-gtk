# Description: Te_X equation,import TeX equation as GRA,

TEX_COMMAND = "pdflatex"
TEX_OPTION = "-halt-on-error"

PSTOEDIT_COMMAND = "pstoedit"
PSTOEDIT_OPTION = "-q -flat 0.1 -nc -ssp -dt"

sys = Ngraph::System[0]
menu = Ngraph::Menu[0]

TEX_FILE = "_tex_eqn_#{sys::pid}"
require "#{sys.data_dir}/addin/fig2gra.rb"

text_obj = menu.focused('text')
if (text_obj.size < 1)
  Ngraph.err_puts("focus text object to convert.")
  exit
end

merge_ary = []

text_obj.each {|str|
  text = Ngraph.str2inst(str)[0]
  merge_name = text.name
  unless (merge_name)
    t = sprintf("%X", Time.now.to_i)
    merge_name = "tex_eqn_#{text.oid}_#{t}"
  end

  tex_file = "#{TEX_FILE}.tex"
  pdf_file = "#{TEX_FILE}.pdf"
  fig_file = "#{TEX_FILE}.fig"

  File.open(tex_file, "w") {|f|
    f.puts('\documentclass[12pt]{article}')
    f.puts('\usepackage{amsmath,txfonts}')
    f.puts('\begin{document}')
    f.puts('\pagestyle{empty}')
    f.puts('\[')
    f.puts(text.text)
    f.puts('\]')
    f.puts('\end{document}')
  }
  
  menu.echo(`#{TEX_COMMAND} #{TEX_OPTION} "#{tex_file}"`)
  menu.echo("---------------")
  menu.echo

  unless (FileTest.readable?(pdf_file))
    Ngraph.err_puts("Faital error occurred while executing #{TEX_COMMAND}.")
    File.delete(*Dir.glob("#{TEX_FILE}.*"))
    menu.show_window(5)
    exit
  end

  cmd = %Q!#{PSTOEDIT_COMMAND} #{PSTOEDIT_OPTION} -f fig "#{pdf_file}" "#{fig_file}"!
  system(cmd)
  unless (FileTest.readable?(fig_file))
    Ngraph.err_puts("Faital error occurred while executing #{PSTOEDIT_COMMAND}.")
    File.delete(*Dir.glob("#{TEX_FILE}.*"))
    exit
  end

  gra_file = "#{Dir.pwd}/#{merge_name}.gra"
  fig2gra = Fig2Gra.new
  fig2gra.convert(fig_file, gra_file) 
  unless (FileTest.readable?(gra_file))
    Ngraph.err_puts("Faital error occurred while executing fig2gra.")
    File.delete(*Dir.glob("#{TEX_FILE}.*"))
    exit
  end

  merge = Ngraph::Merge[merge_name][-1]
  unless (merge)
    merge = Ngraph::Merge.new
    merge.name = merge_name
    merge.file = gra_file
  end

  merge.left_margin = 0
  merge.top_margin = 0

  text_ary = text.bbox
  graf_ary = merge.bbox

  merge.left_margin = text_ary[0] - graf_ary[0]
  merge.top_margin = text_ary[1] - graf_ary[1]

  text.hidden = true
  text.raw = true
  text.name = merge_name

  merge_ary.push(merge)

  File.delete(*Dir.glob("#{TEX_FILE}.*"))
}

menu.unfocus
menu.draw

merge_ary.each {|merge|
  menu.focus(merge)
}
menu.modified = true
