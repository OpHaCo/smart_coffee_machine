#!/bin/bash
for i in $(mpc --format ""); do
    a=$i
    break
done
if [ $a == "[playing]" ]; then
    mpc pause > /dev/null
		echo 'was_playing'
elif [ ! -z $0 ] && [ $1 == 'play' ]
then 
		mpc play > /dev/null
fi
