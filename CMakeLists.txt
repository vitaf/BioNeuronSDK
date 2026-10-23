# Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
# E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
# Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

cmake_minimum_required(VERSION 3.20)
project(BioNeuronSDK LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# ==================== КОНФИГУРАЦИЯ ====================

# Путь к BioNeuronCore (для include)
if(NOT DEFINED BIOCORE_ROOT)
    set(BIOCORE_ROOT "${CMAKE_SOURCE_DIR}/../BioNeuronCore" CACHE PATH "Путь к корню BioNeuronCore")
endif()

set(BIOCORE_INCLUDE_DIR "${BIOCORE_ROOT}/include" CACHE PATH "BioNeuronCore include")
set(BIOCORE_SDK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include" CACHE PATH "BioNeuronSDK include")

# ==================== INSTALL ====================

include(GNUInstallDirs)

install(FILES include/plugin-devkit.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/biocore-sdk)

install(FILES cmake/biocore-plugin.cmake
        DESTINATION ${CMAKE_INSTALL_DATADIR}/biocore-sdk/cmake)

install(FILES src/biocore-gen-dispatch.py
        DESTINATION ${CMAKE_INSTALL_DATADIR}/biocore-sdk
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)

install(DIRECTORY templates/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/biocore-sdk/templates)

install(DIRECTORY examples/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/biocore-sdk/examples)

# ==================== ПРИМЕРЫ (опционально) ====================

option(BUILD_EXAMPLES "Собирать примеры плагинов" ON)

if(BUILD_EXAMPLES)
    include(cmake/biocore-plugin.cmake)

    add_subdirectory(examples)
endif()

# ==================== ТЕСТЫ ДИСПЕТЧЕРА ====================

option(BUILD_DISPATCH_TEST "Тест генератора dispatch" OFF)

if(BUILD_DISPATCH_TEST)
    enable_testing()

    # Проверяем что Python доступен
    find_program(PYTHON3 python3)
    if(PYTHON3)
        # Генерация dispatch из встроенных плагинов BioNeuronCore
        add_test(
            NAME biocore_gen_dispatch_test
            COMMAND ${PYTHON3}
                    ${CMAKE_SOURCE_DIR}/src/biocore-gen-dispatch.py
                    ${BIOCORE_ROOT}/plugins/builtin
                    -o ${CMAKE_BINARY_DIR}/test_dispatch_generated.cpp
        )
    endif()
endif()

# ==================== ТЕСТЫ SDK ====================

option(BUILD_SDK_TESTS "Сборка и запуск тестов SDK" ON)

if(BUILD_SDK_TESTS)
    enable_testing()

    # C++ тесты plugin-devkit.h
    find_program(GPP g++)
    if(GPP)
        add_test(
            NAME test_plugin_devkit
            COMMAND bash -c "${GPP} -std=c++17 -O2 -Wall -I${CMAKE_SOURCE_DIR}/include -o ${CMAKE_BINARY_DIR}/test_plugin_devkit ${CMAKE_SOURCE_DIR}/tests/test_plugin_devkit.cpp && ${CMAKE_BINARY_DIR}/test_plugin_devkit"
        )
    endif()

    # C++ тесты example плагинов (dlopen/dlsym)
    if(GPP)
        add_test(
            NAME test_example_plugins
            COMMAND bash -c "${GPP} -std=c++17 -O2 -Wall -I${CMAKE_SOURCE_DIR}/include -I${BIOCORE_INCLUDE_DIR} -o ${CMAKE_BINARY_DIR}/test_example_plugins ${CMAKE_SOURCE_DIR}/tests/test_example_plugins.cpp -ldl && cd ${CMAKE_BINARY_DIR} && ${CMAKE_BINARY_DIR}/test_example_plugins"
        )
    endif()

    # C++ тесты biocore-plugin.cmake (сборка .so через CMake)
    if(GPP)
        add_test(
            NAME test_cmake_module
            COMMAND bash -c "${GPP} -std=c++17 -O2 -Wall -o ${CMAKE_BINARY_DIR}/test_cmake_module ${CMAKE_SOURCE_DIR}/tests/test_cmake_module.cpp -ldl && cd ${CMAKE_BINARY_DIR} && ${CMAKE_BINARY_DIR}/test_cmake_module"
        )
    endif()

    # Python тесты biocore-gen-dispatch.py
    find_program(PYTHON3 python3)
    if(PYTHON3)
        add_test(
            NAME test_dispatch_gen
            COMMAND ${PYTHON3} -m unittest discover -s ${CMAKE_SOURCE_DIR}/tests -p test_dispatch_gen.py -v
        )
    endif()

    # ── SDK Security: .desc JSON validation, path traversal ──────────
    add_test(
        NAME test_sdk_security
        COMMAND bash -c "${GPP} -std=c++17 -O2 -Wall -I${CMAKE_SOURCE_DIR}/include -o ${CMAKE_BINARY_DIR}/test_sdk_security ${CMAKE_SOURCE_DIR}/tests/test_sdk_security.cpp && ${CMAKE_BINARY_DIR}/test_sdk_security"
    )

    # ── SDK Integration: full plugin lifecycle ──────────────────────
    add_test(
        NAME test_sdk_integration
        COMMAND bash -c "${GPP} -std=c++17 -O2 -Wall -I${CMAKE_SOURCE_DIR}/include -I${BIOCORE_INCLUDE_DIR} -o ${CMAKE_BINARY_DIR}/test_sdk_integration ${CMAKE_SOURCE_DIR}/tests/test_sdk_integration.cpp -ldl && ${CMAKE_BINARY_DIR}/test_sdk_integration"
    )
endif()
