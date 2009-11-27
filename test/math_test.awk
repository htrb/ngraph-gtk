#! /usr/bin/awk

BEGIN {
    FS = "#";
    print("set +e");
    print;
}
{
    print("RESULT=`dexpr '" $1 "'`");
    print("if [ \"$RESULT\" != \"" $2 "\" ]");
    print("then");
    print("  echo");
    print("  echo error '" $1 "'");
    print("  echo '    expected:' " $2);
    print("  echo '      result:' $RESULT");
    print("  echo '  difference:' `dexpr $RESULT-" $2 "`");
    print("fi\n");
}
END {
    print("del system");
}
