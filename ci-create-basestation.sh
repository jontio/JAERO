#!/bin/bash

#warning will delete all *.sqb files in output folder

#input file of links and output folder where the downloading and uncompressing happens
#relitive links are wrt to where this script is
input="basestationlinks"
outputdir="../basestation"
save_sqb_file_as="basestation.sqb"

echo "ci-create-basestation"

#get script path
SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname "$SCRIPT")
cd "$SCRIPTPATH"

#create output path
mkdir -p "$outputdir"

#change to absolute paths
outputdir=$(realpath "$outputdir")
save_sqb_file_as=$(realpath "$outputdir/$save_sqb_file_as")

#goto output path
cd "$outputdir"

#remove any sqb files in the output folder
rm -f "$outputdir"/*.[sS][qQ][bB]

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
      echo "saved uncompressed files to $outputdir"
      find "$outputdir"/*.[sS][qQ][bQ] -maxdepth 1
      if [ $? -ne 0 ]; then
        echo "error: sqb not one of in uncompressed files."
        continue
      fi
      break
    fi
    if  echo "$MIMETYPE" | grep -q 'application/gzip' ; then
      echo "file is a gzip file"
      tar -zxf "$filename" -C "$outputdir"
      rm "$filename"
      echo "saved uncompressed files to $outputdir"
      find "$outputdir"/*.[sS][qQ][bQ] -maxdepth 1
      if [ $? -ne 0 ]; then
        echo "error: sqb not one of in uncompressed files."
        continue
      fi
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
#rename sqb file that should be in the download if not then fail
outputfiles=$(find "$outputdir" -iname "*.sqb")
if [ $? -ne 0 ]; then
  echo "error: sqb not one of in uncompressed files."
  exit 1
else
  filename=$(echo "$outputfiles" |head -1)
  if [ "$filename" != "$save_sqb_file_as" ]; then
    echo "renaming $filename to $save_sqb_file_as"
    mv "$filename" "$save_sqb_file_as"
  fi
fi

