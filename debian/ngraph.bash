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
        -png|-pdf|-ps|-eps|-svg|-gra)
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
    esac

    case "$cur" in
        -*)
            COMPREPLY=( $( compgen -W '-h --help --version \
		-x -y -X -Y -d -m -o -l -w -c -C -s -r -f \
		-vx -vy -mx -my -ex -ey -zx -zy -g \
		-png -pdf -ps -eps -svg -gra -n' -- $cur ) ) 
            ;;
	*)
	    _filedir
	    ;;
    esac
}
[ "${have:-}" ] && complete -F _ngraph $filenames ngraph
