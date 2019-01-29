#!/bin/bash

if [ $# == 2]; then
  file_name=$1
  line_num=$2
  codeclap linux-hh $file_name:$line_num
else
  codeclap linux-hh main 
fi
