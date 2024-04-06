#!/bin/bash

tests=500

for i in $(eval echo "{1..$tests}")
do
  echo test : $i/$tests
  ctest -C Release
  # ctest -C Release -R CwaDataTestMpRingSocket
  # ctest -C Release -R "CwaDataTestMpRing.*"

  if [[ $? -ne 0 ]]; then
    break
  fi
done
