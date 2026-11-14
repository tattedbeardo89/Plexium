// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "plex/streaming.h"
#include <iostream>
using namespace plex;
int main(){
 Item item;item.id="1";item.type="movie";item.media=Json::parse(R"([{"container":"mkv","videoCodec":"h264","audioCodec":"aac","width":1920,"height":1080,"Part":[{"id":100,"key":"/library/parts/1/file.mkv","Stream":[{"id":10,"streamType":2,"languageCode":"eng","codec":"aac","channels":2,"displayTitle":"English Stereo"},{"id":11,"streamType":2,"languageCode":"spa","codec":"truehd","channels":8},{"id":20,"streamType":3,"languageCode":"eng","codec":"srt","key":"/library/streams/20"},{"id":21,"streamType":3,"languageCode":"spa","codec":"pgs"}]}]}])");
 auto audio=media_tracks(item,0,false),subtitles=media_tracks(item,0,true);TEST_REQUIRE(audio.size()==2&&subtitles.size()==2&&audio[1].label.find("8 ch")!=std::string::npos);
 std::vector<NativeTrack> native={{42,"English","eng","mp4a",2},{88,"Spanish","spa","",8}};
 TEST_REQUIRE(native_track_match(audio[0],native)==42&&native_track_match(audio[1],native)==88);native.push_back(native[0]);TEST_REQUIRE(native_track_match(audio[0],native)==-1);
 std::string request;Client client([&](const Request&q){request=q.url;return Reply{200,R"({"MediaContainer":{"generalDecisionCode":1001,"Metadata":[{"ratingKey":"1","Media":[{"videoCodec":"h264","audioCodec":"aac","videoDecision":"transcode","audioDecision":"transcode","width":1920,"height":1080,"Part":[{"Stream":[{"streamType":3,"decision":"burn"}]}]}]}]}})",""};},"fixture");
 auto plan=negotiate_video(client,"https://fixture","token",item,0,Quality::Original,true,"session",false,{"11","21"});TEST_REQUIRE(plan.playback.mode==Mode::Transcode);TEST_REQUIRE(request.find("audioStreamID=11")!=std::string::npos&&request.find("subtitleStreamID=21")!=std::string::npos&&request.find("subtitles=burn")!=std::string::npos);
 TEST_REQUIRE(negotiate_video(client,"https://fixture","token",item,0,Quality::Original,true,"session",false,{"999",""}).playback.mode==Mode::Unsupported);
 auto direct=negotiate_video(client,"https://fixture","token",item,0,Quality::Original,true,"session",false,{"10","20"});TEST_REQUIRE(direct.playback.mode==Mode::DirectPlay);
 TEST_REQUIRE(external_subtitle("https://fixture","token",Json{{"streamType",3},{"codec","srt"},{"key","https://foreign/secret"}}).empty());
 std::cout<<"PASS: named/language/codec/channel streams, distinct native ES IDs, ambiguous rejection, selected-stream negotiation and subtitle burning policy\n";
}
