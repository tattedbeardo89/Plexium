// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "plex/client.h"
#include "../check.h"
struct Fixture {
 int decisions=0,timelines=0,stops=0,pings=0;bool outage=false,bad=false;std::string video="copy",audio="copy",last_decision,last_timeline;
 plex::Json metadata=plex::Json::parse(R"({"ratingKey":"1","key":"/library/metadata/1","type":"movie","title":"Synthetic film","duration":90000,"viewOffset":1000,"Media":[{"container":"mp4","videoCodec":"h264","audioCodec":"aac","width":1920,"height":1080,"Part":[{"key":"/library/parts/1/file.mp4"}]},{"container":"avi","videoCodec":"mpeg4","audioCodec":"aac","width":1920,"height":1080,"Part":[{"key":"/library/parts/2/file.avi"}]}]})");
 plex::Reply operator()(const plex::Request &q){
  auto ok=[](plex::Json j){return plex::Reply{200,j.dump(),""};};
  auto has=[&](const char *p){return q.url.find(p)!=std::string::npos;};
  if(has("/decision")){
   ++decisions;last_decision=q.url;TEST_REQUIRE(q.headers.at("X-Plex-Token")=="server-token");
   if(bad)return ok({{"MediaContainer",{{"generalDecisionCode",1001}}}});
   return ok({{"MediaContainer",{{"generalDecisionCode",1001},{"Metadata",plex::Json::array({{{"ratingKey","1"},{"Media",plex::Json::array({{{"videoCodec","h264"},{"audioCodec","aac"},{"videoDecision",video},{"audioDecision",audio},{"width",1920},{"height",1080},{"Part",plex::Json::array({{{"key","/ignored-server-url"}}})}}})}}})}}}});
  }
  if(has("universal/stop")){++stops;return outage?plex::Reply{503,"{}",""}:ok({});}
  if(has("universal/ping")){++pings;TEST_REQUIRE(q.headers.at("X-Plex-Token")=="server-token");return ok({});}
  if(has("/:/timeline")){++timelines;last_timeline=q.url;TEST_REQUIRE(q.headers.at("X-Plex-Token")=="server-token");if(!outage){auto at=q.url.find("time=");if(at!=std::string::npos){at+=5;metadata["viewOffset"]=plex::number(plex::Json{{"position",q.url.substr(at,q.url.find('&',at)-at)}},"position");}}return outage?plex::Reply{503,"{}",""}:ok({});}
  if(has("/api/v2/user"))return ok({{"id",7}});
  if(has("/api/v2/home/users"))return ok({{"users",plex::Json::array({{{"id",1},{"title","Owner"},{"protected",false}}})}});
  if(has("/api/home/users/"))return ok({{"id",1},{"authenticationToken","profile-token"}});
  if(has("/api/v2/resources"))return ok(plex::Json::array({{{"name","Fixture"},{"clientIdentifier","server"},{"provides","server"},{"accessToken","server-token"},{"connections",plex::Json::array({{{"uri","https://fixture.test"},{"local",true}}})}}}));
  if(has("/identity"))return ok({{"MediaContainer",{{"machineIdentifier","server"}}}});
  if(has("/library/sections"))return ok({{"MediaContainer",{{"Directory",plex::Json::array({{{"key","1"},{"title","Movies"},{"type","movie"}}})}}}});
  if(has("/library/metadata/")&&!has("/related"))return ok({{"MediaContainer",{{"Metadata",plex::Json::array({metadata})}}}});
  return ok({{"MediaContainer",{{"Metadata",plex::Json::array()}}}});
 }
};
