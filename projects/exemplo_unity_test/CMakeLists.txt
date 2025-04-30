cmake_minimum_required(VERSION 3.10)
project(exemplo_unity_test C)

# Defina o padrão de compilação
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g3")

# Inclua diretórios
include_directories(
    ${PROJECT_SOURCE_DIR}/lib/Unity/src
)

# Adicione os arquivos-fonte
set(SOURCES
    exemplo_unity_test.c
    lib/Unity/src/unity.c
)

# Defina o executável
add_executable(exemplo_unity_test ${SOURCES})

# Defina diretório de saída
set_target_properties(exemplo_unity_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/output
)
