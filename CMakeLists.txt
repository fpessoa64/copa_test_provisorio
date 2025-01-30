cmake_minimum_required(VERSION 3.10)

###
# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Set the project name
project(HelloApp)

# Add the JSON for Modern C++ library
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.10.5
)
FetchContent_MakeAvailable(json)

# Adiciona a biblioteca pybind11
# FetchContent_Declare(
#     pybind11
#     GIT_REPOSITORY https://github.com/pybind/pybind11.git
#     GIT_TAG v2.11.1
# )
# FetchContent_MakeAvailable(pybind11)

# Encontra o Python
find_package(Python3 REQUIRED COMPONENTS Development)

# Add an executable
add_executable(hello_app main.cpp image.server.cpp manager.inspection.cpp inspection.cpp consolidate.results.cpp)

# Linka as bibliotecas ao executável
target_link_libraries(hello_app 
    PRIVATE 
    glog::glog 
    nlohmann_json::nlohmann_json 
    ${OpenCV_LIBS}
)

### add Google log library 
# Add the Google logging library (glog)
FetchContent_Declare(
    glog
    GIT_REPOSITORY https://github.com/google/glog.git
    GIT_TAG v0.5.0
)
FetchContent_MakeAvailable(glog)



#library opencv
find_package(OpenCV REQUIRED)
target_link_libraries(hello_app PRIVATE ${OpenCV_LIBS})


# Link the Google logging library to the executable
target_link_libraries(hello_app PRIVATE glog::glog)

# Link the JSON library to the executable
target_link_libraries(hello_app PRIVATE nlohmann_json::nlohmann_json)

target_link_libraries(hello_app PRIVATE Python3::Python)