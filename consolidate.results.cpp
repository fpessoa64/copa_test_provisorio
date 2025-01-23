#include "consolidate.results.h"

namespace copa {

    void ConsolidateResult::prepare_parms()
    {
        LOG(INFO) << "prepare_parms called << " << m_flow_data.dump();
        m_node_components = get_node_components_data(m_flow_data, m_main_cutter_name, m_cylinder_tare_ocr_name, m_cylinder_expiration_date_ocr_name, m_tare_class_label, m_expiration_date_class_label, m_color_class_label);

    }
    //--------------------------------------------------------------------------------

    json ConsolidateResult::get_node_components_data(json flow_data ,
            std::string main_cutter_name,std::string cylinder_tare_ocr_name,
            std::string cylinder_expiration_date_ocr_name,
            std::string tare_class_label,
            std::string expiration_date_class_label,
            std::string color_class_label) 
    {
        json node_components = json::object();
        for (auto &comp : flow_data["nodes"])
        {
            if (comp.value("name","") == cylinder_tare_ocr_name)
            {
                node_components["ocr_tare_component"] = "component_" + comp.value("_id","");
            }
            if (comp.value("name","") == color_class_label)
            {
                node_components["color_class_component"] = "component_" + comp.value("_id","");
            }
            else if (comp.value("name","") == cylinder_expiration_date_ocr_name)
            {
                node_components["ocr_expiration_date_component"] = "component_" + comp.value("_id","");
            }
            else if (comp.value("name","") == main_cutter_name)
            {
                node_components["main_cutter"] = "component_" + comp.value("_id","");
                for (auto &[key, val] : comp["outputs"].items())
                {
                    if (val["label"] == tare_class_label)
                    {
                        node_components["tare_class_id"] = key;
                    }
                    else if (val["label"] == expiration_date_class_label)
                    {
                        node_components["expiration_date_class_id"] = key;
                    }
                    else
                    {
                        node_components[val.value("label","")] = key;
                    }
                }
            }
        }

        if(node_components.contains("ocr_tare_component") == false)
        {
            throw new std::runtime_error("did not find ocr_tare_component in flow");
        }
        if(node_components.contains("ocr_expiration_date_component") == false)
        {
            throw new std::runtime_error("did not find ocr_expiration_date_component in flow");
        }
        if(node_components.contains("main_cutter") == false)
        {
            throw new std::runtime_error("did not find main_cutter in flow");
        }
        if(node_components.contains("tare_class_id") == false)
        {
            throw new std::runtime_error("did not find tare_class_id in flow");
        }

        LOG(INFO) << "node_components: " << node_components.dump();
        return node_components;
    }
    //--------------------------------------------------------------------------------

    json ConsolidateResult::run(json &results)
    {
        LOG(INFO) << "run called << " << results.dump();

        json classifier_detections = {
            {"color", json::object()},
            {"exists", false}
        };
        json main_cutter_detections = {
            {"tare", json::array()},
            {"expiration_date",json::array()}
        };
        json brand_detections = {
            {"exists", false},
            {"brand", json::object()}
        };
        bool tare_plate_dectected = false;

        for(auto &[key, result] : results.items())
        {
            LOG(INFO) << "processing result: key: " << key << " val: " << result.dump();
            /// for component_key in [key for key in result.keys() if key.startswith("component_")]:
            ///    log.info(f"Processing component_key: {component_key}")
            for(auto &[k, v] : result.items())
            {
                if(k.find("component_") != std::string::npos)
                {
                    LOG(INFO) << "Processing component_key: " << k << " main_cutter: " << m_node_components["main_cutter"];
                    if(k != m_node_components["main_cutter"])
                    {
                        if(k == m_node_components["color_class_component"])
                        {
                            classifier_detections["color"] = v["outputs"];
                            classifier_detections["exists"] = true;
                        }
                        continue;
                    }
                    json component = v;
                    LOG(INFO) << "component: " << component.dump();
                    // valid_class_id_list = [class_id for class_id in component["outputs"].keys() if class_id in [self.node_components["tare_class_id"], self.node_components["expiration_date_class_id"]]]
                    // log.info(f'valid_class_list: {valid_class_id_list}')
                 
                    json valid_class_id_list = json::array();
                    for(auto &[key, val] : component["outputs"].items())
                    {
                        if(key == m_node_components["tare_class_id"] || key == m_node_components["expiration_date_class_id"])
                        {
                            valid_class_id_list.push_back(key);
                        }
                    }
                    LOG(INFO) << "valid_class_list: " << valid_class_id_list.dump();

                }
            }
        }

        return json::object();
    }
    //--------------------------------------------------------------------------------
}