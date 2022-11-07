#/usr/bin/env bash

function usage()
{
  echo "Usage:"
  echo "  export.sh -d <dependency version> [-i] <version>"
  echo "    deps version:"
  echo "      - old: the lowest versions of dependencies"
  echo "      - new: the current versions of dependencies"
  echo "    -i          : export darwin-toolchain/1.0.8 if specified"
  echo "    version     : pichi version"
}

function cleanup()
{
  if [ -n "${show_help}" ]; then
    usage
    exit 1
  fi
}

function patch_recipe()
{
  local recipe="${recipes}/$(echo $1 | awk -F'/' '{print $1}')"
  for file in "$(ls ${recipe})"; do
    patch -td "${HOME}/.conan/data/$1/_/_/export" < "${recipe}/${file}"
  done
}

function do_export_old_deps()
{
  conan inspect boost/1.77.0@
  conan inspect mbedtls/2.25.0@
  conan inspect libsodium/1.0.18@
  conan inspect rapidjson/1.1.0@
  conan export "${recipes}/libmaxminddb" libmaxminddb/1.5.0@
  conan export "${recipes}/boringssl" boringssl/17@
}

function do_export_new_deps()
{
  conan inspect boost/1.79.0@
  conan inspect mbedtls/3.2.1@
  conan inspect libsodium/1.0.18@
  conan inspect rapidjson/1.1.0@
  conan export "${recipes}/libmaxminddb" libmaxminddb/1.6.0@
  conan export "${recipes}/boringssl" boringssl/18@
}

set -o errexit
trap cleanup EXIT
show_help="true"

recipes="$(dirname $0)/../recipes"
args=`getopt d: $*`
set -- $args
unset show_help
for i; do
  case "$i" in
    -d)
      shift
      case "$1" in
        old) do_export_old_deps;;
        new) do_export_new_deps;;
        *) usage; exit 1;;
      esac
      deps_exported="true"
      shift
      ;;
    --)
      show_help="true"
      [ -n "${deps_exported}" ]
      shift
      [ -n "$1" ]
      unset show_help
      conan export "${recipes}/pichi" "pichi/${1}@";
      shift
      ;;
  esac
done
