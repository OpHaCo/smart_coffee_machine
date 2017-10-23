#!/bin/bash


curr_folder="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Use sox to play music : http://sox.sourceforge.net/sox.html
if [[ $2 == '-f' ]]
then
  if ! play $1 fade t 4 -0 4  
  then
    echo "Cannot play sound $1"
    exit 1
  fi
else
  if ! play $1 
  then
    echo "Cannot play sound $1"
    exit 1
  fi
fi

#Info for openhab
echo "Sound $1 successfully played"
exit 0
