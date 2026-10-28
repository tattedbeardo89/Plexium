// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/session.h"
#include <cassert>
#include <future>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
using namespace plexium;
struct Player:PlaybackPort{bool open(const std::string&,int64_t)override{return false;}void stop()override{}};
int main(){
 auto root="/tmp/plexium-runtime-"+std::to_string(getpid());std::filesystem::create_directory(root);chmod(root.c_str(),0700);
 assert(plex::save_credentials(root+"/account.json","fixture-device","fixture-account"));
 Player player;std::promise<void> entered,release;auto available=release.get_future().share();
 auto transport=[&](const plex::Request&){entered.set_value();available.wait();return plex::Reply{401,"{}",""};};
 {
  Runtime runtime(std::make_unique<Session>(transport,root,player));assert(runtime.send({Action::Restore,{},{}}));
  assert(entered.get_future().wait_for(std::chrono::seconds(5))==std::future_status::ready);
  auto before=std::chrono::steady_clock::now();assert(runtime.send({Action::Cancel,{},{}}));
  assert(std::chrono::steady_clock::now()-before<std::chrono::milliseconds(100));
  auto m=runtime.snapshot();assert(m.screen==Screen::Welcome&&m.busy);assert(runtime.media_context().token.empty());
  Model reusable;MediaContext access;reusable.entries.reserve(100);const auto capacity=reusable.entries.capacity();
  runtime.snapshot_into(reusable,access);assert(reusable.screen==m.screen&&reusable.busy==m.busy&&access.token.empty());
  assert(reusable.entries.capacity()==capacity); // persistent render buffer retains its allocation
  reusable.message="local-only";access.base="local-only";
  runtime.snapshot_into(reusable,access);assert(reusable.message.empty()&&access.base.empty());
  release.set_value();for(int i=0;i<5000&&runtime.snapshot().busy;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
  assert(!runtime.snapshot().busy);assert(runtime.snapshot().screen==Screen::Welcome);assert(runtime.snapshot().message.empty());
 }
 // A successful worker result must be copied from Session before its borrowed
 // view can change; snapshots remain independent, reusable caller storage.
 {
  assert(plex::save_credentials(root+"/account.json","fixture-device","fixture-account"));
  Runtime runtime(std::make_unique<Session>([](const plex::Request &request){
   if(request.url=="https://plex.tv/api/v2/user")return plex::Reply{200,"{\"id\":1}",""};
   assert(request.url.find("/home/users")!=std::string::npos);
   return plex::Reply{200,"<users><User id=\"1\" title=\"Owner\" /></users>",""};
  },root,player));
  Model reusable;MediaContext access;reusable.profiles.reserve(10);
  assert(runtime.send({Action::Restore,{},{}}));
  for(int i=0;i<5000;++i){runtime.snapshot_into(reusable,access);if(!reusable.busy&&reusable.screen==Screen::Profiles)break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
  assert(!reusable.busy&&reusable.screen==Screen::Profiles&&reusable.profiles.size()==1);
  assert(reusable.profiles[0].name=="Owner"&&reusable.profiles.capacity()>=10);
  assert(access.account_token=="fixture-account");
  reusable.profiles[0].name="local-only";access.account_token.clear();
  runtime.snapshot_into(reusable,access);
  assert(reusable.profiles[0].name=="Owner"&&access.account_token=="fixture-account");
 }
 std::filesystem::remove_all(root);std::cout<<"PASS: render-thread nonblocking dispatch, cancellation generation, stale result rejection, joined worker cleanup\n";
}
