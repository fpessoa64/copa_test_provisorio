#include "consolidate.results.h"

namespace copa {

    void ConsolidateResult::prepare_parms()
    {
        LOG(INFO) << "prepare_parms called << " << m_flow_data.dump();
        json node_components = get_node_components_data(m_flow_data, m_main_cutter_name, m_cylinder_tare_ocr_name, m_cylinder_expiration_date_ocr_name, m_tare_class_label, m_expiration_date_class_label, m_color_class_label);

    }
    //--------------------------------------------------------------------------------

    json ConsolidateResult::get_node_components_data(json flow_data ,
            std::string main_cutter_name,std::string cylinder_tare_ocr_name,
            std::string cylinder_expiration_date_ocr_name,
            std::string tare_class_label,
            std::string expiration_date_class_label,
            std::string color_class_label) 
    {
        /**
         * node_components = {}
    for comp in flow_data["nodes"]:
        log.info(f'comp: {comp}')
        if comp.get("name") == cylinder_tare_ocr_name:
            node_components["ocr_tare_component"] = "component_" + comp["_id"]
        if comp.get("name") == color_class_label:
            node_components["color_class_component"] = "component_" + comp["_id"]
        elif comp.get("name") == cylinder_expiration_date_ocr_name:
            node_components["ocr_expiration_date_component"] = "component_" + comp["_id"]
        elif comp.get("name") == main_cutter_name:
            node_components["main_cutter"] = "component_" + comp["_id"]
            for key, val in comp["outputs"].items():
                log.info(f'key: {key} val: {val}')
                if val["label"] == tare_class_label:
                    node_components["tare_class_id"] = key
                elif val["label"] == expiration_date_class_label:
                    node_components["expiration_date_class_id"] = key
                else:
                    log.info(f'check key: {key} val: {val}')
                    node_components[val['label']] = key
                
         */

        json node_components = json::object();
        for (auto &comp : flow_data["nodes"])
        {
            LOG(INFO) << "comp: " << comp.dump();
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
                    LOG(INFO) << "key: " << key << " val: " << val.dump();
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
                        LOG(INFO) << "check key: " << key << " val: " << val.dump();
                        node_components[val.value("label","")] = key;
                    }
                }
            }
        }
        LOG(INFO) << "node_components: " << node_components.dump();
        return node_components;
    }
    
    

}