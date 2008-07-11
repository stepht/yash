savepath=$PATH

echo ===== : true false =====

:
echo $?
true
echo $?
! false # "false" may return any non-zero status
echo $?

(
# a function may override a non-special builtin
false () { true; }
if false; then echo false true; fi

# a non-regular builtin should be executed if not in PATH
PATH=
if : && true && false; then
	PATH=$savepath
	echo : true false
else
	PATH=$savepath
fi
)

echo ===== return break continue =====

retfunc () {
	return 3
	echo retfunc ng
}
retfunc
echo $?

retfunc () {
	(exit 4)
	return
	echo retfunc ng 2
}
retfunc
echo $?

retfunc () {
	retfunc_inner () {
		return $1
	}
	retfunc_inner 5
	retfunc_inner=$?
	echo retfunc
	return $retfunc_inner
}
retfunc
echo $?

while true; do
	echo while ok
	break
	echo while ng
done
until false; do
	echo until ok
	break
	echo until ng
done
for i in 1 2 3 4; do
	echo for $i
	break
	echo for ng
done

i=0
while [ $i -eq 0 ]; do
	echo while $i
	i=1
	continue
	echo while ng
done
i=0
until [ $i -ne 0 ]; do
	echo until $i
	i=1
	continue
	echo until ng
done
for i in 1 2 3 4; do
	echo for $i
	continue
	echo for ng
done

for i in 1 2 3; do
	for j in 7 8 9; do
		echo $i $j
		if [ $i -eq 3 ]; then
			break 2
		elif [ $j -eq 8 ]; then
			continue 2
		fi
	done
done

k=0
for i in 1 2 3; do
	if true; then
		while true; do
			until false; do
				case $i in
					1)
					for j in 7 8 9; do
						echo $i $j $k
						if [ $k -ne 0 ]; then
							break 3
						fi
						k=1
					done
					continue 3
					;;
					2)
					while true; do
						until false; do
							echo i=2
							break 4
						done
					done
					;;
					*)
					echo i=3
					break 999
				esac
			done
		done
	fi
	echo !
done
echo done


echo ===== . =====

. ./dot.t
echo $count


echo ===== exec =====

$INVOKE $TESTEE -c 'exec echo exec'
$INVOKE $TESTEE -c 'exec /dev/null' 2>/dev/null
echo $?

exec echo exec echo
echo not reached
