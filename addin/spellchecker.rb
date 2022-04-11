# -*- coding: utf-8 -*-
# Description: _Spell check,check spelling of legend text,

begin
  require 'raspell'
rescue LoadError
  Ngraph::Dialog.new {|dialog|
    dialog.title = "spell check"
    dialog.message("Cannot load 'raspell'.")
    exit
  }
end

class NgraphSpellchecker
  ABORT = 1
  IGNORE_ALL = 2
  IGNORE = 3
  APPLY = 4
  APPLY_ALL = 5
  DICT_LANG = "en_US"

  def initialize
    snap = ENV["SNAP"]
    p snap
    if (snap)
      @speller = Aspell.new1("lang" => DICT_LANG, "prefix" => "#{snap}/usr")
    else
      @speller = Aspell.new(DICT_LANG)
    end
    @speller.suggestion_mode = Aspell::NORMAL
    @speller.set_option("ignore-case", "true")

    @alphabet = Regexp.new("[A-Za-zÀ-ʯ]")
    @menu = Ngraph::Menu[0]

    @ignore = {}
    @apply = {}

    @modified = false
    @x = nil
    @y = nil
  end

  def focused
    return nil unless (@menu)
    inst = @menu.focused("text")
    if (inst.empty?)
      nil
    else
      inst.map {|inst| Ngraph.str2inst(inst)[0]}
    end
  end

  def spell_check(original_string, id, word)
    str = nil
    return word if (@speller.check(word))
    return word if (@ignore[word])
    return @apply[word] if (@apply[word])
    Ngraph::Dialog.new {|dialog|
      dialog.title = "spell check (text:#{id})"
      dialog.buttons = ["_Abort", "_Ignore all", "_Ignore", "_Apply all", "_Apply"]
      dialog.caption = "#{original_string}\nPossible correction for '#{word}':"
      dialog.x = @x if (@x)
      dialog.y = @y if (@y)
      str = dialog.combo_entry(@speller.suggest(word))
      response = dialog.response_button
      @x = dialog.x
      @y = dialog.y
      case response
      when IGNORE
        word
      when IGNORE_ALL
        @ignore[word] = true if (str)
        word
      when APPLY
        str
      when APPLY_ALL
        @apply[word] = str if (str)
        str
      when ABORT
        nil
      else
        word
      end
    }
  end

  def skip_bracket(original_string, i, modified_string)
    chr1 = original_string[i]
    chr2 = case (chr1)
           when '['
             ']'
           when '{'
             '}'
           end

    nest = 0
    len = original_string.size
    while (i < len - 1)
      i += 1
      chr = original_string[i]
      break unless (chr)

      modified_string << chr
      if (chr == chr1)
        nest += 1
      elsif (chr == chr2)
        nest -= 1
        break if (nest < 0)
      end
    end
    return i + 1
  end

  def skip_prm(original_string, i ,modified_string)
    i += 1
    chr = original_string[i]
    return i unless (chr)

    modified_string << chr
    return skip_bracket(original_string, i, modified_string) if (chr == '[' || chr == '{')

    i += 1
    chr = original_string[i]
    return i unless (chr)

    while (chr =~ @alphabet)
      modified_string << chr
      i += 1
      chr = original_string[i]
    end

    return i unless (chr)
    modified_string << chr
    return skip_bracket(original_string, i, modified_string) if (chr == '{')

    return i
  end

  def check_word(original_string, id, modified_string, word)
    str = word
    str = spell_check(original_string, id, word) if (word.size > 1)
    if (str)
      modified_string << str
    else
      modified_string << word
    end
    word.clear
    str
  end

  def check(text)
    original_string = if (text.instance_of?(Ngraph::Text))
			text.text
		      else
			text.title
		      end
    modified_string = ""
    word = ""

    len = original_string.size
    i = 0
    r = true
    while (i < len)
      c = original_string[i]
      case (c)
      when '\\'
        r = check_word(original_string, text.id, modified_string, word)
        modified_string << c
        i += 1
        c = original_string[i]
        if (c)
          modified_string << c
          i += 1
        end
      when '%'
        r = check_word(original_string, text.id, modified_string, word)
        modified_string << c
        i = skip_prm(original_string, i, modified_string)
      when @alphabet
        word << c
        i += 1
        r = check_word(original_string, text.id, modified_string, word) if (i >= len)
      else
        r = check_word(original_string, text.id, modified_string, word)
        modified_string << c
        i += 1
      end
      unless (r)
        lest = original_string[i, len - i]
        modified_string << lest if (lest)
        break
      end
    end

    if (original_string != modified_string)
      if (text.instance_of?(Ngraph::Text))
	text.text = modified_string
      else
	text.title = modified_string
      end
      @modified = true
    end
    r
  end

  def check_inst(inst)
    inst.each {|text|
      break unless (check(text))
    }
  end

  def check_all
    Ngraph::Text.each {|text|
      break unless (check(text))
    }
    Ngraph::Parameter.each {|text|
      break unless (check(text))
    }
  end

  def run
    inst = focused
    if (inst)
      check_inst(inst)
    else
      check_all
    end
    finalize
  end

  def finalize
    Ngraph::Dialog.new {|dialog|
      dialog.title = "spell check"
      dialog.message("Spell check completed.")
    }
    if (@menu && @modified)
      @menu.modified = true
      @menu.draw
    end
  end
end

checker = NgraphSpellchecker.new
checker.run
