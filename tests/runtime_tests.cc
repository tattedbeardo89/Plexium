#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/session.h"
#include <future>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
using namespace plexium;
struct Player:PlaybackPort{bool open(const std::string&,int64_t)override{return false;}void stop()override{}};
int main(){
 auto root="/tmp/plexium-runtime-"+std::to_string(getpid());std::filesystem::create_directory(root);test_posix_result(chmod(root.c_str(),0700),"chmod");
 TEST_REQUIRE(plex::save_credentials(root+"/account.json","fixture-device","fixture-account"));
 Player player;std::promise<void> entered,release;auto available=release.get_future().share();
 auto transport=[&](const plex::Request&){entered.set_value();available.wait();return plex::Reply{401,"{}",""};};
 {
  Runtime runtime(std::make_unique<Session>(transport,root,player));TEST_REQUIRE(runtime.send({Action::Restore,{},{}}));
  TEST_REQUIRE(entered.get_future().wait_for(std::chrono::seconds(5))==std::future_status::ready);
  auto before=std::chrono::steady_clock::now();TEST_REQUIRE(runtime.send({Action::Cancel,{},{}}));
  TEST_REQUIRE(std::chrono::steady_clock::now()-before<std::chrono::milliseconds(100));
  auto m=runtime.snapshot();TEST_REQUIRE(m.screen==Screen::Welcome&&m.busy);TEST_REQUIRE(runtime.media_context().token.empty());
  Model reusable;MediaContext access;reusable.entries.reserve(100);const auto capacity=reusable.entries.capacity();
  runtime.snapshot_into(reusable,access);TEST_REQUIRE(reusable.screen==m.screen&&reusable.busy==m.busy&&access.token.empty());
  TEST_REQUIRE(reusable.entries.capacity()==capacity); // persistent render buffer retains its allocation
  reusable.message="local-only";access.base="local-only";
  runtime.snapshot_into(reusable,access);TEST_REQUIRE(reusable.message.empty()&&access.base.empty());
  release.set_value();for(int i=0;i<5000&&runtime.snapshot().busy;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
  TEST_REQUIRE(!runtime.snapshot().busy);TEST_REQUIRE(runtime.snapshot().screen==Screen::Welcome);TEST_REQUIRE(runtime.snapshot().message.empty());
 }
 // A successful worker result must be copied from Session before its borrowed
 // view can change; snapshots remain independent, reusable caller storage.
 {
  TEST_REQUIRE(plex::save_credentials(root+"/account.json","fixture-device","fixture-account"));
  Runtime runtime(std::make_unique<Session>([](const plex::Request &request){
   if(request.url=="https://plex.tv/api/v2/user")return plex::Reply{200,"{\"id\":1}",""};
   TEST_REQUIRE(request.url.find("/home/users")!=std::string::npos);
   return plex::Reply{200,"<users><User id=\"1\" title=\"Owner\" /></users>",""};
  },root,player));
  Model reusable;MediaContext access;reusable.profiles.reserve(10);
  TEST_REQUIRE(runtime.send({Action::Restore,{},{}}));
  for(int i=0;i<5000;++i){runtime.snapshot_into(reusable,access);if(!reusable.busy&&reusable.screen==Screen::Profiles)break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
  TEST_REQUIRE(!reusable.busy&&reusable.screen==Screen::Profiles&&reusable.profiles.size()==1);
  TEST_REQUIRE(reusable.profiles[0].name=="Owner"&&reusable.profiles.capacity()>=10);
  TEST_REQUIRE(access.account_token=="fixture-account");
  reusable.profiles[0].name="local-only";access.account_token.clear();
  runtime.snapshot_into(reusable,access);
  TEST_REQUIRE(reusable.profiles[0].name=="Owner"&&access.account_token=="fixture-account");
  runtime.close();runtime.close();TEST_REQUIRE(!runtime.ready());
  TEST_REQUIRE(!runtime.send({Action::Restore,{},{}}));
  TEST_REQUIRE(runtime.media_context().account_token.empty() && runtime.snapshot().profiles.empty());
 }
 std::filesystem::remove_all(root);std::cout<<"PASS: render-thread nonblocking dispatch, cancellation generation, stale result rejection, joined worker cleanup\n";
}
