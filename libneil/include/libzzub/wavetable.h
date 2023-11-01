/*
  Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "libzzub/wave_info.h"

/*#include <fcntl.h>
  #include <sys/stat.h>
  #include <sys/types.h>
*/
namespace zzub {

struct wave_proxy {
    player* _player;
    int wave;

    wave_proxy(player* _playr, int _wave):_player(_playr), wave(_wave) { }
};

struct wavelevel_proxy {
    player* _player;
    int wave;
    int level;

    wavelevel_proxy(player* _playr, int _wave, int _level)
        :_player(_playr), wave(_wave), level(_level) { }
};




struct wave_table {
    std::vector<wave_info_ex*> waves;
    wave_info_ex monitorwave; // for prelistening

    wave_table(void);
    ~wave_table(void);
    void clear();
};

};
