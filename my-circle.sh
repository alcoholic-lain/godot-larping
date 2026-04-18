#!/bin/sh
printf '\033c\033]0;%s\a' basic movements
base_path="$(dirname "$(realpath "$0")")"
"$base_path/my-circle.x86_64" "$@"
