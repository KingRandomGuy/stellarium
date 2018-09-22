set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(TelescopeControl_AltAzGoTo
    AltAzGoToCommand.hpp
    AltAzGoToCommand.cpp
    AltAzGoToConnection.hpp
    AltAzGoToConnection.cpp
    TelescopeClientDirectAltAzGoTo.hpp
    TelescopeClientDirectAltAzGoTo.cpp
    )

target_link_libraries(TelescopeControl_AltAzGoTo
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    TelescopeControl_common
    )

SET_TARGET_PROPERTIES(TelescopeControl_AltAzGoTo PROPERTIES FOLDER "plugins/TelescopeControl")
