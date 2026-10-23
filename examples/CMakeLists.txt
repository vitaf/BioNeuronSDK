# Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
# E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
# Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

# CMakeLists.txt — примеры плагинов BioNeuronSDK.
# Используйте как образец для своего проекта.

# Пример 1: Ca-канал (ионотропный, без пресинаптической модуляции)
biocore_add_plugin(example_ca_channel example_ca_channel.cpp
                   DESC example_ca_channel.desc)

# Пример 2: D2-рецептор (Gi/o, с пресинаптической модуляцией)
biocore_add_plugin(example_d2_receptor example_d2_receptor.cpp
                   DESC example_d2_receptor.desc)
