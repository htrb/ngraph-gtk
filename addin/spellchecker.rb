# -*- coding: utf-8 -*-
# Description: _Spell check,check spelling of legend text,

require 'rubygems'

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
  def initialize
    @speller = Aspell.new
    @speller.suggestion_mode = Aspell::NORMAL
    @speller.set_option("ignore-case", "true")

    @alphabet = Regexp.new("[A-Za-zÀ-ʯ]")
    @menu = Ngraph::Menu[0]

    @ignore = {}
    @apply = {}
  end

  def focused
    return nil unless (@menu)
    inst = @menu.focused("text")
    return nil if (inst.empty?)
    inst
  end

  begin
    require 'gtk3'

    ABORT = 1
    IGNORE = 2
    APPLY = 3
    IGNORE_ALL = 4
    APPLY_ALL = 5
    def show_dialog(title, caption, text, item)
      combo = Gtk::ComboBoxText.new(:entry => true)
      item.each {|str|
        combo.append(str, str)
      }
      combo.child.text = item[0] ? item[0] : text
      label = Gtk::Label.new(caption)

      dialog = Gtk::Dialog.new(:title => title,
                               :buttons => [["_Ignore all", IGNORE_ALL],
                                            ["_Ignore", IGNORE],
                                            ["_Apply all", APPLY_ALL],
                                            ["_Apply", APPLY]])
      dialog.default_response = Gtk::ResponseType::APPLY
      dialog.content_area.pack_start(label)
      dialog.content_area.pack_start(combo)
      dialog.show_all
      r = dialog.run
      s = combo.active_text
      dialog.destroy
      [r, s]
    end

    def spell_check(original_string, id, word)
      str = nil
      return word if (@speller.check(word))
      return word if (@ignore[word])
      return @apply[word] if (@apply[word])

      response, str = show_dialog("spell check (text:#{id})", 
                                  "#{original_string}\nPossible correction for '#{word}':",
                                  word,
                                  @speller.suggest(word))
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
    end
  rescue LoadError
    def spell_check(original_string, id, word)
      str = nil
      return word if (@speller.check(word))
      return word if (@ignore[word])
      Ngraph::Dialog.new {|dialog|
        dialog.title = "spell check (text:#{id})"
        dialog.caption = "#{original_string}\nPossible correction for '#{word}':"
        str = dialog.combo_entry(@speller.suggest(word))
        @ignore[word] = true unless (str)
      }
      str
    end
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
    str = nil
    str = spell_check(original_string, id, word) if (word.size > 1)
    if (str)
      modified_string << str
    else
      modified_string << word
    end
    word.clear
  end

  def check(text)
    original_string = text.text
    modified_string = ""
    word = ""

    len = original_string.size
    i = 0
    while (i < len)
      c = original_string[i]
      case (c)
      when '\\'
        check_word(original_string, text.id, modified_string, word)
        modified_string << c
        i += 1
        c = original_string[i]
        if (c)
          modified_string << c
          i += 1
        end
      when '%'
        check_word(original_string, text.id, modified_string, word)
        modified_string << c
        i = skip_prm(original_string, i, modified_string)
      when @alphabet
        word << c
        i += 1
        check_word(original_string, text.id, modified_string, word) if (i >= len)
      else
        check_word(original_string, text.id, modified_string, word)
        modified_string << c
        i += 1
      end
    end

    if (original_string != modified_string)
      text.text = modified_string
      @menu.modified = true if (@menu)
    end
  end
end

checker = NgraphSpellchecker.new

focused = checker.focused
if (focused)
  focused.each {|inst|
    Ngraph.str2inst(inst).each {|text|
      checker.check(text)
    }
  }
else
  Ngraph::Text.each {|text|
    checker.check(text)
  }
end

Ngraph::Dialog.new {|dialog|
  dialog.title = "spell check"
  str = dialog.message("Spell check completed.")
}
