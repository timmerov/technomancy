#
# Copyright (C) 2012-2016 tim cotter. All rights reserved.
#

# search for "you need to"

# you need to set these:
set(THIS_PROJECT_NAME technomancy)
set(THIS_CMAKE_DIR "${CMAKE_SOURCE_DIR}/cmake")

# cmake boilerplate
include("${THIS_CMAKE_DIR}/scripts/preamble.txt")

# more cmake boilerplate
# unfortunately, we can't put this in preamble.
cmake_minimum_required(VERSION ${THIS_CMAKE_VERSION})

# add the targets
add_subdirectory(agm)
add_subdirectory(common)
add_subdirectory(helloworld)
add_subdirectory(mnist)
add_subdirectory(polar2cube)
add_subdirectory(spheregen)
add_subdirectory(stereogram)
add_subdirectory(world)
