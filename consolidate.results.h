#include <iostream>
#include <nlohmann/json.hpp>
#include "glog/logging.h"

using json = nlohmann::json;

namespace copa
{
    class ConsolidateResult
    {
    public:
        ConsolidateResult(json flow_data,std::string main_cutter_name = "CUTTER", std::string cylinder_tare_ocr_name = "TARA", 
                std::string cylinder_expiration_date_ocr_name = "OCR VENCIMENTO", 
                std::string tare_class_label = "TARA", std::string expiration_date_class_label = "FERRADURA", std::string color_class_label = "COR BOTIJAO") : m_flow_data(flow_data),m_main_cutter_name(main_cutter_name), m_cylinder_tare_ocr_name(cylinder_tare_ocr_name), m_cylinder_expiration_date_ocr_name(cylinder_expiration_date_ocr_name), m_tare_class_label(tare_class_label), m_expiration_date_class_label(expiration_date_class_label), m_color_class_label(color_class_label)
        {
            LOG(INFO) << "ConsolidateResult constructor called";
        }

        ~ConsolidateResult()
        {
            LOG(INFO) << "ConsolidateResult destructor called";
        }

        void prepare_parms();

    private:
        json m_flow_data;
        std::string m_main_cutter_name;
        std::string m_cylinder_tare_ocr_name;
        std::string m_cylinder_expiration_date_ocr_name;
        std::string m_tare_class_label;
        std::string m_expiration_date_class_label;
        std::string m_color_class_label;

        json get_node_components_data(json flow_data ,
            std::string main_cutter_name,std::string cylinder_tare_ocr_name,
            std::string cylinder_expiration_date_ocr_name,
            std::string tare_class_label,
            std::string expiration_date_class_label,
            std::string color_class_label) ;


    };
} // namespace copa