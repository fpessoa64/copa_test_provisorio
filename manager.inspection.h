
#ifndef COPA_INSPECTION_MANAGER_H
#define COPA_INSPECTION_MANAGER_H

#include "common.h"
#include "inspection.h"
#include <mutex>

using json = nlohmann::json;

namespace copa
{

    class InspectionManager
    {
    public:
        static InspectionManager &get_instance();
        
        std::string get_name() { return m_name; };
        void set_parms(const json &parms);

        void add_data_event(const std::string &inspectionId,const std::string &key, json &data);
        void update_inspection(const std::string &inspectionId, const std::string &key, const json &value);
        void create_inspection(const std::string inspectionId,cv::Mat &img);
        void force_inspection(const std::string inspectionId,std::string image_id,std::string image_abs_id,std::string image_border_id);
        void consolidate_inspection(const std::string &inspectionId);
        Inspection *get_inspection(const std::string &inspectionId);


    private:
        InspectionManager();
        std::mutex m_lock_inspection;
        std::string m_name;
        json m_parms;
        std::map<std::string, Inspection *> m_inspections;
       
    };

}
#endif // COPA_INSPECTION_MANAGER_H