#!/usr/bin/python

import sys
import os.path
from os import listdir 

####################
# global variables #
####################
nbPatternOccurence = 0
patternSize = 0 
# Dictionary of {'fileName' : '[fileDesc, fileContent ]'}
filesContent = {}
patternFile = ""

#####################
# static variables  #
#####################


def findPattern() :
    nullPattern=""
    line_buff={}
    
    for j in range(0, patternSize) :
        nullPattern += "0\n"

    filesContent[patternFile][0].seek(0)
    
    for i,line in enumerate(filesContent[patternFile][0]) : 
        if (patternSize > 1) and (patternSize - 2) not in line_buff:
            line_buff[i] = line
            continue
        else  :
            pattern = ""
            for j in range(0, patternSize - 1) :
                pattern += line_buff[j]
            pattern += line
            if pattern != nullPattern :
                isUniquePattern(filesContent[patternFile][1], pattern)
            for j in range(0, patternSize - 2) :
                line_buff[j] = line_buff[j+1]
            line_buff[patternSize - 2] = line

def isUniquePattern(patternFileContent, pattern):
    global nbPatternOccurence
    found=False
    
    for fileName in filesContent :   
        if str(fileName) != str(patternFile) :
            if pattern in filesContent[fileName][1]:
                #print('pattern [' + pattern.replace('\n', ' ') + '] found' + ' in ' + './modified_files/' + fileName )
                return
            else:
                #print('pattern [' + pattern.replace('\n', ' ')  + '] not found' + ' in ' + './modified_files/' + fileName)
                continue

    loc_nbPatternOccurence=patternFileContent.count(pattern)
    if nbPatternOccurence <  loc_nbPatternOccurence :
        nbPatternOccurence = loc_nbPatternOccurence
        print('new pattern [' + pattern.replace('\n', ' ') + '] is unique - nb occurences = ' + str(nbPatternOccurence))

def openFiles() :
    global filesContent
    global patternFile 

    if not os.path.exists("./modified_files_python"):
        os.makedirs("./modified_files_python")
    
    try:
        for i,fileName in enumerate(sys.argv[1:-1]) :
            inFile =  open(fileName, 'r')
            if i == 0 :
                patternFile="./modified_files_python/" + sys.argv[1].split("/")[-1] + ".extract"

            # each line of out file is a 10 base SPI byte. First line is removed 
            outFileName="./modified_files_python/" + fileName.split("/")[-1] + ".extract"
            outFile = open(outFileName, 'w+')
            
            for i,line in enumerate(inFile) :
                #remove first line containing header
                if i != 0 :
                    # last comma separated value is SPI byte
                    outFile.write(line.split(",")[-1])
            
            outFile.seek(0)
            filesContent[outFileName]=[outFile, outFile.read()]
            outFile.seek(0)

    except FileNotFoundError as err:
        print("{0}".format(err))
        exit(1)

def printHelp() :
    print("\nDESCRIPTION : ")
    print("  Try to find a pattern in PATTERN_FILE not matched in OTHER_FILES")
    print("  PATTERN_FILE and OTHER_FILES are some SPI (or other protocol) decoded frames from SALAE Logic capture") 
    print("PATTERN_SIZE is minimum size of pattern in bytes")
    print("\nSYNTAX :")
    print("    ./get_pattern.py PATTERN_FILE OTHER_FILES PATTERN_SIZE")

def checkArguments() :
    if len(sys.argv) <= 3 :
        print("bad syntax")
        printHelp()
        exit(1)     


def main():
    checkArguments()
    global patternSize

    try :
        patternSize = int(sys.argv[-1])
    except ValueError :
        print("Invalid pattern size given : " + sys.argv[-1])
        printHelp()
        exit(1)

    openFiles()
    
    print("Try to find a unique pattern for a pattern size of " + str(patternSize))
    findPattern()
    while nbPatternOccurence == 0 : 
        print("No unique pattern found for a pattern size of " + str(patternSize))
        patternSize += 1
        print("Try to find a unique pattern for a pattern size of " + str(patternSize))
        findPattern()

if __name__ == "__main__":
    main()

