# compares a and b and puts the result into the variable name referenced in result
function(max  a  b result)    
    if ( ${a} GREATER  ${b} )
        SET(${result} ${a} PARENT_SCOPE)
    else()
        SET(${result} ${b} PARENT_SCOPE)
    endif()
endfunction()

# formatted variable output
function(vprint varname)
    if(NOT DEFINED vprint_width)
        SET (vprint_width 30) # formatting width
    endif()
    
    SET(varcontent ${${varname}} )
    
    string(LENGTH "${varname}" vname_length )
    math(EXPR padding_length ${vprint_width}-${vname_length})
    max( ${padding_length}  0 padding_length)
    string(REPEAT " " ${padding_length} vname_padding)
    MESSAGE( "${varname}${vname_padding} = ${varcontent}")
endfunction()

macro(print_all_vars)
    message("=================================================================")
    message("Defined CMake Variables: ")
    message("=================================================================")
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        vprint(${_variableName})
    endforeach()
    message("=================================================================")
endmacro(print_all_vars)

