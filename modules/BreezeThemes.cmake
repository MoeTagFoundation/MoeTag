# Setup Qt: this works with both Qt5 and Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(
  QT NAMES Qt6 Qt5
  COMPONENTS Core
  REQUIRED)
find_package(
  Qt${QT_VERSION_MAJOR}
  COMPONENTS ${AE_REQUIRED_QT_COMPONENTS}
  REQUIRED)
# -------------------

# Get Python to compile the stylesheets.
# Fetch the repository, configure, compile the stylesheets.
find_package(Python COMPONENTS Interpreter)

include(FetchContent)

set(FETCHCONTENT_QUIET
    OFF
    CACHE BOOL "Silence fetch content" FORCE)

FetchContent_Declare(
  breeze_stylesheets
  GIT_REPOSITORY https://github.com/Alexhuszagh/BreezeStyleSheets.git
  GIT_TAG origin/main
  GIT_PROGRESS ON
  USES_TERMINAL_DOWNLOAD TRUE)

FetchContent_GetProperties(breeze_stylesheets)
if(NOT breeze_stylesheets_POPULATED)
  FetchContent_Populate(breeze_stylesheets)

  add_library(breeze_themes STATIC "${breeze_stylesheets_SOURCE_DIR}/dist/qrc/breeze.qrc")

  add_custom_target(
    run_python_breeze ALL
    COMMAND ${Python_EXECUTABLE} configure.py --resource breeze.qrc
    WORKING_DIRECTORY ${breeze_stylesheets_SOURCE_DIR}
    BYPRODUCTS "${breeze_stylesheets_SOURCE_DIR}/dist/qrc/breeze.qrc"
    COMMENT "Generating themes")

  add_dependencies(breeze_themes run_python_breeze)
endif()
