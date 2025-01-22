#include <iostream>
#include <nlohmann/json.hpp>
#include "glog/logging.h"
#include <filesystem>
#include "image.server.h"
#include "manager.inspection.h"

void add_event(std::string path,std::string inspection_id,std::string image_id,copa::InspectionManager &inspectionManager) {
     for (const auto & entry : std::filesystem::directory_iterator(path))
    {
        std::cout << entry.path() << std::endl;
        /// abrir arquivo json
        std::ifstream i(entry.path());
        json j = json::parse(i);
        std::cout << j.dump(4) << std::endl;

        inspectionManager.add_data_event(inspection_id,image_id,j);
    }
}

int main(int argc, char* argv[])
{
    // Create log directory if it doesn't exist
   
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = "/workspaces/c++/build/log";
    FLAGS_logtostderr = false;
 
    using json = nlohmann::json;
    std::cout << "Hello, World!" << std::endl;

    ///load json from  folder conf file parms.json
    std::ifstream i("/workspaces/c++/conf/parms.json");
    json parms = json::parse(i);
    std::cout << parms.dump(4) << std::endl;

    copa::InspectionManager &inspectionManager = copa::InspectionManager::get_instance();
    inspectionManager.set_parms(parms);
    std::cout <<  "InspectionManager name: " << inspectionManager.get_name() << std::endl;
    sleep(2);
    copa::ImageServer server("0.0.0.0", 8002);
    server.setupRoutes();
    server.start();

    //sleep(60);

    std::string frame_id_orig =  "9dadac7c-7884-496c-b59f-00006b5c12e132d617f3";
    std::string frame_id_abs =  "6a994b0b-2ad6-42fb-bee1-0000ea4f2f59d05e798d";
    std::string frame_id_border =  "5de62f61-973a-473d-9cae-000048505f7c90edc62b";
    std::string inspection_id = "1737405909";

    inspectionManager.force_inspection(inspection_id,frame_id_orig,frame_id_abs,frame_id_border);

    /// List dir de files em json ./events
    std::string path = "/workspaces/c++/events/image_original";
    add_event(path,inspection_id,frame_id_orig,inspectionManager);

    path = "/workspaces/c++/events/image_abs";
    add_event(path,inspection_id,frame_id_abs,inspectionManager);

    path = "/workspaces/c++/events/image_border";
    add_event(path,inspection_id,frame_id_border,inspectionManager);

    inspectionManager.consolidate_inspection(inspection_id);

    std::cout << "Server is running in a separate thread. Press Enter to exit..." << std::endl;
    std::cin.get();
    return 0;

}