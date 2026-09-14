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
    src/mainwindow.cpp
    src/mainwindow.h
    src/workspace_indexer.cpp
    src/workspace_indexer.h
    src/file_filter_model.cpp
    src/file_filter_model.h
    src/pdf_generator.cpp
    src/pdf_generator.h
    src/customize_toolbar_dialog.cpp
)

# Qt Resources
set(RESOURCES
    resources/resources.qrc
)

# Translations
set(TS_FILES
    # translations/en_US.ts
    # translations/pt_BR.ts
)
# qt6_add_translation(QM_FILES ${TS_FILES})

add_executable(FelsicNotes
    ${SOURCES}
    ${RESOURCES}
    # ${QM_FILES}
)

target_link_libraries(FelsicNotes PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineCore
)

# Installation
install(TARGETS FelsicNotes
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

# Use qt_generate_deploy_app_script to bundle DLLs on Windows automatically when installing/packaging
if(WIN32)
    qt_generate_deploy_app_script(
        TARGET FelsicNotes
        FILENAME_VARIABLE deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

# CPack setup for installers/packages
set(CPACK_PACKAGE_NAME "FelsicNotes")
set(CPACK_PACKAGE_VENDOR "Deomkds")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple markdown editor")

# Windows-specific CPack MSI (WIX) settings
if(WIN32)
    set(CPACK_GENERATOR "WIX")
    set(CPACK_WIX_INSTALL_SCOPE "perUser") # Installs to AppData without UAC/Admin
    set(CPACK_WIX_UPGRADE_GUID "0d8dbf43-128a-4db3-96b4-245c55ec3f63") # Needed for WIX upgrades
    set(CPACK_PACKAGE_EXECUTABLES "FelsicNotes" "Felsic Notes") # Creates a Start Menu shortcut
endif()

include(CPack)
