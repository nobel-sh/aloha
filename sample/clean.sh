#!/bin/bash

echo "Clearing all object files..."
object_files=$(ls *.o 2>/dev/null)
if [ -z "$object_files" ]; then
  echo "No object files to delete."
else
  echo "Deleting the following object files:"
  echo "- ""$object_files"
  rm -f *.o
  echo "All object files deleted."
fi

echo ""
echo "Clearing all executable files..."
executable_files=$(ls *.out 2>/dev/null)
if [ -z "$executable_files" ]; then
  echo "No executable files to delete."
else
  echo "Deleting the following executable files:"
  echo "- ""$executable_files"
  rm -f *.out
  echo "All executables deleted."
fi
