#!/usr/bin/env python3
# Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
# E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
# Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

"""Тесты biocore-gen-dispatch.py: parse_desc_file, scan_plugins_dir, generate_dispatch, generate_stub."""

import os
import sys
import json
import tempfile
import unittest

# Добавляем src/ в путь для импорта модуля
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))
import importlib.util
spec = importlib.util.spec_from_file_location(
    "dispatch_gen",
    os.path.join(os.path.dirname(__file__), '..', 'src', 'biocore-gen-dispatch.py')
)
dispatch_gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dispatch_gen)


class TestParseDescFile(unittest.TestCase):
    """Тесты parse_desc_file()."""

    def test_valid_modifiers_array(self):
        """Парсинг .desc с массивом modifiers."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.desc', delete=False) as f:
            json.dump({
                "modifiers": [
                    {"id": 121, "name": "Ca-канал", "category": "Ионотропные", "release": False},
                    {"id": 122, "name": "D2-рецептор", "category": "Метаботропные", "release": True}
                ]
            }, f)
            path = f.name

        try:
            mods = dispatch_gen.parse_desc_file(path)
            self.assertEqual(len(mods), 2)
            self.assertEqual(mods[0]['id'], 121)
            self.assertEqual(mods[0]['name'], 'Ca-канал')
            self.assertFalse(mods[0]['release'])
        finally:
            os.unlink(path)

    def test_single_modifier_format(self):
        """Парсинг старого формата (один модификатор без массива)."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.desc', delete=False) as f:
            json.dump({"id": 125, "name": "Test", "release": False}, f)
            path = f.name

        try:
            mods = dispatch_gen.parse_desc_file(path)
            self.assertEqual(len(mods), 1)
            self.assertEqual(mods[0]['id'], 125)
        finally:
            os.unlink(path)

    def test_invalid_json(self):
        """Невалидный JSON — возвращает пустой список."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.desc', delete=False) as f:
            f.write("not valid json {{{")
            path = f.name

        try:
            mods = dispatch_gen.parse_desc_file(path)
            self.assertEqual(mods, [])
        finally:
            os.unlink(path)

    def test_no_modifiers_field(self):
        """JSON без поля modifiers — сам объект становится модификатором."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.desc', delete=False) as f:
            json.dump({"id": 125, "name": "Solo"}, f)
            path = f.name

        try:
            mods = dispatch_gen.parse_desc_file(path)
            self.assertEqual(len(mods), 1)
            self.assertEqual(mods[0]['id'], 125)
        finally:
            os.unlink(path)


class TestScanPluginsDir(unittest.TestCase):
    """Тесты scan_plugins_dir()."""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp()

    def tearDown(self):
        import shutil
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def test_empty_directory(self):
        """Пустая директория — пустой список."""
        mods = dispatch_gen.scan_plugins_dir(self.tmpdir)
        self.assertEqual(mods, [])

    def test_nonexistent_directory(self):
        """Несуществующая директория — пустой список."""
        mods = dispatch_gen.scan_plugins_dir("/tmp/__nonexistent_plugins__/")
        self.assertEqual(mods, [])

    def test_only_user_types(self):
        """Только типы 121-127 попадают в результат."""
        desc = {
            "modifiers": [
                {"id": 50, "name": "Builtin"},       # < 121 — игнорируется
                {"id": 120, "name": "BuiltinLast"},   # = 120 — игнорируется (последний встроенный)
                {"id": 121, "name": "User1"},          # >= 121 — включается
                {"id": 127, "name": "User2"},         # граничный
                {"id": 128, "name": "Beyond"}         # > 127 — игнорируется
            ]
        }
        with open(os.path.join(self.tmpdir, "test.desc"), 'w') as f:
            json.dump(desc, f)

        mods = dispatch_gen.scan_plugins_dir(self.tmpdir)
        ids = [m['id'] for m in mods]
        self.assertIn(121, ids)
        self.assertIn(127, ids)
        self.assertNotIn(10, ids)
        self.assertNotIn(120, ids)
        self.assertNotIn(128, ids)

    def test_source_file_tracked(self):
        """Поле _source содержит имя .desc файла."""
        desc = {"modifiers": [{"id": 121, "name": "Test"}]}
        with open(os.path.join(self.tmpdir, "my_plugin.desc"), 'w') as f:
            json.dump(desc, f)

        mods = dispatch_gen.scan_plugins_dir(self.tmpdir)
        self.assertEqual(mods[0]['_source'], 'my_plugin.desc')


class TestMakeFuncName(unittest.TestCase):
    """Тесты make_func_name()."""

    def test_current_func(self):
        fn = dispatch_gen.make_func_name(121, "CaChannel", False)
        self.assertEqual(fn, "plugin_mod_121_current")

    def test_release_func(self):
        fn = dispatch_gen.make_func_name(122, "D2Receptor", True)
        self.assertEqual(fn, "plugin_mod_122_release")


class TestGenerateStub(unittest.TestCase):
    """Тесты generate_stub()."""

    def test_stub_cpp_mode(self):
        stub = dispatch_gen.generate_stub(header_mode=False)
        self.assertIn("BIOCORE_DEVICE_CALLABLE", stub)
        self.assertIn("dispatch_user_modifier_current", stub)
        self.assertIn("dispatch_user_release_modulator", stub)
        self.assertIn("return 0.0f", stub)
        self.assertIn("return 1.0f", stub)

    def test_stub_header_mode(self):
        stub = dispatch_gen.generate_stub(header_mode=True)
        self.assertIn("static inline", stub)
        self.assertIn("dispatch_user_modifier_current", stub)


class TestGenerateDispatch(unittest.TestCase):
    """Тесты generate_dispatch()."""

    def test_no_modifiers(self):
        """Без модификаторов — возвращает заглушку."""
        output = dispatch_gen.generate_dispatch([], header_mode=False)
        self.assertIn("(void)type", output)
        self.assertIn("return 0.0f", output)
        self.assertIn("return 1.0f", output)

    def test_with_current_modifiers(self):
        """С текущими модификаторами — switch с case."""
        mods = [
            {"id": 121, "name": "CaChannel", "release": False},
            {"id": 122, "name": "NaChannel", "release": False},
        ]
        output = dispatch_gen.generate_dispatch(mods, header_mode=False)
        self.assertIn("case 121:", output)
        self.assertIn("case 122:", output)
        self.assertIn("plugin_mod_121_current", output)
        self.assertIn("plugin_mod_122_current", output)
        self.assertIn("extern float plugin_mod_121_current", output)

    def test_with_release_modifiers(self):
        """С пресинаптическими модуляторами — extern + switch."""
        mods = [
            {"id": 125, "name": "D2", "release": True},
        ]
        output = dispatch_gen.generate_dispatch(mods, header_mode=False)
        self.assertIn("case 125:", output)
        self.assertIn("plugin_mod_125_release", output)
        self.assertIn("extern float plugin_mod_125_release", output)

    def test_header_mode_no_extern(self):
        """В header-режиме нет extern-деклараций."""
        mods = [{"id": 121, "name": "Test", "release": False}]
        output = dispatch_gen.generate_dispatch(mods, header_mode=True)
        self.assertNotIn("extern", output)
        self.assertIn("static inline", output)

    def test_generated_header_comment(self):
        """Вывод содержит предупреждение об автогенерации."""
        mods = [{"id": 121, "name": "Test", "release": False}]
        output = dispatch_gen.generate_dispatch(mods)
        self.assertIn("АВТОГЕНЕРАЦИЯ", output)
        self.assertIn("НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ", output)


class TestMain(unittest.TestCase):
    """Тесты main() — различные режимы запуска."""

    def test_stub_flag(self):
        """--stub генерирует заглушку."""
        import argparse
        old_argv = sys.argv
        try:
            sys.argv = ['biocore-gen-dispatch.py', '--stub']
            # Перенаправляем stdout
            import io
            saved_stdout = sys.stdout
            sys.stdout = io.StringIO()
            try:
                rc = dispatch_gen.main()
            finally:
                output = sys.stdout.getvalue()
                sys.stdout = saved_stdout
            self.assertEqual(rc, 0)
            self.assertIn("return 0.0f", output)
        finally:
            sys.argv = old_argv

    def test_default_stdout(self):
        """Без аргументов — заглушка в stdout."""
        import io
        old_argv = sys.argv
        try:
            sys.argv = ['biocore-gen-dispatch.py']
            saved_stdout = sys.stdout
            sys.stdout = io.StringIO()
            try:
                rc = dispatch_gen.main()
            finally:
                output = sys.stdout.getvalue()
                sys.stdout = saved_stdout
            self.assertEqual(rc, 0)
            self.assertIn("dispatch_user_modifier_current", output)
        finally:
            sys.argv = old_argv


if __name__ == '__main__':
    unittest.main()
