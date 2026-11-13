// SPDX-License-Identifier: GPL-3.0-or-later
#include "plex/guide.h"
#include <cassert>
#include <iostream>
using namespace plex;
int main(){
 int calls=0;
 Client empty([&](const Request&q){++calls;assert(q.url=="https://fixture/livetv/dvrs");return Reply{200,R"({"MediaContainer":{"size":0}})",""};},"fixture");
 auto response=live_channels(empty,"https://fixture","profile");assert(response&&!flag(response.data,"configured")&&response.data["channels"].empty()&&calls==1);
 for(const auto &body:{"null","[]","true",R"({"MediaContainer":{"Dvr":false}})",R"({"MediaContainer":[]})",R"({"MediaContainer":{}})"}){
  Client bad([&](const Request&){return Reply{200,body,""};},"fixture");assert(!live_channels(bad,"https://fixture","profile"));
 }
 for(int status:{401,403,404,500,503}){
  calls=0;Client failed([&](const Request&){++calls;return Reply{status,"{}",""};},"fixture");
  auto failure=live_channels(failed,"https://fixture","profile");assert(!failure&&failure.status==status&&calls==1);
 }
 Client server([&](const Request &q){
  assert(q.headers.at("X-Plex-Token")=="profile");
  assert(q.url.rfind("https://fixture/",0)==0);++calls;
  if(q.url=="https://fixture/livetv/dvrs")return Reply{200,R"({"MediaContainer":{"Dvr":[{"key":"12"}]}})",""};
  if(q.url=="https://fixture/media/providers")return Reply{200,R"({"MediaContainer":{"MediaProvider":[{"identifier":"tv.plex.providers.epg.xmltv:12"},{"identifier":"tv.plex.providers.epg.cloud:99"},{"identifier":"https://other"}]}})",""};
  assert(q.url.find("xmltv:12/lineups/dvr/channels")!=std::string::npos);
  return Reply{200,R"({"MediaContainer":{"Channel":[{"id":"authorized","gridKey":"grid1","vcn":"2.1","title":"From Plex"},{"id":"../bad","gridKey":"grid2"},false]}})",""};
 },"fixture");
 calls=0;response=live_channels(server,"https://fixture","profile");assert(response&&flag(response.data,"configured")&&response.data["channels"].size()==1&&calls==3);
 const auto channel=response.data["channels"][0];
 Client guide([&](const Request&q){assert(q.url=="https://fixture/tv.plex.providers.epg.xmltv:12/grid?channelGridKey=grid1&date=2026-11-12");return Reply{200,R"({"MediaContainer":{"Metadata":[null,{"title":"Later","Media":[{"beginsAt":2000,"endsAt":3000}]},{"title":"Earlier","summary":"Supplied by Plex","Media":[{"beginsAt":1000,"endsAt":2100}]},{"title":"Invalid","Media":[{"beginsAt":3000,"endsAt":1}]}]}})",""};},"fixture");
 auto programs=live_guide(guide,"https://fixture","profile",channel,"2026-11-12");assert(programs&&programs.data["programs"].size()==2);
 auto parsed=guide_programs(programs.data,1800,2800);assert(parsed.size()==2&&parsed[0].title=="Earlier"&&parsed[0].summary=="Supplied by Plex");
 assert(guide_programs(programs.data,0,2800).empty());assert(guide_programs(programs.data,1800,100000).empty());
 assert(guide_programs(Json{{"programs",false}},1800,2800).empty());
 assert(channel_order("2.1")<channel_order("2.10")&&channel_order("2.10")<channel_order("10.1"));
 assert(channel_order("18446744073709551616")==std::vector<uint64_t>{UINT64_MAX});
 assert(channel_order("2.")==std::vector<uint64_t>{UINT64_MAX});
 assert(live_date("2024-02-29")&&!live_date("2026-02-29")&&!live_date("2026-13-01")&&!live_date("----------"));
 assert(guide_date(0)=="1970-01-01");assert(guide_date(1794527999)=="2026-11-12"&&guide_date(1794528000)=="2026-11-13");
 // Incomplete tuning must not allocate a tuner or fabricate a stream URL.
 calls=0;Client no_tune([&](const Request&){++calls;return Reply{};},"fixture");
 assert(!live_tune(no_tune,"https://fixture","profile",channel,"consumer")&&calls==0);
 std::cout<<"PASS: DVR empty/malformed/unauthorized/server failures, scoped channel discovery, guide missing fields/ranges/order, midnight, disabled unverified tuning\n";
}
