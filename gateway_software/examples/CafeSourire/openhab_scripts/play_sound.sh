#!/bin/bash

# Use sox to play music : http://sox.sourceforge.net/sox.html
if [[ $2 == '-f' ]]
then
  play $1 fade t 4 -0 4 > /dev/null 2>&1 
else
  play $1 > /dev/null 2>&1
fi

if [ $? -ne 0 ]
then
  #Info for openhab
  echo "Unable to play sound $1 - error : $?"
  exit 1
fi
#Info for openhab
echo "Sound $1 successfully played"
exit 0
