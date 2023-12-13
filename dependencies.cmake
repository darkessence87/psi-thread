
function (find_submodule name path)
    if (EXISTS ../../${name})
        set (${path} ../../${name} PARENT_SCOPE)
    elseif (EXISTS ${3rdPARTY_DIR}/${name})
        set (${path} ${3rdPARTY_DIR}/${name} PARENT_SCOPE)
    endif()
endfunction()

add_subdirectory(3rdparty/psi-comm)
add_subdirectory(3rdparty/psi-tools)