#!/bin/bash

#warning will delete all *.sqb files in output folder

#input file of links and output folder where the downloading and uncompressing happens
#relitive links are wrt to where this script is
input="basestationlinks"
outputdir="../basestation"

#get script path
SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname "$SCRIPT")
cd "$SCRIPTPATH"

#create a basestation path and go to it
mkdir -p "$outputdir"
cd "$outputdir"

#remove any sqb files in the output folder
rm "$outputdir"/*.[sS][qQ][bB]

#try and download one of the files and decompress it to the output folder
filename=""
while IFS= read -r line
do
  line=$(echo "$line" | sed 's/^[ \t]*//;s/[ \t]*$//')
  if [ -z "$line" ]; then
    continue
  fi
  wget_output=$(wget -nv "$line" 2>&1 )
  if [ $? -eq 0 ]; then
    filename=$(echo "$wget_output" |cut -d\" -f2)
    echo "downloaded $line ok as $filename"
    MIMETYPE=$(file --mime-type "$filename" |sed -n -e 's/^.*:\s//p')
    if echo "$MIMETYPE" | grep -q 'application/zip' ; then
      echo "file is a zip file"
      unzip "$filename" -d "$outputdir"
      rm "$filename"
      echo "saved uncompressed files to "$(realpath "$outputdir")
      break
    fi
    if  echo "$MIMETYPE" | grep -q 'application/gzip' ; then
      echo "file is a gzip file"
      tar -zxf "$filename" -C "$outputdir"
      rm "$filename"
      echo "saved uncompressed files to "$(realpath "$outputdir")
      break
    fi
    echo "error: file has a mime type of $MIMETYPE and is not a zip/gzip compressed file"
    rm "$filename"
  else
    echo "failed to download $line"
  fi
done < "$SCRIPTPATH/$input"
#exit with a failure if we didn't compress anything
if [ -z "$filename" ]; then
  exit 1
fi

