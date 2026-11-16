// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "plex/progress_journal.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace plex;
int main(){
 char directory[]="/tmp/plexium-journal-XXXXXX";TEST_REQUIRE(mkdtemp(directory));std::string root=directory;
 ProgressJournal journal;ProgressOutbox box;TEST_REQUIRE(journal.bind(root,"7/1/server",box,1000,10));
 Item item;item.id="1";item.type="episode";item.duration=90000;item.offset=12000;item.last_played=900;
 box.put(item,"playing",20000,1000,"session1");box.failed("1",box.entries[0].sequence,10);double due=box.entries[0].due;
 box.put(item,"paused",22000,1001,"session1");TEST_REQUIRE(box.entries.size()==1&&box.entries[0].attempts==1&&box.entries[0].due==due);
 TEST_REQUIRE(journal.save(box,1001,10));
 ProgressJournal restart;ProgressOutbox recovered;TEST_REQUIRE(restart.bind(root,"7/1/server",recovered,1002,20));TEST_REQUIRE(recovered.entries.size()==1&&recovered.entries[0].recovered&&recovered.entries[0].position==22000&&recovered.entries[0].due>20);
 ProgressJournal foreign;ProgressOutbox other;TEST_REQUIRE(foreign.bind(root,"7/2/server",other,1002,20)&&other.entries.empty());
 auto update=recovered.entries[0];Item server=item;TEST_REQUIRE(reconcile_progress(update,server)==Reconcile::Send);
 server.offset=23000;TEST_REQUIRE(reconcile_progress(update,server)==Reconcile::Conflict);
 server.offset=22000;TEST_REQUIRE(reconcile_progress(update,server)==Reconcile::AlreadyApplied);
 server.last_played=1003;TEST_REQUIRE(reconcile_progress(update,server)==Reconcile::Conflict);
 // A torn staging file is ignored; the committed journal is still authoritative.
 {std::ofstream staged(root+"/progress-v1.json.tmp");staged<<"{broken";}test_posix_result(chmod((root+"/progress-v1.json.tmp").c_str(),0600),"private staging file");
 TEST_REQUIRE(restart.bind(root,"7/1/server",recovered,1002,20)&&recovered.entries.size()==1);
 recovered.ack("1",recovered.entries[0].sequence);TEST_REQUIRE(restart.save(recovered,1003,20));TEST_REQUIRE(restart.bind(root,"7/1/server",recovered,1004,20)&&recovered.entries.empty());
 Json bytes;TEST_REQUIRE(plexium::read_private_json(root+"/progress-v1.json",bytes));TEST_REQUIRE(bytes["schema"]==1);
 TEST_REQUIRE(bytes.dump().find("Token")==std::string::npos&&bytes.dump().find("https:")==std::string::npos);
 TEST_REQUIRE(plexium::write_private_json(root+"/progress-v1.json",Json{{"schema",99},{"records",Json::array()}}));TEST_REQUIRE(!restart.bind(root,"7/1/server",recovered,1004,20));TEST_REQUIRE(!restart.save(recovered,1005,20));
 test_posix_result(unlink((root+"/progress-v1.json").c_str()),"remove fixture");test_posix_result(symlink("/nonexistent",(root+"/progress-v1.json").c_str()),"symlink fixture");TEST_REQUIRE(!restart.bind(root,"7/1/server",recovered,1005,20));
 ProgressOutbox bounded;for(int i=0;i<128;++i){item.id=std::to_string(i);TEST_REQUIRE(bounded.put(item,"stopped",i,1000,"run"));}item.id="overflow";TEST_REQUIRE(!bounded.put(item,"stopped",0,1000,"run"));
 std::filesystem::remove_all(root);std::cout<<"PASS: durable bounded journal, coalescing/backoff, restart/scope isolation, newer-server conflict, duplicate ack, torn staging recovery, schema and symlink rejection\n";
}
