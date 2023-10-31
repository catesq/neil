/*
  Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
  Copyright (C) 2006-2007 Leonard Ritter

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

#include <cstring>
#include <cstdio>
#include <vector>
#include <list>
#include <map>
#include "libzzub/streams.h"


namespace zzub {


struct archive {
    virtual outstream *get_outstream(const char *path) = 0;
    virtual instream *get_instream(const char *path) = 0;
    virtual ~archive() {}
};


/*! \struct mem_archive
    \brief Memory-based archive stream
  */
struct mem_archive : zzub::archive {
    typedef std::map<std::string, std::vector<char> > buffermap;
    typedef std::pair<std::string, std::vector<char> > bufferpair;
    buffermap buffers;

    std::list<mem_outstream *> outstreams;
    std::list<mem_instream *> instreams;

    ~mem_archive() {
        for (std::list<mem_outstream *>::iterator i = outstreams.begin(); i != outstreams.end(); ++i) {
            delete *i;
        }
        for (std::list<mem_instream *>::iterator i = instreams.begin(); i != instreams.end(); ++i) {
            delete *i;
        }
    }

    std::vector<char> &get_buffer(const char *path) {
        buffermap::iterator i = buffers.find(path);
        if(i == buffers.end()) {
            buffers.insert(bufferpair(path, std::vector<char>()));
            i = buffers.find(path);
            assert(i != buffers.end());
        }
        return i->second;
    }

    virtual outstream *get_outstream(const char *path) {
        buffers.insert(bufferpair(path, std::vector<char>()));
        buffermap::iterator i = buffers.find(path);
        assert(i != buffers.end());
        mem_outstream *mos = new mem_outstream(i->second);
        outstreams.push_back(mos);
        return mos;
    }

    virtual instream *get_instream(const char *path) {
        buffermap::iterator i = buffers.find(path);
        if (i == buffers.end())
            return 0;
        mem_instream *mis = new mem_instream(i->second);
        instreams.push_back(mis);
        return mis;
    }
};

} // namespace zzub

