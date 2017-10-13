#!/bin/bash

# check an mpd server is found
if ! mpc  > /dev/null 2>&1 ; then
    echo "Cannot connect server" 
    exit 1
fi

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
