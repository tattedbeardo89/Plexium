// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "live_tv.h"
#include <ctime>
namespace plex {
struct GuideProgram {std::string title,series,summary;int64_t begins=0,ends=0;};
struct GuideChannel {std::string id,number,title,thumb;std::vector<GuideProgram> programs;};
struct Guide {std::vector<GuideChannel> channels;int offset=0,total=0;int64_t begins=0,ends=0,refreshed=0;bool configured=false,stale=false;};
inline std::string guide_date(int64_t seconds){time_t value=seconds;struct tm utc{};if(!gmtime_r(&value,&utc))return {};char out[11];if(!strftime(out,sizeof out,"%Y-%m-%d",&utc))return {};return out;}
inline std::string guide_time(int64_t seconds){time_t value=seconds;struct tm local{};if(!localtime_r(&value,&local))return {};char out[6];strftime(out,sizeof out,"%H:%M",&local);return out;}
inline std::vector<GuideProgram> guide_programs(const Json &data,int64_t begins,int64_t ends){
 std::vector<GuideProgram> result;auto list=data.find("programs");
 if(begins<=0 || ends<=begins || ends-begins>6*3600 || list==data.end() || !list->is_array())return result;
 for(const auto &item:*list){
  auto start=number(item,"beginsAt"),stop=number(item,"endsAt");
  if(start<=0 || stop<=start || stop<=begins || start>=ends)continue;
  result.push_back({live_label(text(item,"title")),live_label(text(item,"series")),live_label(text(item,"summary")),start,stop});if(result.size()==100)break;
 }
 std::stable_sort(result.begin(),result.end(),[](const auto&a,const auto&b){return a.begins<b.begins;});
 result.erase(std::unique(result.begin(),result.end(),[](const auto&a,const auto&b){return a.begins==b.begins&&a.ends==b.ends&&a.title==b.title;}),result.end());
 return result;
}
// Numeric channel components compare naturally: 2.1, 2.10, 10.1. Labels remain
// Plex's original values. No narrowing conversion or floating-point rounding.
inline std::vector<uint64_t> channel_order(const std::string &value){
 std::vector<uint64_t> out;uint64_t part=0;bool digit=false;
 for(unsigned char c:value){if(c>='0'&&c<='9'){if(part>1000000)return {UINT64_MAX};part=part*10+c-'0';digit=true;}else if(c=='.'&&digit){out.push_back(part);part=0;digit=false;}else return {UINT64_MAX};if(out.size()>4)return {UINT64_MAX};}
 if(!digit)return {UINT64_MAX};out.push_back(part);return out;
}
}
