// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "browse.h"
#include <set>
namespace plexium {
inline std::vector<plex::Hub> search_rows(const plex::Json &data,const std::vector<plex::Item> &sections,const std::string &scope){
 auto rows=recommendation_rows(data);if(rows.empty()){auto values=plex::items(data,false);if(!values.empty())rows.push_back({"results","Results",false,std::move(values)});}
 std::set<std::string> allowed;for(const auto &s:sections)allowed.insert(s.id);
 for(auto &row:rows){row.entries.erase(std::remove_if(row.entries.begin(),row.entries.end(),[&](const auto&i){return (!scope.empty()&&i.section_id!=scope)||(!i.section_id.empty()&&!allowed.count(i.section_id));}),row.entries.end());}
 rows.erase(std::remove_if(rows.begin(),rows.end(),[](const auto&r){return r.entries.empty();}),rows.end());return rows;
}
inline bool cacheable_detail(const plex::Item &item){return item.media.is_null()||item.media.dump().size()<=512*1024;}
inline int64_t seek_position(int64_t position,int64_t delta,int64_t duration){
 auto limit=duration>0?duration:INT64_MAX;position=std::clamp<int64_t>(position,0,limit);
 if(delta>0)return delta>limit-position?limit:position+delta;
 return delta< -position?0:position+delta;
}
inline std::string duration_label(int64_t milliseconds){auto seconds=std::max<int64_t>(0,milliseconds)/1000;return std::to_string(seconds/3600)+":"+(seconds/60%60<10?"0":"")+std::to_string(seconds/60%60)+":"+(seconds%60<10?"0":"")+std::to_string(seconds%60);}
}
