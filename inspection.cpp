#include "inspection.h"
#include <mutex>
#include <opencv2/opencv.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>
#include <thread>
#include "httplib.h"

namespace copa
{

    Inspection::Inspection(const std::string &inspection_id, const json parms) : m_inspection_id(inspection_id), m_parms(parms) {}

    void Inspection::add_event(const std::string &key, json &value)
    {
        try {
            std::cerr << "Erro ao add a event" << key << ": " << value.dump() << std::endl;
            data_images[key]->fixAnnotations(value);
            data_images[key]->add_event(value);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Erro ao add a event" << key << ": " << e.what() << std::endl;
            exit(1);
        }
    }

    void Inspection::update_data(const std::string &key, const json &value)
    {
        // data_[key] = value;
    }

    void Inspection::update_ferradura(json &json_schema, const json &value)
    {
        json c = json_schema["component_632b6916044e9ebb72172e36"].value("outputs", json::object());
        std::cout << "component_632b6916044e9ebb72172e36: " << c.dump() << std::endl;
        json outputs = json_schema["component_632b6916044e9ebb72172e36"].is_object() ? json_schema["component_632b68ef3ae7b7cd39782b13"].value("outputs", json::object()) : json::object();

        for (auto &output : outputs.items())
        {
            // std::cout << "output: " << output.key() << " value: " << output.value() <<std::endl;
            for (auto &output_data : output.value())
            {
                // std::cout << "output_data dump: " << output_data.dump() <<std::endl;
                if (output_data["label"] == "FERRADURA")
                {
                    std::cout << "LOG " << "output_data: " << output_data.dump() << std::endl;
                    output_data["bbox"] = value.value("bbox", json::object());
                    output_data["bbox_normalized"] = value.value("bbox_normalized", json::object());
                    output_data["confidence"] = value.value("confidence", 0);
                    output_data["component_632b68ef3ae7b7cd39782b13"]["outputs"] = value.value("outputs", json::object());
                    std::cout << "LOG " << "output_data modified ferradura: " << output_data.dump() << std::endl;
                    // output_data["value"] = value;
                }
            }
        }
        json_schema["component_632b68ef3ae7b7cd39782b13"]["outputs"] = outputs;
    }

    void Inspection::updateJsonByKey(std::string name, json &jsonData, const std::string &key, const json &newValue)
    {
        std::cout << "LOG " << " updateJsonByKey key: " << key << " value: " << newValue.dump() << std::endl;
        if (jsonData.is_object())
        {
            for (auto &[k, v] : jsonData.items())
            {
                if (k == key)
                {
                    std::cout << "LOG Found " << "updateJsonByKey key: " << key << " value: " << v.dump() << std::endl;
                    v = newValue;
                    return;
                   
                }
                updateJsonByKey(name,v, key, newValue);
            }
        }
        else if (jsonData.is_array())
        {
            for (auto &item : jsonData)
            {
                updateJsonByKey(name,item, key, newValue);
            }
        }
    }
    //--------------------------------------------------------------------------------

    void Inspection::update_brand(std::string name, json &jsonData, const std::string &key, const json &newValue)
    {
        std::cout << "LOG " << " update_brand key: " << key << " value: " << newValue.dump() << std::endl;
        if (jsonData.is_object())
        {
            for (auto &[k, v] : jsonData.items())
            {
                std::cout << "LOG " << " update_brand key_0: " << k << " value: " << v.dump() << std::endl;
                if (k == key)
                {
                    
                    std::cout << "LOG Found " << "update_brand key key: " << key << " value: " << v.dump() << std::endl;
                    v.push_back(newValue);
                    //v = newValue;
                    return;
                   
                }
                update_brand(name,v, key, newValue);
            }
        }
        else if (jsonData.is_array())
        {
            for (auto &item : jsonData)
            {
                std::cout << "LOG " << " update_brand item: " << item.dump() << std::endl;  
                update_brand(name,item, key, newValue);
            }
        }
    }
    //--------------------------------------------------------------------------------
    
    void Inspection::update_brand(json &json_schema, const json &value)
    {
        json c = json_schema["component_632b68ef3ae7b7cd39782b13"].value("outputs", json::object());
        // std::cout << "component_632b68ef3ae7b7cd39782b13: " << c.dump() << std::endl;
        json outputs = json_schema["component_632b68ef3ae7b7cd39782b13"].is_object() ? json_schema["component_632b68ef3ae7b7cd39782b13"].value("outputs", json::object()) : json::object();
        const std::string class_id = value.value("class", "");
        if (class_id.empty())
        {
            std::cerr << "class_id is empty" << std::endl;
            return;
        }
        std::cout << "LOG class_id: " << class_id << " outputs:  " << outputs.dump() << std::endl;
        if (outputs.contains(class_id))
        {
            json output = outputs.value(class_id, json::array());
            output.push_back(value);
            outputs[class_id] = output;
            std::cout << "LOG output update class_id : " << output.dump() << std::endl;
            json_schema["component_632b68ef3ae7b7cd39782b13"]["outputs"] = outputs;
        }
        else
        {
            std::cerr << "LOG class_id not found: " << class_id << std::endl;
        }
    }
    //--------------------------------------------------------------------------------

    void Inspection::extract_classes(const json &node, std::unordered_set<std::string> &classes)
    {

        if (node.is_object())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                std::cout << "node: " << it.value() << std::endl;
                extract_classes(it.value(), classes);
            }
        }
        else if (node.is_array())
        {

            for (const auto &item : node)
            {
                std::cout << "node array: " << item.dump() << std::endl;
                extract_classes(item, classes);
            }
        }
        else if (node.is_string() && node.contains("class"))
        {
            std::cout << "class: " << node.get<std::string>() << " node: " << node.dump() << std::endl;
            classes.insert(node.get<std::string>());
        }
    }
    //--------------------------------------------------------------------------------

    json Inspection::consolidate()
    {
        /// load file json of folder conf parms.json
      
        json consolidate;
        json consolidated_data;
        int index = 0;
        for (auto &image : data_images)
        {
            json c = image.second->consolidate();
            if (c != nullptr)
                consolidated_data[std::to_string(index++)] = c;
        }
        std::cout << "LOG - consolidated_data: " << consolidated_data.dump() << std::endl;

        for (auto &[k, v] : consolidated_data.items()) {
            std::ifstream file("/workspaces/c++/conf/schema.json");
            if (!file.is_open())
            {
                throw std::runtime_error("Could not open parms.json");
            }
            json schema_json = json::parse(file);
            file.close();

            std::cout << "LOG - key: " << k << " result: " << v.dump() << std::endl;
            for(auto &v1 : v) {
                std::cout << "LOG - v1: " << v1.dump() << std::endl;
                for (auto &[k2,v2]: v1["output_data"].items()) {
                    std::cout << "LOG - "<< " k2: " << k2 <<  " v2: " << v2.dump() << std::endl;
                    std::string name = v2.value("name","");
                    if(name == "TARA") {
                        std::cout << "LOG - k2: " << k2  << " update tara: " << v2.dump() << std::endl;
                        std::string key = "component_" + v2.value("component_id","");
                        updateJsonByKey("TARA",schema_json, key, v2);
                        //update_tara(schema_json,v2);
                    }
                    else if(name == "OCR VENCIMENTO") {
                        std::cout << "LOG - k2: " << k2  << " update ferradura: " << v2.dump() << std::endl;
                        std::string key = "component_" + v2.value("component_id","");
                        updateJsonByKey("FERRADURA",schema_json, key, v2);
                    }
                    else if(name == "CUTTER") {
                        std::cout << "LOG - k2: " << k2  << " update cutter: " << v2.dump() << std::endl;
                        
                        for (auto &output : v2["outputs"])
                        {
                            std::cout << "LOG - "<< " output: " << output.dump() << std::endl;
                            std::string label = output.value("label","");
                            if(label != "TARA" && label != "FERRADURA" && label != "") {
                                std::string key = output.value("class","");
                                if(!key.empty()) {
                                    update_brand("CUTTER",schema_json, key, output);
                                }
                            }
                        }
                    }
                }
            }
            std::cout << "LOG - schema_json: " << schema_json.dump() << std::endl;
            consolidate[k] = schema_json;
        }
        return consolidate ;
    }
    //--------------------------------------------------------------------------------

    json Inspection::get_data()
    {
        return inspection_data;
    }

    void Inspection::run_inspection(cv::Mat &img)
    {
        std::cout << "m_parms: " << m_parms.dump() << std::endl;
        // Create Image object
        data_images[generate_id()] = new ImageAddBorder(img);
        data_images[generate_id()] = new ImageScaleAbs(img);
        data_images[generate_id()] = new ImageOriginal(img);

        // criate thread for send images to external api  com post enviar as tres images em data_images simultaneas
        std::thread t1(&Inspection::send_images, this);
        t1.join();
    }

    void Inspection::force_inspection(std::string image_id, std::string image_abs_id, std::string image_border_id)
    {
        std::cout << "m_parms: " << m_parms.dump() << std::endl;
        // Create Image object
        data_images[image_id] = new ImageAddBorder(cv::Mat());
        data_images[image_abs_id] = new ImageScaleAbs(cv::Mat());
        data_images[image_border_id] = new ImageOriginal(cv::Mat());

        std::cout << "force_inspection: " << image_id << " image_abs_id: " << image_abs_id << " image_border_id: " << image_border_id << std::endl;
    }

    /**
     * @brief Send images to external API
     */
    void Inspection::send_images()
    {
        std::string input_endpoint_url = m_parms["options"].value("input_endpoint_url", "");
        if (input_endpoint_url.empty())
        {
            throw std::runtime_error("input_endpoint_url is empty");
        }
        std::cout << "input_endpoint_url: " << input_endpoint_url << std::endl;

        for (auto &image : data_images)
        {
            ImageSender sender(m_inspection_id, input_endpoint_url);
            sender.sendImage(image.second->getImage(), image.first);
        }
    }

    /**
     * @brief Generate a random UUID
     */
    std::string Inspection::generate_id()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

        std::ostringstream oss;
        oss << std::hex << std::setfill('0')
            << std::setw(8) << dist(gen) << "-"
            << std::setw(4) << (dist(gen) & 0xFFFF) << "-"
            << std::setw(4) << ((dist(gen) & 0x0FFF) | 0x4000) << "-"
            << std::setw(4) << ((dist(gen) & 0x3FFF) | 0x8000) << "-"
            << std::setw(12) << dist(gen) << dist(gen);

        return oss.str();
    }

}