#! /bin/sh

target=$1 ; shift

die() {
    echo "mk-help fatal error:  $*"
    trap '' 0
    rm -f ${target}-tmp *.HELP
    exit 1
} 1>&2

mkhelp() {
    ./$1 $2 2>&1 | grep -v illegal > $3.HELP
    if cmp -s $3.HELP $3.txt
    then rm -f $3.HELP
    else mv -f $3.HELP $3.txt
         echo $3 has been updated
    fi
}

touch ${target}-tmp
trap "rm -f ${target}-tmp *.HELP" 0

for f in "$@"
do
    test -x "$f" || die "$f program has not been built"
    mkhelp $f --help $f-full-help
    mkhelp $f --error-bogus-option-name $f-short-help
done

mv -f ${target}-tmp ${target}
trap '' 0
exit 0
