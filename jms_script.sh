#!/bin/bash

TEMP=`getopt --long -o "c:l:" "$@"`
eval set -- "$TEMP"
while true ; do
    case "$1" in
        -l )
            thePath=$2
            shift 2
        ;;
        -c )
            theCommand=$2
            theSize=$4
            if [ $theCommand == "list" ] 
            	then dirs=($(find $thePath -type d))
            	for dir in "${dirs[@]}"; do
            	  echo $dir
            	done
            elif [ $theCommand == "size" ]
            	then du -h $thePath | sort -hr | head -n $theSize
            elif [ $theCommand == "purge" ] 
            	then find $thePath -mindepth 1 -delete
            else
            	echo "Wrong argument."
            fi
            shift 2
        ;;
        *)
            break
        ;;
    esac 
done;


#sed -i 's/\r$//' jms_script.sh