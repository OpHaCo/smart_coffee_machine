#!/bin/sh
# Say some words in a given langage
if [ -z "$1" ]
then
  echo "no language given"
  exit 1
fi

if [ -z "$2" ] 
then
  echo "No words given"
fi

pico2wave -l "$1" -w /tmp/sound_temp.wav "$2"

if [ "$?" -ne 0 ]
then
  echo "Unable to generate wav"
  exit 1
fi
play /tmp/sound_temp.wav > /dev/null 2>&1
exit 0
