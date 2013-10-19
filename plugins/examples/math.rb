# Line-calculator using shell-builtin command "dexpr".
#
# Description: _Math...,calculator,

menu = Ngraph::Menu[0]
Ngraph::Math.new {|math|
  Ngraph::Dialog.new {|dialog|
    eqn = ""
    dialog.caption = 'Input mathematical expression'
    while true
      eqn = dialog.input(eqn)

      break unless (eqn)
      break if (eqn.length < 1)

      math.formula = eqn
      if (math.formula == eqn)
        result = math.calc
        str = sprintf("%G", result)
        dialog.message("#{eqn}=#{str}")
        if (menu)
          menu.echo("Math: #{eqn}=#{str}")
        else
          puts("#{eqn} = #{str}")
        end
        break
      end
    end
  }
}
