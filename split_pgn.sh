#!/bin/bash

SRC=$1
DEST=$2

SRC_FILE_ONLY=$(basename "$SRC")

SRC_NAME="${SRC_FILE_ONLY%.*}"
SRC_EXT="${SRC_FILE_ONLY##*.}"

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
while read line
do
    if [[ -n `echo $line | grep "\[Event "` ]]
    then
        if [[ $COUNTER -ne 0 ]]
        then
            sed -i '${/^$/d;}' "$DEST/${SRC_NAME}_${COUNTER}.$SRC_EXT"
        fi
        ((COUNTER++))
        touch "$DEST/${SRC_NAME}_${COUNTER}.$SRC_EXT"
    fi
    echo $line >> "$DEST/${SRC_NAME}_${COUNTER}.$SRC_EXT"
done < $SRC
sed -i '${/^$/d;}' "$DEST/${SRC_NAME}_${COUNTER}.$SRC_EXT"
