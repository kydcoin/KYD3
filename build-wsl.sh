#!/bin/bash
#  __    __  __      __  _______
# /  |  /  |/  \    /  |/       \
# $$ | /$$/ $$  \  /$$/ $$$$$$$  |
# $$ |/$$/   $$  \/$$/  $$ |  $$ |
# $$  $$<     $$  $$/   $$ |  $$ | - KYD compile script(windows, linux) for wsl'
# $$$$$  \     $$$$/    $$ |  $$ | - AUTHOR - Bob Thomas
# $$ |$$  \     $$ |    $$ |__$$ |
# $$ | $$  |    $$ |    $$    $$/
# $$/   $$/     $$/     $$$$$$$/
#
# Simple script to automate some of the wallet and deamon compiling
# Made for windows subsystem for linux and able to compile for linux and windows

KYD_VERSION=$(git describe --abbrev=0 --tags)
UPDATE_COMPLETED=0

# Show intro message and ussage
function intro {
    echo -e "\e[1;32m"
    echo " __    __  __      __  _______  "
    echo "/  |  /  |/  \    /  |/       \ "
    echo '$$ | /$$/ $$  \  /$$/ $$$$$$$  |'
    echo '$$ |/$$/   $$  \/$$/  $$ |  $$ |'
    echo '$$  $$<     $$  $$/   $$ |  $$ | - KYD compile script(windows, linux) for wsl'
    echo '$$$$$  \     $$$$/    $$ |  $$ |' "- KYD VERSION - $KYD_VERSION"
    echo '$$ |$$  \     $$ |    $$ |__$$ |'
    echo '$$ | $$  |    $$ |    $$    $$/ '
    echo '$$/   $$/     $$/     $$$$$$$/'
    echo -e "\e[1;31m"
    echo "- Usages"
    echo -e "
    \e[1;33m- ./build-wsl.sh --all    \e[1;34m#builds windows32, windows64, linux-gnu\e[0m
    \e[1;33m- ./build-wsl.sh --win32  \e[1;34m#builds windows32\e[0m
    \e[1;33m- ./build-wsl.sh --win64  \e[1;34m#build windows64\e[0m
    \e[1;33m- ./build-wsl.sh --linux  \e[1;34m#build linux\e[0m
    \e[1;33m- ./build-wsl.sh --help   \e[1;34m#shows this message\e[0m"
}

# compile target uses the $1 param as target name
# Cleans previously installed target files
# Build dependencies or uses cached ones
# Compiles and installs the target in ~/kyd_releases
function build {
#    if [ "$UPDATE_COMPLETED" -eq "0" ]; then
#        install_core_deps
#    fi
    # Some hacky code to make a cute box message
    stringLength=$(expr ${#1} + 6)
    stars=`(printf '%*s' "$stringLength" | tr ' ' "*")`
    echo -e "\e[1;33m$stars\e[0m"
    echo -e "\e[1;33m*\e[0m$(printf '%*s' "$(expr 7)" | tr ' ' " ")\e[1;32mBUILDING$(printf '%*s' "$(expr $stringLength - 17)" | tr ' ' " ")\e[1;33m*\e[0m"
    echo -e "\e[1;33m*\e[0m$(printf '%*s' "$(expr $stringLength - 2)" | tr ' ' "-")\e[1;33m*\e[0m"
    echo -e "\e[1;33m*\e[0m  \e[1;32m$1  \e[1;33m*\e[0m"
    echo -e "\e[1;33m$stars\e[0m"
    # End box message

    make clean
    echo -e "\e[1;32m$1 MAKE FILES CLEANED\e[0m"
    echo -e "\e[1;32m$1 INSTALLING DEPENDENCIES\e[0m"
    cd depends
    make -j16  HOST="$1"
    echo -e "\e[1;32m$1 DEPENDENCIES READY\e[0m"

    echo -e "\e[1;32m$1 STARTING AUTOGEN\e[0m"
    cd ..
    ./autogen.sh
    CONFIG_SITE=$PWD/depends/"$1"/share/config.site CXXFLAGS=-fPIC -std=c++11 ./configure --prefix=/
    echo -e "\e[1;32m$1 AUTOGEN COMPLETED\e[0m"

    echo -e "\e[1;32m$1 STARTING KYD COMPILE\e[0m"
    make -j16
    echo -e "\e[1;32m$1 COMPILE COMPLETE AND INSTALLING TO\e[0m"
    echo -e "\e[1;31m\t~/kyd_releases/$KYD_VERSION/"$1"\e[0m"
    make install DESTDIR=~/kyd_releases/$KYD_VERSION/"$1"
    echo -e "\e[1;32m KYD-$1 INSTALLED AT ~/kyd_releases/$KYD_VERSION/$1"
}

# Install needed packages for the ubuntu instalation
function install_core_deps {
    echo -e "\e[1;32mINSTALLING AND UPDATING REQUIRED DEPENDENCIES\e[0m"
    sudo apt update
    sudo apt upgrade
    sudo apt install build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git
    sudo apt install g++-mingw-w64-x86-64
    sudo apt install g++-mingw-w64-i686 mingw-w64-i686-dev
    sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix # Set the default mingw32 g++ compiler option to posix.
    sudo update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++ # Set the default mingw32 g++ compiler option to posix.
    PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') # strip out problematic Windows %PATH% imported var
}

# Build all available targets
function build_all() {
    build "i686-w64-mingw32"    #32bit windows
    build "x86_64-w64-mingw32"  #64bit windows
    build "x86_64-pc-linux-gnu" #64bit linux gnu
}

# Parse commandline args
function parse_args
{
    # positional args
    args=()
    # named args
    while [ "$1" != "" ]; do
        case "$1" in
            --all )                  build_all;                     shift;;
            --win32 )                build "i686-w64-mingw32";      shift;;
            --win64 )                build "x86_64-w64-mingw32";    shift;;
            --linux )                build "x86_64-pc-linux-gnu";   shift;;
            --help )                 intro;                         shift;; # quit and show usage
            * )                      intro;                         exit;; # quit and show usage
        esac
    done

    # restore positional args
    set -- "${args[@]}"
}

function run {
    if  [ "$#" -lt 1 ]; then
        intro
        exit
    fi
    parse_args "$@"
}

run "$@"
