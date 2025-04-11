#!/bin/bash

SRC=$1
DEST=$2
SRC_NAME="${SRC%.*}"
SRC_EXT="${SRC##*.}"

INVALID_ARGUMENT_COUNT_ERROR_TXT="Usage: $0 <source_pgn_file> <destination_directory>"
FILE_DOES_NOT_EXIST_ERROR_TXT="Error: File '$SRC' does not exist."

CREATED_DIR_TXT="Created directory '$DEST'."

if [[ -n $3 || -z $2 ]]
then
    echo $INVALID_ARGUMENT_COUNT_ERROR_TXT
    exit 1
fi

if [[ ! -f $SRC ]]
then
    echo $FILE_DOES_NOT_EXIST_ERROR_TXT
    exit 1
fi

if [[ ! -d $DEST ]]
then
    mkdir $DEST
    echo $CREATED_DIR_TXT
fi

COUNTER=0
cat $SRC | while read line
do
    if [[ -n `echo $line | grep "\[Event "` ]]
    then
        if [[ $COUNTER -ne 0 ]]
        then
            sed '${/^$/d;}' "$DEST/${SRC_NAME}_${COUNTER}.$SRC_EXT" > "$DEST/${SRC_NAME}_${COUNTER}.$SRC_EXT"
        fi
        ((COUNTER++))
    fi
    echo $line >> "$DEST/${SRC_NAME}_${COUNTER}.$SRC_EXT"
done
