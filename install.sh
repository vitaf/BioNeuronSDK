#!/usr/bin/env bash
# Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
# E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
# Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.
#
# install.sh — установка и сборка BioNeuronSDK
# Устанавливает Python (если нужно), проверяет BioNeuronCore, собирает примеры,
# проверяет генератор dispatch.

set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'
BOLD='\033[1m'; NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo -e "${CYAN}${BOLD}================================================${NC}"
echo -e "${CYAN}${BOLD}  Установка BioNeuronSDK${NC}"
echo -e "${CYAN}${BOLD}  Инструментарий разработчика плагинов${NC}"
echo -e "${CYAN}${BOLD}================================================${NC}"
echo ""

# ── Определение дистрибутива ────────────────────────────────────────────────

SUDO=""
if command -v sudo &>/dev/null; then
    SUDO="sudo"
fi

# ── Шаг 1: Python ────────────────────────────────────────────────────────────

echo -e "${BOLD}[1/5] Проверка Python 3...${NC}"

PYTHON=""
if command -v python3 &>/dev/null; then
    PYTHON="python3"
elif command -v python &>/dev/null; then
    PYTHON="python"
fi

if [ -n "${PYTHON}" ]; then
    PY_VER=$("${PYTHON}" --version 2>&1)
    echo "  ✓ ${PY_VER}"
else
    echo "  Python 3 не найден. Устанавливаю..."
    if [ -f /etc/debian_version ]; then
        ${SUDO} apt-get update -qq && ${SUDO} apt-get install -y -qq python3
    elif [ -f /etc/fedora-release ]; then
        ${SUDO} dnf install -y python3
    elif [ -f /etc/arch-release ]; then
        ${SUDO} pacman -S --needed --noconfirm python
    else
        echo -e "  ${RED}✗ Не удалось определить пакетный менеджер. Установите Python 3.6+ вручную.${NC}"
        exit 1
    fi
    PYTHON="python3"
    echo "  ✓ Python установлен."
fi

# ── Шаг 2: GCC ───────────────────────────────────────────────────────────────

echo ""
echo -e "${BOLD}[2/5] Проверка компилятора GCC...${NC}"

if command -v g++ &>/dev/null; then
    GCC_VER=$(g++ --version 2>&1 | head -1)
    echo "  ✓ ${GCC_VER}"
else
    echo "  GCC не найден. Устанавливаю..."
    if [ -f /etc/debian_version ]; then
        ${SUDO} apt-get update -qq && ${SUDO} apt-get install -y -qq g++
    elif [ -f /etc/fedora-release ]; then
        ${SUDO} dnf install -y gcc-c++
    elif [ -f /etc/arch-release ]; then
        ${SUDO} pacman -S --needed --noconfirm gcc
    else
        echo -e "  ${RED}✗ Установите GCC 9+ вручную.${NC}"
        exit 1
    fi
    echo "  ✓ GCC установлен."
fi

# ── Шаг 3: Проверка BioNeuronCore ────────────────────────────────────────────

echo ""
echo -e "${BOLD}[3/5] Проверка BioNeuronCore...${NC}"

CORE_DIR="${SCRIPT_DIR}/../BioNeuronCore"
CORE_INCLUDE="${CORE_DIR}/include"
CORE_OK=false

if [ -d "${CORE_INCLUDE}" ]; then
    echo "  ✓ Заголовки Core найдены: ${CORE_INCLUDE}"
    CORE_OK=true
else
    echo -e "  ${YELLOW}⚠  BioNeuronCore не найден.${NC}"
    echo "  SDK требует заголовки Core для сборки плагинов."
    echo "  Ожидаемое расположение: ${CORE_DIR}/"
    echo ""
    echo "  Вы можете указать путь через переменную окружения:"
    echo "    export BIOCORE_ROOT=/путь/к/BioNeuronCore"
fi

# ── Шаг 4: Проверка генератора ───────────────────────────────────────────────

echo ""
echo -e "${BOLD}[4/5] Проверка генератора dispatch...${NC}"

GEN_PY="${SCRIPT_DIR}/src/biocore-gen-dispatch.py"
if [ -f "${GEN_PY}" ]; then
    echo "  ✓ Генератор найден: src/biocore-gen-dispatch.py"
    echo "  Проверяю работу..."
    if "${PYTHON}" "${GEN_PY}" --stub > /dev/null 2>&1; then
        echo "  ✓ Генератор работает корректно."
    else
        echo -e "  ${YELLOW}⚠  Генератор запустился с ошибкой. Проверьте Python-зависимости.${NC}"
    fi
else
    echo -e "  ${YELLOW}⚠  Генератор не найден: ${GEN_PY}${NC}"
fi

# ── Шаг 5: Сборка примеров ───────────────────────────────────────────────────

echo ""
echo -e "${BOLD}[5/5] Сборка примеров...${NC}"

EXAMPLES_DIR="${SCRIPT_DIR}/examples"
BUILT_COUNT=0

if [ -d "${EXAMPLES_DIR}" ] && [ "${CORE_OK}" = true ]; then
    for src in "${EXAMPLES_DIR}"/*.cpp; do
        [ -f "${src}" ] || continue
        name=$(basename "${src}" .cpp)
        out="/tmp/sdk_example_${name}.so"
        echo "  Сборка ${name}..."
        if g++ -std=c++17 -fPIC -shared -O2 -Wall \
               -I "${SCRIPT_DIR}/include" \
               -I "${CORE_INCLUDE}" \
               -o "${out}" "${src}" 2>&1; then
            echo "    ${GREEN}✓ ${out}${NC}"
            BUILT_COUNT=$((BUILT_COUNT + 1))
            rm -f "${out}"
        else
            echo -e "    ${RED}✗ Ошибка сборки${NC}"
        fi
    done

    if [ ${BUILT_COUNT} -gt 0 ]; then
        echo "  ${GREEN}✓ Собрано примеров: ${BUILT_COUNT}${NC}"
    fi
elif [ "${CORE_OK}" != true ]; then
    echo -e "  ${YELLOW}⚠  Пропущено: BioNeuronCore не найден.${NC}"
else
    echo "  Примеры не найдены в ${EXAMPLES_DIR}/"
fi

# ── Итог ─────────────────────────────────────────────────────────────────────

echo ""
echo -e "${GREEN}${BOLD}================================================${NC}"
echo -e "${GREEN}${BOLD}  Установка BioNeuronSDK завершена!${NC}"
echo -e "${GREEN}${BOLD}================================================${NC}"
echo ""
echo "  Собрано примеров плагинов: ${BUILT_COUNT}"
echo ""
echo "  Для создания своего плагина:"
echo "    - Используйте шаблон: ${SCRIPT_DIR}/templates/plugin_template.cpp"
echo "    - Сборка: g++ -std=c++17 -fPIC -shared -O2 -Wall \\"
echo "              -I ${SCRIPT_DIR}/include \\"
echo "              -I ${CORE_INCLUDE} \\"
echo "              -o my_plugin.so my_plugin.cpp"
echo "    - Генерация dispatch:"
echo "      ${PYTHON} ${GEN_PY} <plugins_dir>/ -o dispatch_generated.cpp"
echo ""
