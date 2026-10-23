#!/usr/bin/env python3
# Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
# E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
# Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

# biocore-gen-dispatch — генератор dispatch_generated.cpp для BioNeuronCore.
#
# Сканирует .desc файлы в директории плагинов и генерирует
# dispatch_generated.cpp с switch-диспетчером для GPU/SYCL-пути.
#
# Использование:
#   python3 biocore-gen-dispatch.py plugins_dir/ -o dispatch_generated.cpp
#   python3 biocore-gen-dispatch.py plugins_dir/ --core-path ../BioNeuronCore/
#
# Без аргументов: генерирует заглушку (все user-типы возвращают 0.0f).

import os
import sys
import json
import argparse
from pathlib import Path


def parse_desc_file(path):
    """Парсит .desc JSON-файл. Возвращает dict с ключами: id, name, category, release."""
    try:
        with open(path, 'r') as f:
            data = json.load(f)
        modifiers = data.get('modifiers', [data])  # поддержка обоих форматов
        if not isinstance(modifiers, list):
            modifiers = [modifiers]
        return modifiers
    except (json.JSONDecodeError, KeyError) as e:
        print(f"  WARNING: failed to parse {path}: {e}", file=sys.stderr)
        return []


def scan_plugins_dir(plugins_dir):
    """Сканирует директорию плагинов, возвращает список модификаторов."""
    plugins_dir = Path(plugins_dir)
    all_modifiers = []

    if not plugins_dir.exists():
        print(f"WARNING: directory not found: {plugins_dir}", file=sys.stderr)
        return all_modifiers

    for desc_file in sorted(plugins_dir.glob("*.desc")):
        modifiers = parse_desc_file(desc_file)
        for mod in modifiers:
            mod_id = mod.get('id', -1)
            # Только пользовательские типы (121-127) идут в dispatch_generated
            # Встроенные типы (0-120) уже в kernel_dispatch.h
            if 121 <= mod_id <= 127:
                mod['_source'] = desc_file.name
                all_modifiers.append(mod)

    return all_modifiers


def make_func_name(mod_id, name, is_release):
    """Генерирует C-имя функции из ID и имени."""
    # Создаём безопасное C-имя: plugin_mod_{id}_{current|release}
    suffix = "release" if is_release else "current"
    return f"plugin_mod_{mod_id}_{suffix}"


def generate_dispatch(modifiers, header_mode=False):
    """Генерирует содержимое dispatch_generated.cpp или .h."""
    lines = []
    ext = "h" if header_mode else "cpp"
    lines.append(f"// dispatch_generated.{ext} — АВТОГЕНЕРАЦИЯ BioNeuronSDK.")
    lines.append(f"// Сгенерировано biocore-gen-dispatch. НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ.")
    lines.append(f"// Пользовательских типов: {len(modifiers)}")
    if header_mode:
        lines.append("// Режим: inline header (для GPU/SYCL device-кода)")
    lines.append("")
    lines.append('#include "types.h"')
    lines.append('#include "kernel_dispatch.h"')
    lines.append("")

    # Группируем: текущие и пресинаптические
    current_mods = [m for m in modifiers if not m.get('release', False)]
    release_mods = [m for m in modifiers if m.get('release', False)]

    # Собираем уникальные ID для current
    current_ids = set()
    for m in current_mods:
        current_ids.add(m['id'])

    # Собираем уникальные ID для release
    release_ids = set()
    for m in release_mods:
        release_ids.add(m['id'])

    func_prefix = "static inline " if header_mode else "BIOCORE_DEVICE_CALLABLE "

    # Weak-определения для текущих функций (только в .cpp режиме).
    # Это fallback-заглушки: если .so плагин НЕ загружен, modifier_current_table[]
    # содержит NULL, и dispatch_user_modifier_current() вызывает weak-заглушку
    # (возвращает 0 — нет тока, биологически безопасно).
    # Если .so плагин ЗАГРУЖЕН, register_modifier_current() устанавливает
    # указатель таблицы на реальную функцию из .so → заглушка НЕ вызывается.
    # weak-атрибут позволяет .so (с RTLD_GLOBAL) переопределить символ при необходимости.
    if not header_mode and current_ids:
        lines.append("// WEAK-заглушки: текущие функции пользовательских плагинов")
        lines.append("// Вызываются ТОЛЬКО если .so плагин не загружен (modifier_current_table[id] == NULL).")
        lines.append("// Возвращают 0.0f — нет тока, нейрон в покое (биологически безопасно).")
        for m in current_mods:
            fn = make_func_name(m['id'], m.get('name', ''), False)
            lines.append(f"__attribute__((weak))")
            lines.append(f"float {fn}(Modifier* m, Segment* s, float dt) {{")
            lines.append(f"    (void)m; (void)s; (void)dt;")
            lines.append(f"    return 0.0f;")
            lines.append(f"}}")
        lines.append("")

    # Weak-определения для пресинаптических модуляторов
    if not header_mode and release_ids:
        lines.append("// WEAK-заглушки: пресинаптические модуляторы пользовательских плагинов")
        lines.append("// Вызываются ТОЛЬКО если .so плагин не загружен (modulator_release_table[id] == NULL).")
        lines.append("// Возвращают 1.0f — нет модуляции выброса (биологически безопасно).")
        for m in release_mods:
            fn = make_func_name(m['id'], m.get('name', ''), True)
            lines.append(f"__attribute__((weak))")
            lines.append(f"float {fn}(Modifier* m, float* cleft_nt, float dt) {{")
            lines.append(f"    (void)m; (void)cleft_nt; (void)dt;")
            lines.append(f"    return 1.0f;")
            lines.append(f"}}")
        lines.append("")

    # В header-режиме: ожидаем что пользовательские функции определены
    # в подключаемых пользователем заголовках (включаются перед dispatch_generated.h)
    if header_mode:
        lines.append("// ВНИМАНИЕ: для GPU пользовательские функции должны быть определены")
        lines.append("// как static inline в подключаемых заголовках ДО этого файла.")
        lines.append("// См. документацию BioNeuronSDK: GPU-плагины.")
        lines.append("")

    # Диспетчер текущих функций
    func_kw = "static inline " if header_mode else ""
    lines.append(f"{func_prefix}float dispatch_user_modifier_current(uint8_t type, Modifier* m, Segment* s, float dt) {{")
    if current_ids:
        lines.append("    switch (type) {")
        for type_id in sorted(current_ids):
            mod = next(m for m in current_mods if m['id'] == type_id)
            fn = make_func_name(type_id, mod.get('name', ''), False)
            lines.append(f"        case {type_id}: return {fn}(m, s, dt);")
        lines.append("        default: return 0.0f;")
        lines.append("    }")
    else:
        lines.append("    (void)type; (void)m; (void)s; (void)dt;")
        lines.append("    return 0.0f;")
    lines.append("}")
    lines.append("")

    # Диспетчер пресинаптических модуляторов
    lines.append(f"{func_prefix}float dispatch_user_release_modulator(uint8_t type, Modifier* m, float* cleft_nt, float dt) {{")
    if release_ids:
        lines.append("    switch (type) {")
        for type_id in sorted(release_ids):
            mod = next(m for m in release_mods if m['id'] == type_id)
            fn = make_func_name(type_id, mod.get('name', ''), True)
            lines.append(f"        case {type_id}: return {fn}(m, cleft_nt, dt);")
        lines.append("        default: return 1.0f;")
        lines.append("    }")
    else:
        lines.append("    (void)type; (void)m; (void)cleft_nt; (void)dt;")
        lines.append("    return 1.0f;")
    lines.append("}")
    lines.append("")

    return '\n'.join(lines)


def generate_stub(header_mode=False):
    """Генерирует заглушку dispatch_generated.cpp/.h (без пользовательских плагинов)."""
    ext = "h" if header_mode else "cpp"
    prefix = "static inline " if header_mode else "BIOCORE_DEVICE_CALLABLE "
    return f"""// dispatch_generated.{ext} — заглушка для пользовательских типов (121-127).
// Генерируется SDK: biocore-gen-dispatch при добавлении пользовательских плагинов.
// Пересоберите этот файл после добавления плагинов для активации GPU/SYCL-пути.
// CPU-путь использует .so плагины через modifier_current_table[] — без пересборки.
#include "types.h"
#include "kernel_dispatch.h"

// Заглушка: все пользовательские типы возвращают 0.0f (нет тока)
{prefix}float dispatch_user_modifier_current(uint8_t type, Modifier* m, Segment* s, float dt) {{
    (void)type; (void)m; (void)s; (void)dt;
    return 0.0f;
}}

// Заглушка: все пользовательские модуляторы возвращают 1.0f (нет модуляции)
{prefix}float dispatch_user_release_modulator(uint8_t type, Modifier* m, float* cleft_nt, float dt) {{
    (void)type; (void)m; (void)cleft_nt; (void)dt;
    return 1.0f;
}}
"""


def main():
    parser = argparse.ArgumentParser(
        description='biocore-gen-dispatch — генератор dispatch_generated.cpp для BioNeuronCore'
    )
    parser.add_argument('plugins_dir', nargs='?', default=None,
                        help='Путь к директории с .desc файлами плагинов')
    parser.add_argument('-o', '--output', default=None,
                        help='Выходной файл (по умолчанию: stdout)')
    parser.add_argument('--core-path', default=None,
                        help='Путь к BioNeuronCore (для записи в src/dispatch_generated.cpp)')
    parser.add_argument('--stub', action='store_true',
                        help='Генерировать заглушку (без плагинов)')
    parser.add_argument('--header', action='store_true',
                        help='Генерировать .h с inline-функциями (для GPU/SYCL)')

    args = parser.parse_args()

    header_mode = args.header

    if args.stub or args.plugins_dir is None:
        output = generate_stub(header_mode)
    else:
        modifiers = scan_plugins_dir(args.plugins_dir)
        print(f"Найдено типов: {len(modifiers)}", file=sys.stderr)
        for m in modifiers:
            print(f"  [{m['id']}] {m.get('name','?')} (release={m.get('release',False)})",
                  file=sys.stderr)
        output = generate_dispatch(modifiers, header_mode)

    # Определяем выходной путь
    if args.core_path:
        out_path = Path(args.core_path) / 'src' / 'dispatch_generated.cpp'
    elif args.output:
        out_path = Path(args.output)
    else:
        print(output)
        return 0

    # Проверяем что существующий файл не заглушка или перезаписываем
    out_path = out_path.resolve()
    if out_path.exists():
        old = open(out_path).read()
        if 'АВТОГЕНЕРАЦИЯ' in old or 'заглушка' in old:
            pass  # перезаписываем
        else:
            print(f"WARNING: {out_path} существует и не похож на автогенерированный.",
                  file=sys.stderr)
            print("Делаем резервную копию...", file=sys.stderr)
            bak = str(out_path) + '.bak'
            open(bak, 'w').write(old)

    open(str(out_path), 'w').write(output)
    print(f"Сгенерирован: {out_path}", file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
