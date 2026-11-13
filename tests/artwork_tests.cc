// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "plex/artwork.h"
#include "library.h"
#include <thread>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"
#pragma GCC diagnostic pop
namespace {
const auto render_thread=std::this_thread::get_id();
unsigned uploads=0,frees=0;
template<class Condition> void until(Condition condition){
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(!condition()){TEST_REQUIRE(std::chrono::steady_clock::now()<deadline);std::this_thread::yield();}
}
}
ImTextureID library_upload_rgba(const unsigned char *rgba,int width,int height){
 TEST_REQUIRE(std::this_thread::get_id()==render_thread);
 TEST_REQUIRE(rgba && width>0 && height>0);return ++uploads;
}
void library_free_image(ImTextureID id){TEST_REQUIRE(std::this_thread::get_id()==render_thread && id);++frees;}
int main(){
 std::atomic<unsigned> requests{0};std::atomic<bool> release{false};
 plex::Artwork artwork([&](const plex::Request&){
  TEST_REQUIRE(std::this_thread::get_id()!=render_thread);++requests;
  until([&]{return release.load();});
  return plex::Reply{200,std::string("P6\n1 1\n255\n")+char(255)+char(0)+char(0),""};
 });
 TEST_REQUIRE(!artwork.get("https://fixture","profile","/one"));
 TEST_REQUIRE(!artwork.get("https://fixture","profile","/two"));
 TEST_REQUIRE(!artwork.avatar("https://plex.tv/avatar","profile"));
 until([&]{return requests==3;});
 TEST_REQUIRE(!artwork.get("https://fixture","profile","/three") && requests==3);
 // A profile change retires the three active results without uploading them.
 artwork.clear();release=true;
 until([&]{artwork.update();return !artwork.busy();});
 TEST_REQUIRE(uploads==0 && artwork.cached()==0);
 TEST_REQUIRE(!artwork.get("https://fixture","profile","/new"));
 until([&]{artwork.update();return !artwork.busy();});
 TEST_REQUIRE(uploads==1 && artwork.get("https://fixture","profile","/new")==1);
 TEST_REQUIRE(requests==4);
 artwork.clear();artwork.update();TEST_REQUIRE(frees==1);
 artwork.close();artwork.close();artwork.update();
 TEST_REQUIRE(!artwork.busy() && !artwork.get("https://fixture","profile","/closed") && requests==4);
 // Closing active jobs cooperates at the network boundary, joins all lanes,
 // discards their results and performs no renderer calls.
 std::atomic<bool> entered{false},finish{false};
 plex::Artwork closing([&](const plex::Request&){entered=true;until([&]{return finish.load();});return plex::Reply{200,"",""};});
 closing.get("https://fixture","profile","/pending");until([&]{return entered.load();});
 TEST_REQUIRE(!closing.close(std::chrono::milliseconds(0)) && closing.busy());
 auto release_thread=std::thread([&]{finish=true;});closing.close();release_thread.join();
 TEST_REQUIRE(!closing.busy() && uploads==1 && frees==1);
 puts("PASS: production artwork three-lane bound, async fetch/decode, stale-profile discard, render-thread upload and retirement");
}
