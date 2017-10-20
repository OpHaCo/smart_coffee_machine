#!/bin/bash


curr_folder="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Use sox to play music : http://sox.sourceforge.net/sox.html
if [[ $2 == '-f' ]]
then
  play $1 fade t 4 -0 4  
else
  play $1 
fi

#Info for openhab
echo "Sound $1 successfully played"
exit 0
