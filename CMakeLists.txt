cmake_minimum_required(VERSION 3.16)
project(FelsicNotes VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineCore LinguistTools)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Source files will be added here
set(SOURCES
    src/main.cpp
)

# Qt Resources
set(RESOURCES
    # resources/resources.qrc
)

# Translations
set(TS_FILES
    # translations/en_US.ts
    # translations/pt_BR.ts
)
# qt6_add_translation(QM_FILES ${TS_FILES})

add_executable(FelsicNotes
    ${SOURCES}
    # ${RESOURCES}
    # ${QM_FILES}
)

target_link_libraries(FelsicNotes PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineCore
)

# CPack setup for future installers
set(CPACK_PACKAGE_NAME "FelsicNotes")
set(CPACK_PACKAGE_VENDOR "Deomkds")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
include(CPack)
