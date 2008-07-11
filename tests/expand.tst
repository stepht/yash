[ ~+ = $PWD ] && echo \~+
cd .
[ ~- = $OLDPWD ] && echo \~-

var=123/456/789 asterisks='*****'

echo expand.t${{asterisks#"**"}%"**"}st
echo ${#$(echo a)} ${${`echo abc`#a}%c}

echo ${var/456/xxx}
echo ${var/#123/xxx} ${var/#456/xxx}
echo ${var/%789/xxx} ${var/%456/xxx}
echo ${var//\//-} ${var//5//} ${var///-}
echo ${var:/123*789/x}

$INVOKE $TESTEE -c 'echo "${#*}"' x 1 22 333 4444 55555
$INVOKE $TESTEE -c 'echo "${#@}"' x 1 22 333 4444 55555

echo $((echo foo); (echo bar))

unset IFS
var=1-2-3
echo $var ${IFS= -} $var
