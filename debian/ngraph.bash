# ngraph completion 
# put this file in /etc/bash_completion.d/ 

have ngraph &&
_ngraph()
{
    local cur

    COMPREPLY=()
    cur=${COMP_WORDS[COMP_CWORD]}
    prev=${COMP_WORDS[COMP_CWORD-1]}

    case "$prev" in
        -png|-pdf|-ps|-eps|-svg|-gra|-p)
            _filedir 'ngp'
	    return 0
            ;;
	-ex|-ey)
            COMPREPLY=( $( compgen -W 'linear log inverse' -- $cur ) ) 
	    return 0
            ;;
	-d)
	    COMPREPLY=( $( compgen -W 'mark line polygon curve diagonal \
		arrow rectangle rectangle_fill rectangle_solid_fill \
		errorbar_x errorbar_y staircase_x staircase_y bar_x bar_y \
		bar_fill_x bar_fill_y bar_solid_fill_x bar_solid_fill_y fit' \
		-- $cur ) )
	    return 0
	    ;;
	-m)
	    COMPREPLY=( $( compgen -W "`seq 0 89`" -- $cur ) )
	    return 0
	    ;;
	-x|-y|-o|-l|-w|-cr|-cg|-cb|-CR|-CG|-CB|-s|-r|-f|-vx|-vy|-mx|-my|-minx|-maxx|-incx|-miny|-maxy|-incy)
	    return 0
	    ;;
    esac

    case "$cur" in
        -*)
            COMPREPLY=( $( compgen -W '-h --help --version \
		-x -y -X -Y -d -m -o -l -w -cr -cg -cb -CR -CG -CB -s -r -f \
		-vx -vy -mx -my -ex -ey -minx -maxx -incx -miny -maxy -incy -g \
		-png -pdf -ps -eps -svg -gra -n -p -dialog' -- $cur ) ) 
            ;;
	*)
	    _filedir
	    ;;
    esac
}
[ "${have:-}" ] && complete -F _ngraph $filenames ngraph


have ngp2 &&
_ngp2()
{
    local cur

    COMPREPLY=()
    cur=${COMP_WORDS[COMP_CWORD]}
    case "$cur" in
        -*)
            COMPREPLY=( $( compgen -W '-I -a -A -c \
		-ps -ps3 -ps2 -eps -eps3 -eps2 \
		-wmf -pdf -svg -svg1.1 -svg1.2 -png' -- $cur ) ) 
            ;;
	*)
	    _filedir 'ngp'
	    ;;
    esac
}
[ "${have:-}" ] && complete -F _ngp2 $filenames ngp2


have gra2ps &&
_gra2ps()
{
    local cur

    COMPREPLY=()
    cur=${COMP_WORDS[COMP_CWORD]}
    prev=${COMP_WORDS[COMP_CWORD-1]}

    case "$prev" in
        -o)
            _filedir '@(ps|eps)'
	    return 0
            ;;
	-i)
            _filedir 'ps'
	    return 0
            ;;
	-p)
	    COMPREPLY=( $( compgen -W 'a3 a4 b4 a5 b5 letter legal' -- $cur ) )
	    return 0
	    ;;
	-s)
	    return 0
	    ;;
    esac

    case "$cur" in
        -*)
            COMPREPLY=( $( compgen -W '-o -i -c -e -p -l -r' -- $cur ) ) 
            ;;
	*)
	    _filedir 'gra'
	    ;;
    esac
}
[ "${have:-}" ] && complete -F _gra2ps $filenames gra2ps

have gra2wmf &&
_gra2wmf()
{
    local cur

    COMPREPLY=()
    cur=${COMP_WORDS[COMP_CWORD]}
    prev=${COMP_WORDS[COMP_CWORD-1]}

    case "$prev" in
        -o)
            _filedir 'wmf'
	    return 0
            ;;
	-d)
	    return 0
	    ;;
    esac

    case "$cur" in
        -*)
            COMPREPLY=( $( compgen -W '-o -d' -- $cur ) ) 
            ;;
	*)
	    _filedir 'gra'
	    ;;
    esac
}
[ "${have:-}" ] && complete -F _gra2wmf $filenames gra2wmf
