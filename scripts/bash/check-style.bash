#!/bin/bash

set -e

# editor-config checker
[ -z "$EC" ] && EC=ec

# shellscript checker
[ -z "$SHELLCHECK" ] && SHELLCHECK=shellcheck

# clang-format
[ -z "$CLANG_FORMAT" ] && CLANG_FORMAT=clang-format

# autopep8
[ -z "$AUTOPEP8" ] && AUTOPEP8=autopep8

fix="no"
opt="escp"

while [[ $# -gt 0 ]]
do
  case $1 in
  -f|--fix)
    fix="yes"
    shift
    ;;
  -c|--check)
    if [ $# -lt 2 ]; then
      echo "Expect spec check following -c/--check (choose from 'escp')"
      echo "  e  editorconfig check"
      echo "  s  shellscript check"
      echo "  c  clang-format check/format"
      echo "  p  python-pep8 check/format"
      exit 1
    fi
    shift
    opt="$1"
    if [[ "$opt" =~ [^escp] ]]; then
      echo "Unknown spec check $opt"
      exit 1
    fi
    shift
    ;;
  *)
    echo "Unknown option $1"
    exit 1
    ;;
  esac
done

source "${0%/*}"/bash/utils.sh

function check-ec() {
  green '[ editor-config checker ] begin'
  if ! $EC ; then
    fatal '[ editor-config checker ] failed'
    return 1
  else
    green '[ editor-config checker ] passed'
    return 0
  fi
}

function check-sh() {
  green '[ shellcheck ] begin'
  bash_files=$(grep --exclude-dir=.git -m 1 -rIzl '^#![[:blank:]]*/.*bin/.*bash')
  # shellcheck disable=2086
  if ! $SHELLCHECK -x $bash_files -e 1090,1091 ; then
    fatal '[ shellcheck ] failed'
    return 1
  else
    green '[ shellcheck ] passed'
    return 0
  fi
}

function check-cl() {
  green '[ clang-format ] begin'
  c_files=$(find -- * -regextype sed -regex '.*\.[ch]' -print)

  if [ -z "$c_files" ]; then
    green '[ clang-format ] no c language files found'
    return 0
  fi

  if [ "$fix" = "yes" ]; then
    # shellcheck disable=2086
    if ! $CLANG_FORMAT -i $c_files ; then
      warning '[ clang-format ] maybe version too low?'
    fi
    green '[ clang-format ] fix end'
    return 0
  else
    # shellcheck disable=2086
    cnt=$($CLANG_FORMAT $c_files --output-replacements-xml | grep -c "<replacement "; exit "${PIPESTATUS[0]}")
    if [ "${PIPESTATUS[0]}" -ne 0 ] && [ "$cnt" -eq 0 ]; then
      warning '[ clang-format ] maybe version too low?'
    fi
    if [ "$cnt" -eq 0 ]; then
      green '[ clang-format ] check passed'
      return 0
    else
      fatal "[ clang-format ] check failed"
      return 1
    fi
  fi
}

function check-py() {
  green '[ autopep8 ] begin'
  py_files=$(find -- * -name '*.py' -print)

  if [ -z "$py_files" ]; then
    green '[ autopep8 ] no python files found'
    return 0
  fi

  if [ "$fix" = "yes" ]; then
    # shellcheck disable=2086
    $AUTOPEP8 --max-line-length 100 -i -j 0 $py_files
    green '[ autopep8 ] fix end'
  else
    # shellcheck disable=2086
    if ! $AUTOPEP8 --max-line-length 100 --exit-code -d $py_files ; then
      fatal '[ autopep8 ] check failed'
      return 1
    else
      green '[ autopep8 ] check passed'
    fi
  fi
  return 0
}

r=0
if [[ "$opt" == *"p"* ]] && ! check-py; then
  r=2
fi
if [[ "$opt" == *"c"* ]] && ! check-cl; then
  r=3
fi
if [[ "$opt" == *"s"* ]] && ! check-sh; then
  r=4
fi
if [[ "$opt" == *"e"* ]] && ! check-ec; then
  r=5
fi
exit $r
