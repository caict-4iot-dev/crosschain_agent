#include <iostream>
#include <string>
#include "database/keyvalue_db.h"
#include "database/rocks_db.h"
#include "common/private_key.h"
#include "proto/cpp/common.pb.h"
#include "common/general.h"
#include <vector>
#include <memory>
#include "utils/thread.h"


int main() {
    std::string db_path = "/usr/local/agent/data/ledger.db";
    

    std::cout << "input db path:" << std::endl;
    std::cin >> db_path;
    std::cout << "db_path:" << db_path << std::endl;

    agent::AddressPrefix::InitInstance();
    agent::AddressPrefix::Instance().Initialize();

   

    std::cout << "1:db read" << std::endl;
    std::cout << "2:get layer2 agent status" << std::endl;
    char ch;
	std::cin >> ch;
    if (ch == '1'){
        auto db = std::make_shared<agent::RocksDbDriver>();
        if (db == nullptr) {
            std::cerr << "db nullptr " << std::endl;
            return -1;
        }

        if (!db->OpenReadOnly(db_path)) {
            std::cerr << "db open read only error" << std::endl;
            return -1;
        }
        std::string dbkey = "";
        std::cout << "please input dbkey:" << std::endl;
        std::cin >> dbkey;
        std::cout << "dbkey:" << dbkey << std::endl;
        std::string value = "";
        auto ret = db->Get(dbkey, value);
        std::cout <<"ret :" << ret<< "value:" << value << std::endl;
        db->Close();
    }else{}
   

    return 0;
}