#!/bin/bash
soundFile="./sounds/coucou_${1}.wav"
if [[ -f $soundFile ]]
then
  ./configurations/helper_scripts/play_sound.sh $soundFile
else
  ./configurations/helper_scripts/play_sound.sh "./sounds/coucou.wav"
fi
