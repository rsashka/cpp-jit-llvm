cmake_minimum_required(VERSION 3.15)


set(LLVM_VERSION "22")

# Установка компилятора Clang
set(CMAKE_C_COMPILER "clang-${LLVM_VERSION}")
set(CMAKE_CXX_COMPILER "clang++-${LLVM_VERSION}")

project(cpp-jit-llvm CXX)

# Установка стандарта C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Прямое указание путей LLVM и Clang
set(LLVM_DIR "/usr/lib/llvm-${LLVM_VERSION}")
set(LLVM_INCLUDE_DIR "${LLVM_DIR}/include")
set(LLVM_LIBRARY_DIR "${LLVM_DIR}/lib")

message(STATUS "Using LLVM in: ${LLVM_DIR}")

# Добавление путей заголовочных файлов LLVM и Clang
include_directories(${LLVM_INCLUDE_DIR})

# Определения для сборки
add_definitions(-DBUILD_DEBUG -DENABLE_LLVM_SHARED=1)

# Получение флагов компиляции LLVM через llvm-config
execute_process(
    COMMAND llvm-config-${LLVM_VERSION} --cxxflags
    OUTPUT_VARIABLE LLVM_CXX_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Дополнительные флаги компиляции
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -fcxx-exceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfloat-equal -Wundef -Wwrite-strings")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-declarations -Wno-trigraphs")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-source-encoding")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-variable")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=switch")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")

# Флаги отладки
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0 -g -gdwarf-4")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize-trap=undefined")

# Флаги релиза (если потребуется)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Получение библиотек LLVM через llvm-config
execute_process(
    COMMAND llvm-config-${LLVM_VERSION} --libs all
    OUTPUT_VARIABLE LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND llvm-config-${LLVM_VERSION} --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Добавление путей к библиотекам
link_directories(${LLVM_LIBRARY_DIR})

# Создание исполняемого файла
add_executable(cpp-jit-llvm main.cpp)

# Установка свойств линковки
set_target_properties(cpp-jit-llvm PROPERTIES
    LINK_FLAGS "-Wl,--export-dynamic -Wl,--exclude-libs,ALL -rdynamic"
)

# Линковка с библиотеками LLVM и Clang
target_link_libraries(cpp-jit-llvm
    ${LLVM_LIBS}
    ${LLVM_SYSTEM_LIBS}
    clang-cpp
    dl
    pthread
)

# Дополнительные пути поиска заголовочных файлов
target_include_directories(cpp-jit-llvm PRIVATE
    ${LLVM_INCLUDE_DIRS}
    /usr/local/include
    /usr/include/x86_64-linux-gnu/c++/10
)
