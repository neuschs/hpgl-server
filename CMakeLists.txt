cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "C:/msys64/mingw32/bin/g++.exe")

project(HPGLServer
    LANGUAGES CXX
    )

find_package(Boost 1.80 REQUIRED COMPONENTS
    filesystem
    )

find_package(CLI11 REQUIRED)
find_package(spdlog REQUIRED)

add_executable(HPGLServer
    src/main.cpp
    lib/job.cpp
    )

include_directories(
    lib/
    ${CLI11_DIR}/include
)

target_compile_features(HPGLServer PRIVATE cxx_std_11)

# -Wl,-Bstatic,--whole-archive -lpthread -Wl,--no-whole-archive  -Wl,-Bstatic -lboost_system-mt )
target_link_options(HPGLServer PRIVATE  "-lws2_32"
                                        "-lwsock32"
                                        "-static-libstdc++" 
                                        "-static-libgcc"
)
#"-lboost_program_options-mt"
#"-lboost_filesystem-mt")
#target_compile_options(HPGLServer PRIVATE -static-libstdc++)
#target_compile_options(HPGLServer PRIVATE -lboost_program_options-mt)
#target_compile_options(HPGLServer PRIVATE -lboost_filesystem-mt)

target_link_libraries(HPGLServer Boost::filesystem)
target_link_libraries(HPGLServer ws2_32)
target_link_libraries(HPGLServer spdlog::spdlog)
