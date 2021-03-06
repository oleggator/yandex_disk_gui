cmake_minimum_required(VERSION 2.8.11)
project(yds)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/src/*.h)

find_package(Qt5Core CONFIG REQUIRED Qml Quick Gui Network Core Concurrent)
find_package(Qt5Quick)
find_package(Qt5Qml)
find_package(Qt5Gui)
find_package(Qt5Network)
find_package(Qt5Core)
find_package(Qt5NetworkAuth)
find_package(Qt5WebEngine)

qt5_add_resources(qml_QRC res/qml.qrc)

add_executable(${PROJECT_NAME}  ${qml_QRC} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} Qt5::Qml Qt5::Quick Qt5::Core Qt5::Network
                                      Qt5::NetworkAuth Qt5::WebEngine)
