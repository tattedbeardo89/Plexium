// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "plex/home_feed.h"
namespace plexium {
// Keep server recommendations, including genre/related rails, rather than forcing
// every section into the closed six-row personal Home composition.
inline std::vector<plex::Hub> recommendation_rows(const plex::Json &data,const std::string &section="") {
 std::vector<plex::Hub> result;
 const plex::Json *container=&data;
 if(data.contains("MediaContainer"))container=&data["MediaContainer"];
 if(!container->is_object())return result;
 auto hubs=container->find("Hub");
 if(hubs==container->end()||!hubs->is_array())return result;
 for(const auto &hub:*hubs){
  auto entries=plex::items(plex::Json{{"MediaContainer",hub}},false);
  entries.erase(std::remove_if(entries.begin(),entries.end(),[&](const plex::Item &i){return !section.empty()&&!i.section_id.empty()&&i.section_id!=section;}),entries.end());
  if(entries.empty())continue;
  if(entries.size()>12)entries.resize(12);
  result.push_back({plex::text(hub,"hubIdentifier",std::to_string(result.size())),plex::text(hub,"title","Recommended"),false,std::move(entries)});
  if(result.size()==12)break;
 }
 return result;
}
inline bool audiobook_library(std::string title){
 std::transform(title.begin(),title.end(),title.begin(),[](unsigned char c){return std::tolower(c);});
 return title.find("book")!=std::string::npos;
}
inline std::vector<std::pair<std::string,std::string>> library_tabs(const std::string &type,bool books){
 if(type=="artist")return {{"recommended","Recommended"},{"artists",books?"Authors":"Artists"},{"albums",books?"Books":"Albums"},{"tracks",books?"Chapters":"Tracks"},{"genres","Genres"}};
 return {{"recommended","Recommended"},{"all","Library"},{"recent","Recent"},{"unwatched","Unwatched"},{"genres","Genres"},{"collections","Collections"}};
}
}
