/*
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

#include <map>

#include "pugixml.hpp"

#include "libzzub/archive.h"
#include "libzzub/player.h"
#include "libzzub/ccm_helpers.h"


namespace zzub {
    
using namespace pugi;


struct ccache { // connection and node cache used in CcmReader
    int target;
    xml_node connections;
    xml_node global;
    xml_node tracks;
    xml_node sequences;
    xml_node eventtracks;
    xml_node midi;
};


class CcmReader : pugi::xml_tree_walker {
    ArchiveReader arch;

    typedef std::map<std::string, xml_node> idnodemap;
    idnodemap nodes;

    void registerNodeById(xml_node &item);
    xml_node getNodeById(const std::string &id);
    virtual bool for_each(xml_node&);

    bool loadClasses(xml_node &classes, zzub::player &player);
    bool loadPlugins(xml_node plugins, zzub::player &player);
    bool loadInstruments(xml_node &instruments, zzub::player &player);
    bool loadSequencer(xml_node &seq, zzub::player &player);
public:
    bool open(std::string fileName, zzub::player* player);
};

}
