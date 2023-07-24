#
# Copyright (C) 2012-2020 tim cotter. All rights reserved.
#

# search for "you need to"

# you need to set these:
set(THIS_PROJECT_NAME technomancy)
set(THIS_CMAKE_DIR "${CMAKE_SOURCE_DIR}/cmake")

# cmake boilerplate
# unfortunately, we can't put this in preamble.
# see preamble.txt for explanations.
set(THIS_CMAKE_VERSION 2.8.11)
cmake_minimum_required(VERSION ${THIS_CMAKE_VERSION})
message("Creating project ${THIS_PROJECT_NAME}...")
project(${THIS_PROJECT_NAME})

# more cmake boilerplate
include("${THIS_CMAKE_DIR}/scripts/preamble.txt")

# add the targets
add_subdirectory(agm)
add_subdirectory(clunc)
add_subdirectory(common)
add_subdirectory(helloworld)
add_subdirectory(jupiter)
#add_subdirectory(moon)
add_subdirectory(peg)
add_subdirectory(pickleball)
add_subdirectory(pluto)
add_subdirectory(polar2cube)
add_subdirectory(prisoner)
add_subdirectory(rubiks)
add_subdirectory(spheregen)
add_subdirectory(stereogram)
add_subdirectory(sudoku)
add_subdirectory(voting)
add_subdirectory(world)
