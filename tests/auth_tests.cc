#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plex/auth.h"
#include "qrcodegen/qrcodegen.h"
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
using namespace plex;
static Result response(int expiry=300, const Json &token=nullptr) {
    return {Json{{"id",12345},{"code","AB9Z"},{"authToken",token},
                 {"clientIdentifier","device"},{"expiresIn",expiry}}, "",200};
}
int main() {
    int calls=0;
    Client client([&](const Request &q) {
        ++calls;
        TEST_REQUIRE(q.headers.at("Accept")=="application/json");
        TEST_REQUIRE(q.headers.at("X-Plex-Client-Identifier")=="device");
        TEST_REQUIRE(q.headers.at("X-Plex-Product")=="Plexium");
        if(q.method=="POST") {
            TEST_REQUIRE(q.url=="https://plex.tv/api/v2/pins");
            TEST_REQUIRE(!q.headers.count("X-Plex-Token"));
            TEST_REQUIRE(q.body==(calls==1 ? "strong=false" : "strong=true"));
            auto result=response();
            if(calls==2) result.data["code"]="8lzjqnq8lye02n52jq3fqxf8e";
            return Reply{201,result.data.dump(),""};
        }
        TEST_REQUIRE(q.url=="https://plex.tv/api/v2/pins/12345?code=AB9Z");
        return Reply{200,response().data.dump(),""};
    },"device");
    TEST_REQUIRE(client.create_pin()); auto strong=client.create_pin(true); TEST_REQUIRE(strong);
    TEST_REQUIRE(pin(strong.data).code.size()>4); TEST_REQUIRE(client.poll_pin(12345,"AB9Z"));
    TEST_REQUIRE(!client.poll_pin(0,""));
    auto url=authorization_url("id &é","a/b?+");
    TEST_REQUIRE(url=="https://app.plex.tv/auth#?clientID=id%20%26%C3%A9&code=a%2Fb%3F%2B&context%5Bdevice%5D%5Bproduct%5D=Plexium");
    TEST_REQUIRE(authorization_url("","ABCD").empty());
    AuthSession s; s.begin(10); s.created(response(),12);
    TEST_REQUIRE(s.state==AuthState::Waiting && s.short_code() && s.deadline==310);
    TEST_REQUIRE(!s.poll_due(13) && s.poll_due(14));
    s.polled(response(296),14); TEST_REQUIRE(s.state==AuthState::Waiting && !s.poll_due(15));
    TEST_REQUIRE(s.poll_due(16));
    s.polled(response(294,"fixture-account-token"),16);
    TEST_REQUIRE(s.state==AuthState::Authorized && s.current.token=="fixture-account-token");
    s.begin(0); s.created(response(),0); s.polled(response(0),2); TEST_REQUIRE(s.state==AuthState::Expired);
    s.begin(0); s.created(response(1),2); TEST_REQUIRE(s.state==AuthState::Expired); // network time counts
    s.begin(0); s.created(response(),0); s.tick(301); TEST_REQUIRE(s.state==AuthState::Expired);
    s.polled(response(300,"late-token"),302); TEST_REQUIRE(s.current.token.empty());
    s.begin(0); s.created(response(),0); auto generation=s.generation; s.cancel();
    TEST_REQUIRE(s.generation!=generation); s.polled(response(300,"cancelled-token"),2);
    TEST_REQUIRE(s.state==AuthState::Cancelled && s.current.token.empty());
    s.begin(0); s.created(response(),0); s.polled({{},"Server returned HTTP 404",404},2);
    TEST_REQUIRE(s.state==AuthState::Expired);
    s.begin(0); s.created(response(),0); s.polled({{},"Network or TLS request failed",0},2);
    TEST_REQUIRE(s.state==AuthState::Failed && !s.poll_due(3));
    for(auto bad : {Json{}, Json{{"id",0},{"code","ABCD"},{"expiresIn",300}},
        Json{{"id",1},{"code",""},{"expiresIn",300}},
        Json{{"id",1},{"code","ABCD"},{"expiresIn",-1}}}) {
        s.begin(0); s.created({bad,"",200},0); TEST_REQUIRE(s.state==AuthState::Failed);
    }
    for (auto field : {"id","code","authToken"}) {
        s.begin(0); s.created(response(),0); auto bad=response(); bad.data.erase(field);
        s.polled(bad,2); TEST_REQUIRE(s.state==AuthState::Failed);
    }
    s.begin(0); s.created(response(),0); auto mismatch=response(); mismatch.data["id"]=2;
    s.polled(mismatch,2); TEST_REQUIRE(s.state==AuthState::Failed);
    s.begin(0); s.created(response(),0); auto huge=response(99999);
    s.polled(huge,2); TEST_REQUIRE(s.deadline==300); // polling cannot extend server expiry
    s.begin(0); s.created(strong,0); TEST_REQUIRE(!s.short_code());
    TEST_REQUIRE(s.current.code=="8lzjqnq8lye02n52jq3fqxf8e"); // never truncate
    Client wrong([](const Request &){return Reply{201,R"({"clientIdentifier":"other"})",""};},"device");
    TEST_REQUIRE(!wrong.create_pin());
    Client validation([](const Request &q){
        TEST_REQUIRE(q.url=="https://plex.tv/api/v2/user");
        TEST_REQUIRE(q.headers.at("X-Plex-Token")=="fixture-account-token");
        return Reply{200,R"({"id":42,"username":"fixture"})",""};
    },"device");
    TEST_REQUIRE(validation.validate_account("fixture-account-token"));
    Client invalid([](const Request &){return Reply{401,"do not reveal secret",""};},"device");
    TEST_REQUIRE(!invalid.validate_account("fixture-account-token"));
    // Persist, restore, sign out; same identity, private mode, no token in URLs.
    char dir[]="/tmp/plexium-auth-XXXXXX"; TEST_REQUIRE(mkdtemp(dir));
    std::string file=std::string(dir)+"/account.json", id, token;
    TEST_REQUIRE(save_credentials(file,"device","fixture-account-token"));
    TEST_REQUIRE(load_credentials(file,id,token) && id=="device" && token=="fixture-account-token");
    struct stat st{}; TEST_REQUIRE(stat(file.c_str(),&st)==0 && (st.st_mode&0777)==0600);
    TEST_REQUIRE(save_credentials(file,id,"")); TEST_REQUIRE(load_credentials(file,id,token) && token.empty());
    test_posix_result(unlink(file.c_str()),"unlink"); test_posix_result(rmdir(dir),"rmdir");
    // Generate the exact session URL locally; fixture contains no real PIN/token.
    url=authorization_url("fixture-client-identifier","AB9Z");
    uint8_t qr[qrcodegen_BUFFER_LEN_MAX], scratch[qrcodegen_BUFFER_LEN_MAX];
    TEST_REQUIRE(qrcodegen_encodeText(url.c_str(),scratch,qr,qrcodegen_Ecc_MEDIUM,1,15,qrcodegen_Mask_AUTO,true));
    int n=qrcodegen_getSize(qr), cell=8, size=(n+8)*cell;
    std::ofstream image("build/auth-qr.pgm"); image<<"P2\n"<<size<<" "<<size<<"\n255\n";
    for(int y=0;y<size;++y) for(int x=0;x<size;++x) {
        int col=x/cell-4,row=y/cell-4;
        image<<(col>=0 && row>=0 && col<n && row<n && qrcodegen_getModule(qr,col,row) ? 0:255)<<' ';
    }
    std::ofstream("build/auth-qr-url.txt")<<url;
    puts("Authentication scenarios passed: short/strong PIN, URLs/encoding, polling, expiry, cancellation, failure, persistence/restore/sign-out, QR generation");
}
