#include <iostream>
#include <nlohmann/json.hpp>
#include "glog/logging.h"
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "httplib.h"
#include <thread>
#include "image.server.h"
#include "manager.inspection.h"

namespace copa
{
    void ImageServer::setupRoutes()
    {
        server_.Get("/status", [](const httplib::Request &, httplib::Response &res)
        {
        json response = { {"message", "Server is running"} };
        res.set_content(response.dump(), "application/json"); });

        server_.Post("/process_image", [&](const httplib::Request &req, httplib::Response &res)
        {
        // Decode image
        std::vector<unsigned char> imgData(req.body.begin(), req.body.end());
        cv::Mat img = cv::imdecode(imgData, cv::IMREAD_COLOR);

        if (img.empty()) {
            json error = { {"error", "Invalid image format"} };
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        std::string inspectionId = generateInspectionId();

        // Save image
        std::string imagePath = "/workspaces/c++/temp/" + inspectionId + ".jpg";
        cv::imwrite(imagePath, img);

        copa::InspectionManager &inspectionManager = copa::InspectionManager::get_instance();
        inspectionManager.create_inspection(inspectionId,img);

        json response = { {"message", "Image processed successfully"}, {"inspection_id", inspectionId} };
        res.set_content(response.dump(), "application/json"); });
    }


    void ImageServer::run()
    {
        std::cout << "Server starting on " << host_ << ":" << port_ << std::endl;
        server_.listen(host_.c_str(), port_);
    }

    void ImageServer::start()
    {
        copa::InspectionManager &inspectionManager = copa::InspectionManager::get_instance();
        std::cout <<  "InspectionManager name: " << inspectionManager.get_name() << std::endl;

        serverThread_ = std::thread(&ImageServer::run, this);
        serverThread_.detach();
    }

    std::string ImageServer::generateInspectionId()
    {
        return std::to_string(std::time(nullptr));
    }
}
