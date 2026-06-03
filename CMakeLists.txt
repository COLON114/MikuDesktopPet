cmake_minimum_required(VERSION 3.16)

project(Miku VERSION 0.1 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets Core Multimedia MultimediaWidgets Network
)

qt_standard_project_setup()

# 开启 AUTOMOC（重要！）
set(CMAKE_AUTOMOC ON)

# 添加所有源文件和头文件
qt_add_executable(Miku
    main.cpp
)

target_link_libraries(Miku PRIVATE 
    Qt6::Widgets Qt6::Core Qt6::Multimedia 
    Qt6::MultimediaWidgets Qt6::Network
)

set_target_properties(Miku PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Miku
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)