#!/bin/bash

if [[ $1 == *.mp3 ]]
then
  mpg123 $1 > /dev/null 2>&1
else
  aplay $1 > /dev/null 2>&1
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
