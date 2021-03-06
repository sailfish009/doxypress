#
# Copyright (C) 2012-2017 Barbara Geller
# Copyright (C) 2012-2017 Ansel Sermersheim
# All rights reserved.    
#
# Copyright (c) 2015, Ivailo Monev, <xakepa10@gmail.com>
# Redistribution and use is allowed according to the terms of the BSD license.

# Usage: 
#   COPPERSPICE_RESOURCES(<userinterface.ui> [<resource.qrc>] ...)


macro(COPPERSPICE_RESOURCES RESOURCES)  
    foreach(resource ${RESOURCES} ${ARGN})
        get_filename_component(rscext ${resource} EXT)
        get_filename_component(rscname ${resource} NAME_WE)
      
        if(${rscext} STREQUAL ".ui")
            set(rscout ${CMAKE_CURRENT_BINARY_DIR}/ui_${rscname}.h)              
                       
#   (todo: enhance)
#   COMMAND CopperSpice::uic "${resource}" -o "${rscout}"

           add_custom_command(
                OUTPUT ${rscout}
                COMMAND "z:/cs5_x_lib/bin/uic" "${resource}" -o "${rscout}"
                MAIN_DEPENDENCY "${resource}"
           )

        elseif(${rscext} STREQUAL ".qrc")
            set(rscout ${CMAKE_CURRENT_BINARY_DIR}/qrc_${rscname}.cpp)
            add_custom_command(
                OUTPUT ${rscout}

#   (todo: enhance)
#   COMMAND CopperSpice::rcc "${resource}" -o "${rscout}")

                COMMAND "z:/cs5_x_lib/bin/rcc" "${resource}" -o "${rscout}" -name ${rscname}
                MAIN_DEPENDENCY "${resource}"
            )
            set_property(SOURCE ${resource} APPEND PROPERTY OBJECT_DEPENDS ${rscout})
        endif()
    endforeach()
endmacro()
