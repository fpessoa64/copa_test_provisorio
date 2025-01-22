// ImageServer.h
#ifndef IMAGESERVER_H
#define IMAGESERVER_H

#include "httplib.h"
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <string>
#include <map>
#include <mutex>

using json = nlohmann::json;

namespace copa
{

    // class InspectionManager
    // {
    // public:
    //     static InspectionManager &getInstance();

    //     void addInspection(const std::string &inspectionId, const json &data);
    //     void updateInspection(const std::string &inspectionId, const std::string &key, const json &value);
    //     json getInspection(const std::string &inspectionId);

    // private:
    //     InspectionManager() = default;
    //     std::mutex mutex_;
    //     std::map<std::string, json> inspections_;
    // };

    class ImageServer
    {
    public:
        ImageServer(const std::string& host, int port) : host_(host), port_(port) {}
        void setupRoutes();
        void run();
        void start();

    private:
        std::string generateInspectionId();

        std::string host_;
        int port_;
        httplib::Server server_;
        std::thread serverThread_;
    };

}
#endif // IMAGESERVER_H
