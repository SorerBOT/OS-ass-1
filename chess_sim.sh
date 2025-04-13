#!/bin/bash

SRC=$1
FILE_DOES_NOT_EXIST_ERROR_TXT="File does not exist: $SRC"

if [[ ! -f $SRC ]]
then
    echo $FILE_DOES_NOT_EXIST_ERROR_TXT
    exit 1
fi

# priting metadata
echo "Metadata from PGN file:"
while read line
do
    if [[ -n `echo $line | grep "\["` ]]
    then
        echo $line
    fi
done < $SRC
# printing game

MOV_ARRAY=($(python3 parse_moves.py "$(<$SRC)"))
MOV_COUNTER=0
TOTAL_MOV=${#MOV_ARRAY[@]}
INITIAL_BOARD=(
  "rnbqkbnr"
  "pppppppp"
  "........"
  "........"
  "........"
  "........"
  "PPPPPPPP"
  "RNBQKBNR"
)
WORKING_BOARD=("${INITIAL_BOARD[@]}")

print_board() {
    local -n _BOARD=$1

    echo "Move $MOV_COUNTER/$TOTAL_MOV"
    echo "  a b c d e f g h"

    ROW_IDX=8
    for row in "${_BOARD[@]}"
    do
        local row_printable=""
        for (( i = 0; i < ${#row}; i++))
        do
            row_printable+="${row:i:1} "
        done
        echo $ROW_IDX $row_printable $ROW_IDX
        ((ROW_IDX--))
    done
    echo "  a b c d e f g h"
}

transform_move_to_idx() {
    local _MOVE=$1

    local _COLUMN_CHAR="${_MOVE:0:1}"
    local _ROW_CHAR="${_MOVE:1:1}"
    local _COLUMN_NUM
    _COLUMN_NUM=$(printf "%d" "'$_COLUMN_CHAR")

    local _ROW_IDX=$((8 - _ROW_CHAR))
    local _COLUMN_IDX=$((_COLUMN_NUM - 97))

    echo "$_ROW_IDX$_COLUMN_IDX"
}

perform_move() {
    local _MOV=$1

    local _FROM="${_MOV:0:2}"
    local _TO="${_MOV:2:2}"
    local _PROMOTION_PIECE="${_MOV:4:1}"

    local _FROM_DATA
    local _TO_DATA
    _FROM_DATA=$(transform_move_to_idx $_FROM)
    _TO_DATA=$(transform_move_to_idx $_TO)

    local _FROM_ROW_IDX="${_FROM_DATA:0:1}"
    local _FROM_COLUMN_IDX="${_FROM_DATA:1:1}"

    local _TO_ROW_IDX="${_TO_DATA:0:1}"
    local _TO_COLUMN_IDX="${_TO_DATA:1:1}"

    local _PIECE="${WORKING_BOARD[$_FROM_ROW_IDX]:_FROM_COLUMN_IDX:1}"

    # promotion
    if [[ -n "$_PROMOTION_PIECE" ]]
    then
        _PIECE="$_PROMOTION_PIECE"
    fi

    # castling
    if [[ $_PIECE == "K" ||  $_PIECE == "k" ]]
    then
        local _COLUMN_DIFF=$((_TO_COLUMN_IDX - _FROM_COLUMN_IDX))
        local _COLUMN_DISTANCE=${_COLUMN_DIFF#-}

        if (( _COLUMN_DISTANCE == 2 ))
        then
            if (( _COLUMN_DIFF > 0 ))
            then
                local _CASTLING_PIECE="${WORKING_BOARD[$_FROM_ROW_IDX]:_TO_COLUMN_IDX+1:1}"
                WORKING_BOARD[$_TO_ROW_IDX]="${WORKING_BOARD[$_TO_ROW_IDX]:0:_TO_COLUMN_IDX+1}.${WORKING_BOARD[$_TO_ROW_IDX]:_TO_COLUMN_IDX+2}"
                WORKING_BOARD[$_TO_ROW_IDX]="${WORKING_BOARD[$_TO_ROW_IDX]:0:_TO_COLUMN_IDX-1}$_CASTLING_PIECE${WORKING_BOARD[$_TO_ROW_IDX]:_TO_COLUMN_IDX}"
            else
                local _CASTLING_PIECE="${WORKING_BOARD[$_FROM_ROW_IDX]:_TO_COLUMN_IDX-2:1}"
                WORKING_BOARD[$_TO_ROW_IDX]="${WORKING_BOARD[$_TO_ROW_IDX]:0:_TO_COLUMN_IDX-2}.${WORKING_BOARD[$_TO_ROW_IDX]:_TO_COLUMN_IDX-1}"
                WORKING_BOARD[$_TO_ROW_IDX]="${WORKING_BOARD[$_TO_ROW_IDX]:0:_TO_COLUMN_IDX+1}$_CASTLING_PIECE${WORKING_BOARD[$_TO_ROW_IDX]:_TO_COLUMN_IDX+2}"
            fi
        fi
    fi

    # en passant
    if [[ $_PIECE == 'P' || $_PIECE == 'p' ]]
    then
        if (( _FROM_COLUMN_IDX != _TO_COLUMN_IDX ))
        then
            local _CAPTURED_PIECE="${WORKING_BOARD[$_TO_ROW_IDX]:_TO_COLUMN_IDX:1}"
            if [[ $_CAPTURED_PIECE == "." ]]
            then
                WORKING_BOARD[$_FROM_ROW_IDX]="${WORKING_BOARD[$_FROM_ROW_IDX]:0:_TO_COLUMN_IDX}.${WORKING_BOARD[$_FROM_ROW_IDX]:_TO_COLUMN_IDX+1}"
            fi
        fi
    fi

    WORKING_BOARD[$_TO_ROW_IDX]="${WORKING_BOARD[$_TO_ROW_IDX]:0:_TO_COLUMN_IDX}$_PIECE${WORKING_BOARD[$_TO_ROW_IDX]:_TO_COLUMN_IDX+1}"
    WORKING_BOARD[$_FROM_ROW_IDX]="${WORKING_BOARD[$_FROM_ROW_IDX]:0:_FROM_COLUMN_IDX}.${WORKING_BOARD[$_FROM_ROW_IDX]:_FROM_COLUMN_IDX+1}"
}

calculate_board_after_n_steps() {
    local _STEPS_COUNT=$1

    WORKING_BOARD=("${INITIAL_BOARD[@]}")

    for ((idx=0; idx < _STEPS_COUNT; idx++))
    do
        perform_move ${MOV_ARRAY[$idx]}
    done
}
print_board INITIAL_BOARD
 
while true
do

    echo -n "Press 'd' to move forward, 'a' to move back, 'w' to go to the start, 's' to go to the end, 'q' to quit: "
    read -p "" CMD

    if [[ $CMD == 'd' ]]
    then
        if (( MOV_COUNTER == TOTAL_MOV ))
        then
            echo "No more moves available."
            continue
        fi
        ((MOV_COUNTER++))
        calculate_board_after_n_steps $MOV_COUNTER
        print_board WORKING_BOARD
    elif [[ $CMD == "q" ]]
    then
        echo "Exiting."
        echo "End of game."
        exit 0
    elif [[ $CMD == "a" ]]
    then
        if (( MOV_COUNTER > 0 ))
        then
            ((MOV_COUNTER--))
        fi
        calculate_board_after_n_steps $MOV_COUNTER
        print_board WORKING_BOARD
    elif [[ $CMD == "s" ]]
    then
        MOV_COUNTER=$TOTAL_MOV
        calculate_board_after_n_steps $MOV_COUNTER
        print_board WORKING_BOARD
    elif [[ $CMD == "w" ]]
    then
        MOV_COUNTER=0
        calculate_board_after_n_steps $MOV_COUNTER
        print_board WORKING_BOARD
    else
        echo "Invalid key pressed: $CMD"
    fi
done
