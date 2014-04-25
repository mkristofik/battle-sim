/*
    Copyright (C) 2014 by Michael Kristofik <kristo605@gmail.com>
    Part of the battle-sim project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "json_utils.h"

#include "boost/filesystem.hpp"
#include "rapidjson/filestream.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <iterator>

bool jsonParse(const char *filename, rapidjson::Document &doc)
{
    boost::filesystem::path dataPath{"../data"};
    dataPath /= filename;
    std::shared_ptr<FILE> fp{fopen(dataPath.string().c_str(), "r"), fclose};
    if (!fp) {
        std::cerr << "Couldn't find file " << dataPath.string() << '\n';
        return false;
    }

    rapidjson::FileStream file(fp.get());
    if (doc.ParseStream<0>(file).HasParseError()) {
        std::cerr << "Error reading json file " << dataPath.string() << ": "
            << doc.GetParseError() << '\n';
        return false;
    }
    if (!doc.IsObject()) {
        std::cerr << "Expected top-level object in json file\n";
        return false;
    }

    return true;
}

std::vector<unsigned> jsonListUnsigned(const rapidjson::Value &json)
{
    std::vector<unsigned> ret;

    transform(json.Begin(), json.End(), std::back_inserter(ret),
              [&] (const rapidjson::Value &elem) { return elem.GetInt(); });
    return ret;
}

std::vector<std::string> jsonListStr(const rapidjson::Value &json)
{
    std::vector<std::string> ret;

    transform(json.Begin(), json.End(), std::back_inserter(ret),
              [&] (const rapidjson::Value &elem) { return elem.GetString(); });
    return ret;
}
