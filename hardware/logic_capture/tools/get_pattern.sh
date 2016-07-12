#!/bin/sh
echo "Get patterns"
echo "Convert SPI decoded files to needed format"

patternFile=$1
maxNbOcc=0

function echoHelp
{
  echo "Description : "
  echo "  Try to find a pattern in PATTERN_FILE not matched in OTHER_FILES"
  echo "  PATTERN_FILE and OTHER_FILES are some SPI (or other protocol) decoded frames from SALAE Logic capture" 
  echo "Syntax :"
  echo "    ./get_pattern.sh PATTERN_FILE OTHER_FILES"
}

function isUniquePattern
{
  #echo -e "try to find pattern $2"
  echo -e $1 | awk -F " " '{split($0, a, ":");  if(a[2] > 0) { exit 1}}' 
  if [ $? -eq 0 ]
  then
    nbOcc=$(pcregrep -c -M "$2" ./${patternFile}.extract) 
    if [ $nbOcc -gt $maxNbOcc ]
    then  
      maxNbOcc=$nbOcc
      echo -e "\nPattern : [$(echo $2)] is unique - nb occurences : $nbOcc" 
    fi
  fi
}

if [ "$#" -lt 2 ]; then
  echo -e "Error : Some valid Logic SPI files must be given as parameter\n"
  echoHelp
  exit 1    
fi

if [ ! -d ./modified_files ] 
then   
  echo "create temp folder ./modified_files"
  mkdir modified_files
fi

extractedFileList=""

for file in $@ 
do
  if ! [ -a $file ]
  then  
    echo "$file do not exist"
  else 
    echo "extract  $file"
    # each line is a 10 base SPI byte. First line is removed
    awk -F "\n" '{split($1, a, ","); if (NR!=1) {print a[4]}}' $file > "./modified_files/${file}.extract"
    extractedFileList=$(echo "$extractedFileList ${file}.extract")
  fi
done

cd ./modified_files
count=$(grep -c "" "./$1.extract")

while [ $count -ne 2 ]
do
  
  #pattern=$(sed -n "$(($count - 2)),${count}p" "./$1.extract")
  pattern=$(tail -n $(($count - 2)) "./$1.extract" | head -n 3)
  #echo $pattern
  #echo after sed $(($(date +%s%3N) - $date1))
  if [ "$pattern" != "$(echo -e "0\n0\n0\n")" ] && [ "$pattern" != "$(echo -e "255\n255\n255\n")" ]
  then
    #echo "try to find pattern $1.extract"
    isUniquePattern "$(pcregrep  --exclude=$1.extract  -c -M "$pattern" $extractedFileList)" "$pattern"
  fi
  count=$(($count-1))
done

exit 0
