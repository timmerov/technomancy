#
# Copyright (C) 2012-2017 tim cotter. All rights reserved.
#

# search for "you need to"

# you need to set these:
set(THIS_TARGET_NAME clunc)

# log it
message("-- Adding executable ${THIS_TARGET_NAME}...")

# gather the source files.
file(GLOB_RECURSE THIS_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)

# gather the header files.
file(GLOB_RECURSE THIS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

# gather the bison and flex files.
set(THIS_PARSERS
    src/clunc.y
    src/clunc.l
)

# gather the bison and flex files.
bison_target(PB2CC_PARSER
    ${CMAKE_CURRENT_SOURCE_DIR}/src/clunc.y
    ${CMAKE_CURRENT_BINARY_DIR}/clunc.tab.c)
flex_target(PB2CC_LEXER
    ${CMAKE_CURRENT_SOURCE_DIR}/src/clunc.l
    ${CMAKE_CURRENT_BINARY_DIR}/clunc.flex.c)
add_flex_bison_dependency(PB2CC_LEXER PB2CC_PARSER)

# add the executable with source and includes in separate groups
add_executable(${THIS_TARGET_NAME}
	${THIS_SOURCE}
	${THIS_HEADERS}
    ${BISON_PB2CC_PARSER_OUTPUTS}
    ${FLEX_PB2CC_LEXER_OUTPUTS}
    ${THIS_PARSERS}
)

# add _d to the debug target name
set_target_properties(${THIS_TARGET_NAME} PROPERTIES DEBUG_POSTFIX _d)

# define the target in the source
target_compile_definitions(${THIS_TARGET_NAME} PRIVATE AGM_TARGET=${THIS_TARGET_NAME})

# add the include directories
set(INCS
    ${CMAKE_SOURCE_DIR}/agm/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(${THIS_TARGET_NAME} PUBLIC ${INCS})

# add the libraries
set(LIBS
    agm
)
target_link_libraries(${THIS_TARGET_NAME} ${LIBS})
