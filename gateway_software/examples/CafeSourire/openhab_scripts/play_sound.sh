#!/bin/bash


curr_folder="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
was_playing=$(source $curr_folder/mpd_play_pause.sh)
# Use sox to play music : http://sox.sourceforge.net/sox.html
if [[ $2 == '-f' ]]
then
  while ! play $1 fade t 4 -0 4 > /dev/null 2>&1 
  do
    sleep 1
  done
else
  while ! play $1 > /dev/null 2>&1
  do
    sleep 1
  done
fi

if [ ! -z "$was_playing" ] && [ "$was_playing" == 'was_playing' ]
then
  sleep 3 
  source $curr_folder/mpd_play_pause.sh play
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
