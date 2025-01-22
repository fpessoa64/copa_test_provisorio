#include <iostream>
#include "manager.inspection.h"

namespace copa {

    InspectionManager::InspectionManager() {
        // Initialize the InspectionManager
        m_name = std::to_string(std::time(nullptr));
    }
    
    InspectionManager &InspectionManager::get_instance() {
        static InspectionManager instance;
        return instance;
    }

    void InspectionManager::set_parms(const json  &parms) {
        this->m_parms = parms;
    }

    void InspectionManager::create_inspection(const std::string inspectionId,cv::Mat &img) {
        std::lock_guard<std::mutex> lock(m_lock_inspection);
        m_inspections[inspectionId] = new Inspection(inspectionId,m_parms);
        m_inspections[inspectionId]->run_inspection(img);

    }

    void InspectionManager::force_inspection(const std::string inspectionId,std::string image_id,std::string image_abs_id,std::string image_border_id) {
        std::lock_guard<std::mutex> lock(m_lock_inspection);
        m_inspections[inspectionId] = new Inspection(inspectionId,m_parms);
        m_inspections[inspectionId]->force_inspection(image_id,image_abs_id,image_border_id);

        std::cout << "InspectionManager  force_inspection: " << inspectionId << std::endl;
    }

    void InspectionManager::add_data_event(const std::string &inspectionId,const std::string &key, json &data) {
        std::lock_guard<std::mutex> lock(m_lock_inspection);
        auto &inspection = m_inspections[inspectionId];
        if (inspection) {
            inspection->add_event(key,data);
        }
        else {
            std::cerr << "Inspection not found: " << inspectionId << std::endl;
        }
    }

    void InspectionManager::consolidate_inspection(const std::string &inspectionId) {
        std::lock_guard<std::mutex> lock(m_lock_inspection);
        auto &inspection = m_inspections[inspectionId];
        if (inspection) {
            std::cout << "Inspection: " << inspectionId << std::endl;
            json consolidate = inspection->consolidate();
            std::cout << "LOG Consolidate: " << consolidate.dump() << std::endl;
        }
        else {
            std::cerr << "Inspection not found: " << inspectionId << std::endl;
        }
    }

    void InspectionManager::update_inspection(const std::string &inspectionId, const std::string &key, const json &value) {
        std::lock_guard<std::mutex> lock(m_lock_inspection);
       
    }

    Inspection * InspectionManager::get_inspection(const std::string &inspectionId) {
        std::lock_guard<std::mutex> lock(m_lock_inspection);
        return m_inspections[inspectionId];
    }

   
}