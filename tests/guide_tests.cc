#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plex/guide.h"
#include <iostream>
using namespace plex;
int main(){
 int calls=0;
 Client empty([&](const Request&q){++calls;TEST_REQUIRE(q.url=="https://fixture/livetv/dvrs");return Reply{200,R"({"MediaContainer":{"size":0}})",""};},"fixture");
 auto response=live_channels(empty,"https://fixture","profile");TEST_REQUIRE(response&&!flag(response.data,"configured")&&response.data["channels"].empty()&&calls==1);
 for(const auto &body:{"null","[]","true",R"({"MediaContainer":{"Dvr":false}})",R"({"MediaContainer":[]})",R"({"MediaContainer":{}})"}){
  Client bad([&](const Request&){return Reply{200,body,""};},"fixture");TEST_REQUIRE(!live_channels(bad,"https://fixture","profile"));
 }
 for(int status:{401,403,404,500,503}){
  calls=0;Client failed([&](const Request&){++calls;return Reply{status,"{}",""};},"fixture");
  auto failure=live_channels(failed,"https://fixture","profile");TEST_REQUIRE(!failure&&failure.status==status&&calls==1);
 }
 Client server([&](const Request &q){
  TEST_REQUIRE(q.headers.at("X-Plex-Token")=="profile");
  TEST_REQUIRE(q.url.rfind("https://fixture/",0)==0);++calls;
  if(q.url=="https://fixture/livetv/dvrs")return Reply{200,R"({"MediaContainer":{"Dvr":[{"key":"12"}]}})",""};
  if(q.url=="https://fixture/media/providers")return Reply{200,R"({"MediaContainer":{"MediaProvider":[{"identifier":"tv.plex.providers.epg.xmltv:12"},{"identifier":"tv.plex.providers.epg.cloud:99"},{"identifier":"https://other"}]}})",""};
  TEST_REQUIRE(q.url.find("xmltv:12/lineups/dvr/channels")!=std::string::npos);
  return Reply{200,R"({"MediaContainer":{"Channel":[{"id":"authorized","gridKey":"grid1","vcn":"2.1","title":"From Plex"},{"id":"../bad","gridKey":"grid2"},false]}})",""};
 },"fixture");
 calls=0;response=live_channels(server,"https://fixture","profile");TEST_REQUIRE(response&&flag(response.data,"configured")&&response.data["channels"].size()==1&&calls==3);
 const auto channel=response.data["channels"][0];
 // Valid DVR discovery must not turn malformed provider/lineup data into a
 // successful "no channels" result.
 for(bool bad_lineup:{false,true})for(const auto &body:{"null",R"({"MediaContainer":{}})",R"({"MediaContainer":{"Channel":false,"MediaProvider":false}})"}){
  Client malformed([&](const Request&q){
   if(q.url.find("/livetv/dvrs")!=std::string::npos)return Reply{200,R"({"MediaContainer":{"Dvr":[{"key":"12"}]}})",""};
   if(bad_lineup && q.url.find("/media/providers")!=std::string::npos)return Reply{200,R"({"MediaContainer":{"MediaProvider":[{"identifier":"tv.plex.providers.epg.xmltv:12"}]}})",""};
   return Reply{200,body,""};
  },"fixture");
  TEST_REQUIRE(!live_channels(malformed,"https://fixture","profile"));
 }
 for(const auto &body:{"null","[]","true",R"({"MediaContainer":[]})",R"({"MediaContainer":{}})",R"({"MediaContainer":{"Metadata":false}})"}){
  Client bad([&](const Request&){return Reply{200,body,""};},"fixture");
  TEST_REQUIRE(!live_guide(bad,"https://fixture","profile",channel,"2026-11-12"));
 }
 for(const auto &body:{R"({"MediaContainer":{"size":0}})",R"({"MediaContainer":{"Metadata":[]}})"}){
  Client empty_grid([&](const Request&){return Reply{200,body,""};},"fixture");
  auto empty_programs=live_guide(empty_grid,"https://fixture","profile",channel,"2026-11-12");
  TEST_REQUIRE(empty_programs && empty_programs.data["programs"].empty());
 }
 Json daily=Json::array();
 for(int i=0;i<150;++i)daily.push_back({{"title","Past"},{"Media",Json::array({Json{{"beginsAt",100+i*10},{"endsAt",110+i*10}}})}});
 daily.push_back({{"title","Current from Plex"},{"Media",Json::array({Json{{"beginsAt",2000},{"endsAt",3000}}})}});
 Client large_grid([&](const Request&){return Reply{200,Json{{"MediaContainer",{{"Metadata",daily}}}}.dump(),""};},"fixture");
 auto current=live_guide(large_grid,"https://fixture","profile",channel,"2026-11-12",2000,4000);
 TEST_REQUIRE(current && current.data["programs"].size()==1 && text(current.data["programs"][0],"title")=="Current from Plex");
 Client guide([&](const Request&q){TEST_REQUIRE(q.url=="https://fixture/tv.plex.providers.epg.xmltv:12/grid?channelGridKey=grid1&date=2026-11-12");return Reply{200,R"({"MediaContainer":{"Metadata":[null,{"title":"Later","Media":[{"beginsAt":2000,"endsAt":3000}]},{"title":"Earlier","summary":"Supplied by Plex","Media":[{"beginsAt":1000,"endsAt":2100}]},{"title":"Invalid","Media":[{"beginsAt":3000,"endsAt":1}]}]}})",""};},"fixture");
 auto programs=live_guide(guide,"https://fixture","profile",channel,"2026-11-12");TEST_REQUIRE(programs&&programs.data["programs"].size()==2);
 auto parsed=guide_programs(programs.data,1800,2800);TEST_REQUIRE(parsed.size()==2&&parsed[0].title=="Earlier"&&parsed[0].summary=="Supplied by Plex");
 TEST_REQUIRE(guide_programs(programs.data,0,2800).empty());TEST_REQUIRE(guide_programs(programs.data,1800,100000).empty());
 TEST_REQUIRE(guide_programs(Json{{"programs",false}},1800,2800).empty());
 TEST_REQUIRE(channel_order("2.1")<channel_order("2.10")&&channel_order("2.10")<channel_order("10.1"));
 TEST_REQUIRE(channel_order("18446744073709551616")==std::vector<uint64_t>{UINT64_MAX});
 TEST_REQUIRE(channel_order("2.")==std::vector<uint64_t>{UINT64_MAX});
 TEST_REQUIRE(live_date("2024-02-29")&&!live_date("2026-02-29")&&!live_date("2026-13-01")&&!live_date("----------"));
 TEST_REQUIRE(guide_date(0)=="1970-01-01");TEST_REQUIRE(guide_date(1794527999)=="2026-11-12"&&guide_date(1794528000)=="2026-11-13");
 // Incomplete tuning must not allocate a tuner or fabricate a stream URL.
 calls=0;Client no_tune([&](const Request&){++calls;return Reply{};},"fixture");
 TEST_REQUIRE(!live_tune(no_tune,"https://fixture","profile",channel,"consumer")&&calls==0);
 // Large lineups stay bounded; later-page authorization revocation fails closed.
 for(int scenario:{0,1,2}){
  int pages=0;Client large([&](const Request&q){
   if(q.url.find("/livetv/dvrs")!=std::string::npos)return Reply{200,R"({"MediaContainer":{"Dvr":[{"key":"12"}]}})",""};
   if(q.url.find("/media/providers")!=std::string::npos)return Reply{200,R"({"MediaContainer":{"MediaProvider":[{"identifier":"tv.plex.providers.epg.xmltv:12"}]}})",""};
   auto at=q.url.find("X-Plex-Container-Start=");int start=std::stoi(q.url.substr(at+23));++pages;
   if(scenario==1&&start>0)return Reply{401,"{}",""};
   if(scenario==2)start=0;Json channels=Json::array();for(int n=start;n<start+200;++n)channels.push_back({{"id","c"+std::to_string(n)},{"gridKey","g"+std::to_string(n)}});
   return Reply{200,Json{{"MediaContainer",{{"Channel",channels},{"totalSize",4000}}}}.dump(),""};
  },"fixture");
  auto lineup=live_channels(large,"https://fixture","profile");
  if(scenario==1)TEST_REQUIRE(!lineup&&lineup.status==401&&pages==2);
  else TEST_REQUIRE(lineup&&flag(lineup.data,"truncated")&&lineup.data["channels"].size()==(scenario==0?1600u:200u)&&pages<=8);
 }
 std::cout<<"PASS: DVR empty/malformed/unauthorized/server failures, scoped channel discovery, guide missing fields/ranges/order, midnight, disabled unverified tuning\n";
}
