# /bin/sh
# 
# Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
#  
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as 
# published by the Free Software Foundation version 2.
# 
# This program is distributed "as is" WITHOUT ANY WARRANTY of any
# kind, whether express or implied; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
source "st_log.sh"

########### DEFINE CONSTANTS ############
KB=1024
MB=1048576
GB=$((1024*1024*1024))
DEBUGFS_LOCATION=/sys/kernel/debug/
PSID="$0$$"
START_TIME=`date "+%s"`

########### DEFINE PLATFORM DATA ############
# This is done by ltp-ddt's runltp script, but it is optionally done
# here for cases where we are running the scripts outside ltp-ddt
# If the PATH in the target filesystem doesn't have below path exported,
# please add those path in order to run standalone ltp scripts.
# export PATH="${PATH}:/opt/ltp/testcases/bin:/opt/ltp/testcases/bin/ddt"

resolve_platform_name() {
  case $1 in
    *) PLATFORM="$1" ;; 
  esac
  echo $PLATFORM
}

if [ "x$SOC" == "x" ]
then
   LTPPATH='/opt/ltp'
   export PATH="${PATH}:${LTPPATH}/testcases/bin"$( find ${LTPROOT}/testcases/bin/ddt -type d -exec printf ":"{} \; )
   plat=`uname -a | cut -d' ' -f 2`
  local i=0; local DRIVERS=""
  while read -r file
   do
    echo $file | grep -e "^#.*" > /dev/null
    if [ "$?" == "0" ]; then
      continue
    fi
    mkdir -p ${PLATFORMDIR}/${file}
    case $i in
       0) ARCH="$file"
         export ARCH ;;
       1) SOC="$file"
         export SOC ;;
       2) MACHINE="$file"
         export MACHINE ;;
       3) DRIVERS="$file" ;;
       *) DRIVERS="${DRIVERS},${file}" ;;
    esac
    i=`expr $i + 1`
  done < ${LTPPATH}/platforms/`resolve_platform_name $plat`
  export DRIVERS

fi

########### FUNCTIONS #####################
# Default value for inverted_return is "false" but can
# be overridden by individual scripts.
inverted_return="false"

do_cmd() {
  CMD=$*
  test_print_trc "Inside do_cmd:CMD=$CMD"
  eval $CMD
  RESULT=$?
    if [ "$inverted_return" == "false" ]
    then
      if [ $RESULT -ne 0 ]
      then
        test_print_err "$CMD failed. Return code is $RESULT"
        exit $RESULT
      fi
    else
        if [ $RESULT -eq 0 ]
        then
        test_print_err "$CMD failed. Return code is $RESULT"
        exit 1
        fi
    fi
}
#do_cmd "mount | grep mtdblock4 || echo notmounted"

# Check the given list of parameters and verify that they are set.
check_mandatory_inputs() {
    for x in $*
    do
        eval t="$"$x
        if [ "$t" == "" ]
        then
            test_print_trc "Mandatory input \"$x\" not specified"
            exit 1
        fi
    done
}

die() {
  test_print_err "FATAL: $*"
  exit 1
}



# Extract one return value from name=value list file
# First argument is the file with name=value list
# Second argument is the name of the value to return
get_return_value() {
  if [ $# -ne 2 ]; then
    die "Wrong number of arguments. \
                     Usage: get_return_value <file> <value name>"
  fi
  tmp_ifs=$IFS
        IFS=$'\n'
        file=$1
        name=$2
        for line in $(cat "$file") ; do
                TMPNAME=`echo $line | cut -d"=" -f1 | sed 's/^ *//g' | sed 's/ *$//g'`
                [ "$TMPNAME" == "$name" ] && TMPVAL=`echo $line | cut -d"=" -f2 | \
                sed 's/^ *//g' | sed 's/ *$//g'` && echo $TMPVAL && IFS=$tmp_ifs && exit 0
        done
        IFS=$tmp_ifs
        die "Value $name not found in $file"
}

# Get value for key in "key1=value2 key2=value2" space seperated pairs
# Input:
#   $1 key
#   $2 key-value pairs 
#   $3 key-value delimiter

get_value_for_key() {
  if [ $# -ne 3 ]; then
    die "Wrong number of arguments. \
    Usage: get_value_for_key <key> <key-value pairs> <key-value delimiter>"
  fi

  key=$1
  key_value_pairs=$2
  key_value_delimiter=$3

  for pair in $key_value_pairs; do
    k=`echo $pair |cut -d"=" -f1`
    v=`echo $pair |cut -d"=" -f2` # note: value could be empty
    if [ "$k" == "$key" ]
    then
      rtn="$v"
    break
  fi
  done
  echo "$rtn"
}

# Compare two files based on md5sum
# Input:
#   $1 file1
#   $2 file2
# Return:
#   true if equal; false otherwise
compare_md5sum()
{
  FILE1=$1
  FILE2=$2
  a=$(md5sum "$FILE1"|cut -d' ' -f1)
  if [ $? -ne 0 ]; then
    echo "error getting md5sum of $FILE1"
    exit 1
  fi
  echo "$1: $a"
  b=$(md5sum "$FILE2"|cut -d' ' -f1)
  if [ $? -ne 0 ]; then
    echo "error getting md5sum of $FILE2"
    exit 1
  fi
  echo "$2: $b"
  [ "$a" = "$b" ]
}

# report something with delta time
report()
{
  CUR_TIME=`date "+%s"`
  delta=`expr $CUR_TIME - $START_TIME`
  echo "$PSID:$START_TIME->$CUR_TIME($delta):$test_iteration: $*"
  sync
}

_random()
{
  if [ $1 -gt 32767 ]; then
    max_mult=`expr $1 / 32767`
    mult=`expr $RANDOM  % $max_mult`
  else
    mult=0;
  fi
  v=`echo "$RANDOM + ($RANDOM * $mult)" | bc `
  echo $v
}
# random
# $1 - max_value
random()
{
  v=`_random $1`
  #v=`dd if=/dev/urandom count=1 2> /dev/null | cksum | cut -c 0-10`
  v1=`expr $1 + 1`
  expr $v % $v1
}

# random not equal to 0
random_ne0()
{
  v=`_random $1`
  #v=`dd if=/dev/urandom count=1 2> /dev/null | cksum | cut -c 0-10`
  expr $v % $1 + 1
}

# check different kernel errors
check_kernel_errors()
{
  type=$1
  case $type in
    kmemleak)
      kmemleaks="/sys/kernel/debug/kmemleak"
      if [ ! -e ${kmemleaks} ]; then
        die "kmemleak sys entry doesn't exist; Please enable DEBUG_KMEMLEAK"
      fi

      # trigger memory scan
      do_cmd "echo scan > ${kmemleaks}"
      # give kernel some time to scan
      do_cmd sleep 30
      kmemleak_detail=`cat ${kmemleaks}`
      if [ -n "${kmemleak_detail}" ]; then
        die "There are memory leaks being detected. The details are displayed as below: ${kmemleak_detail}"
      else
        test_print_trc "No memory leaks being detected."
      fi 
       
    ;;
    spinlock)
      zcat /proc/config.gz | grep "DEBUG_SPINLOCK=y" || die "Please enable DEBUG_SPINLOCK"
      # Check dmesg to catch the error
      spinlock_errors="BUG: spinlock"
      dmesg |grep -i "${spinlock_errors}" && die "There is spinlock errors showing in dmesg" || test_print_trc "No spinlock related error found in dmesg"
    ;;
    *)
      die "check_kernel_errors: No logic for type $type yet."
    ;;
  esac
}