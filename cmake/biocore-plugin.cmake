# Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
# E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
# Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

# biocore-plugin.cmake — BioNeuronSDK CMake module.
# Подключается через include() в CMakeLists.txt проекта плагина.
#
# Использование:
#   find_package(biocore-sdk REQUIRED)  # или вручную:
#   include(path/to/biocore-plugin.cmake)
#
#   biocore_add_plugin(my_plugin my_plugin.cpp DESC my_plugin.desc INFO my_plugin.info)
#   biocore_gen_dispatch(PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins
#                        OUTPUT ${CMAKE_SOURCE_DIR}/src/dispatch_generated.cpp)

# ==================== ПРОВЕРКА ПЕРЕМЕННЫХ ====================

if(NOT DEFINED BIOCORE_ROOT)
    if(DEFINED ENV{BIOCORE_ROOT})
        set(BIOCORE_ROOT "$ENV{BIOCORE_ROOT}")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/../BioNeuronCore")
        set(BIOCORE_ROOT "${CMAKE_SOURCE_DIR}/../BioNeuronCore")
    else()
        message(WARNING "BIOCORE_ROOT не задан. Укажите путь к BioNeuronCore через -DBIOCORE_ROOT=...")
    endif()
endif()

if(NOT DEFINED BIOCORE_SDK_DIR)
    if(DEFINED ENV{BIOCORE_SDK_DIR})
        set(BIOCORE_SDK_DIR "$ENV{BIOCORE_SDK_DIR}")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/../BioNeuronSDK")
        set(BIOCORE_SDK_DIR "${CMAKE_SOURCE_DIR}/../BioNeuronSDK")
    else()
        set(BIOCORE_SDK_DIR "${CMAKE_SOURCE_DIR}")
    endif()
endif()

set(BIOCORE_INCLUDE_DIR "${BIOCORE_ROOT}/include")
set(BIOCORE_SDK_INCLUDE_DIR "${BIOCORE_SDK_DIR}/include")
set(BIOCORE_SDK_SRC_DIR "${BIOCORE_SDK_DIR}/src")

# ==================== ФУНКЦИИ ====================

# biocore_add_plugin(имя_плагина source.cpp [DESC source.desc] [INFO source.info] [OUTPUT_DIR dir])
#
# Собирает .so плагин через GCC (не Intel dpcpp) — плагины загружаются
# через dlopen из Studio (GCC-сборка), поэтому Intel-зависимости недопустимы.
#
# Аргументы:
#   имя_плагина  — имя цели и выходного .so (без префикса lib и .so)
#   source.cpp   — исходный файл плагина
#   DESC desc    — (опционально) .desc файл для копирования в выходную директорию
#   INFO info    — (опционально) .info справочный файл для копирования в выходную директорию
#   OUTPUT_DIR d — (опционально) директория для .so, .desc и .info (по умолчанию: CMAKE_BINARY_DIR/plugins)
#
# Результат:
#   ${CMAKE_BINARY_DIR}/plugins/имя_плагина.so
#   ${CMAKE_BINARY_DIR}/plugins/имя_плагина.desc  (если задан DESC)
#   ${CMAKE_BINARY_DIR}/plugins/имя_плагина.info  (если задан INFO)
#
# Плагин добавляется в aggregate target "builtin_plugins" (или "user_plugins").
#
function(biocore_add_plugin PLUGIN_NAME PLUGIN_SOURCE)
    set(options "")
    set(oneValueArgs DESC INFO OUTPUT_DIR)
    set(multiValueArgs "")
    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Приводим относительные пути к абсолютным (от CMAKE_CURRENT_SOURCE_DIR)
    if(NOT IS_ABSOLUTE "${PLUGIN_SOURCE}")
        set(PLUGIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN_SOURCE}")
    endif()
    if(PLUGIN_DESC AND NOT IS_ABSOLUTE "${PLUGIN_DESC}")
        set(PLUGIN_DESC "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN_DESC}")
    endif()
    if(PLUGIN_INFO AND NOT IS_ABSOLUTE "${PLUGIN_INFO}")
        set(PLUGIN_INFO "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN_INFO}")
    endif()

    if(PLUGIN_OUTPUT_DIR)
        set(OUT_DIR "${PLUGIN_OUTPUT_DIR}")
    else()
        set(OUT_DIR "${CMAKE_BINARY_DIR}/plugins")
    endif()

    file(MAKE_DIRECTORY ${OUT_DIR})

    # Компиляция .so через GCC (обязательно: не Intel dpcpp)
    set(SO_FILE "${OUT_DIR}/${PLUGIN_NAME}.so")
    add_custom_command(
        OUTPUT ${SO_FILE}
        COMMAND g++ -std=c++17 -fPIC -shared -O2 -Wall
                -I${BIOCORE_INCLUDE_DIR}
                -I${BIOCORE_SDK_INCLUDE_DIR}
                -o ${SO_FILE}
                ${PLUGIN_SOURCE}
        DEPENDS ${PLUGIN_SOURCE}
                ${BIOCORE_INCLUDE_DIR}/types.h
                ${BIOCORE_INCLUDE_DIR}/modifier_interface.h
        COMMENT "Building plugin: ${PLUGIN_NAME}.so"
    )
    list(APPEND PLUGIN_OUTPUTS ${SO_FILE})
    set(PLUGIN_OUTPUTS ${PLUGIN_OUTPUTS} PARENT_SCOPE)

    # Копирование .desc
    if(PLUGIN_DESC)
        set(DESC_FILE "${OUT_DIR}/${PLUGIN_NAME}.desc")
        add_custom_command(
            OUTPUT ${DESC_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${PLUGIN_DESC} ${DESC_FILE}
            DEPENDS ${PLUGIN_DESC}
            COMMENT "Copying ${PLUGIN_NAME}.desc"
        )
        list(APPEND PLUGIN_OUTPUTS ${DESC_FILE})
        set(PLUGIN_OUTPUTS ${PLUGIN_OUTPUTS} PARENT_SCOPE)
    endif()

    # Копирование .info
    if(PLUGIN_INFO)
        set(INFO_FILE "${OUT_DIR}/${PLUGIN_NAME}.info")
        add_custom_command(
            OUTPUT ${INFO_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${PLUGIN_INFO} ${INFO_FILE}
            DEPENDS ${PLUGIN_INFO}
            COMMENT "Copying ${PLUGIN_NAME}.info"
        )
        list(APPEND PLUGIN_OUTPUTS ${INFO_FILE})
        set(PLUGIN_OUTPUTS ${PLUGIN_OUTPUTS} PARENT_SCOPE)
    endif()

    # Промежуточная цель для этого плагина
    set(_plugin_target "${PLUGIN_NAME}_outputs")
    add_custom_target(${_plugin_target} DEPENDS ${PLUGIN_OUTPUTS})

    # Агрегатная цель: зависит от всех промежуточных целей
    if(NOT TARGET user_plugins)
        add_custom_target(user_plugins ALL)
    endif()
    add_dependencies(user_plugins ${_plugin_target})
endfunction()

# biocore_gen_dispatch(PLUGINS_DIR dir OUTPUT file [HEADER])
#
# Запускает biocore-gen-dispatch.py для генерации dispatch_generated.cpp/.h
# из .desc файлов в указанной директории.
#
# Аргументы:
#   PLUGINS_DIR dir — директория с .desc файлами
#   OUTPUT file     — выходной файл
#   HEADER          — генерировать .h с static inline (для GPU/SYCL)
#
# Требуется Python 3.
#
function(biocore_gen_dispatch)
    set(options HEADER)
    set(oneValueArgs PLUGINS_DIR OUTPUT)
    set(multiValueArgs "")
    cmake_parse_arguments(GEN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT GEN_PLUGINS_DIR)
        message(FATAL_ERROR "biocore_gen_dispatch: PLUGINS_DIR is required")
    endif()

    if(NOT GEN_OUTPUT)
        message(FATAL_ERROR "biocore_gen_dispatch: OUTPUT is required")
    endif()

    find_program(PYTHON3 python3 REQUIRED)

    set(GEN_SCRIPT "${BIOCORE_SDK_SRC_DIR}/biocore-gen-dispatch.py")

    if(GEN_HEADER)
        set(HEADER_ARG --header)
    else()
        set(HEADER_ARG "")
    endif()

    add_custom_command(
        OUTPUT ${GEN_OUTPUT}
        COMMAND ${PYTHON3} ${GEN_SCRIPT} ${GEN_PLUGINS_DIR}
                -o ${GEN_OUTPUT} ${HEADER_ARG}
        DEPENDS ${GEN_SCRIPT}
                ${GEN_PLUGINS_DIR}
        COMMENT "Generating dispatch_generated from plugins in ${GEN_PLUGINS_DIR}"
    )

    if(NOT TARGET gen_dispatch)
        add_custom_target(gen_dispatch ALL DEPENDS ${GEN_OUTPUT})
    endif()
    # gen_dispatch вызывается один раз; если нужно несколько — используйте отдельные цели
endfunction()

# biocore_find_sdk()
#
# Проверяет наличие SDK и устанавливает переменные:
#   BIOCORE_SDK_FOUND       — TRUE/FALSE
#   BIOCORE_SDK_INCLUDE_DIR — путь к plugin-devkit.h
#   BIOCORE_SDK_SRC_DIR     — путь к biocore-gen-dispatch.py
#
macro(biocore_find_sdk)
    if(EXISTS "${BIOCORE_SDK_INCLUDE_DIR}/plugin-devkit.h")
        set(BIOCORE_SDK_FOUND TRUE)
        message(STATUS "BioNeuronSDK found: ${BIOCORE_SDK_DIR}")
    else()
        set(BIOCORE_SDK_FOUND FALSE)
        message(WARNING "BioNeuronSDK not found at ${BIOCORE_SDK_DIR}")
        message(WARNING "Set BIOCORE_SDK_DIR or install BioNeuronSDK")
    endif()
endmacro()

# Сохраняем список результатов в родительском scope
set(PLUGIN_OUTPUTS "" CACHE INTERNAL "Plugin build outputs")
