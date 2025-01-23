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
#include "consolidate.results.h"

namespace copa
{

    Inspection::Inspection(const std::string &inspection_id, const json parms) : m_inspection_id(inspection_id), m_parms(parms) {}

    void Inspection::add_event(const std::string &key, json &value)
    {
        try {
            std::lock_guard<std::mutex> lock(m_lock_consolidate);

            data_images[key]->fixAnnotations(value);
            data_images[key]->add_event(value);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Erro ao add a event" << key << ": " << e.what() << std::endl;
        }
         m_condition.notify_all();
    }
    //--------------------------------------------------------------------------------

    void Inspection::update_schema(json &jsonData, const std::string &key, const json &newValue)
    {
        if (jsonData.is_object())
        {
            for (auto &[k, v] : jsonData.items())
            {
                if (k == key)
                {
                    v = newValue;
                    return;
                }
                update_schema(v, key, newValue);
            }
        }
        else if (jsonData.is_array())
        {
            for (auto &item : jsonData)
            {
                update_schema(item, key, newValue);
            }
        }
    }
    //--------------------------------------------------------------------------------

    void Inspection::update_brand(std::string name, json &jsonData, const std::string &key, const json &newValue)
    {
        if (jsonData.is_object())
        {
            for (auto &[k, v] : jsonData.items())
            {
                if (k == key)
                {
                    v.push_back(newValue);
                    return;
                   
                }
                update_brand(name,v, key, newValue);
            }
        }
        else if (jsonData.is_array())
        {
            for (auto &item : jsonData)
            { 
                update_brand(name,item, key, newValue);
            }
        }
    }
    //--------------------------------------------------------------------------------
    json Inspection::consolidate() 
    {
        return prepare();
    }
    //--------------------------------------------------------------------------------

    json Inspection::prepare()
    {
        json consolidate;
        json consolidated_data;
        int index = 0;

        std::ifstream file("/workspaces/c++/conf/schema.json");
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open parms.json");
        }
        json template_schema_json = json::parse(file);
        file.close();

        // for (auto &image : data_images)
        // {
        //     json c = image.second->consolidate();
        //     if (c != nullptr)
        //         consolidated_data[std::to_string(index++)] = c;
        // }
        //std::cout << "LOG - consolidated_data: " << consolidated_data.dump() << std::endl;
        for (auto &image : data_images) {
            json consolidated_data = image.second->consolidate();
            if(consolidated_data == nullptr || consolidated_data.empty()) {
                continue;
            }            
            json schema_json = template_schema_json;
            for(auto &v1 : consolidated_data) {
                std::cout << "LOG - v1: " << v1.dump() << std::endl;
                for (auto &[k2,v2]: v1["output_data"].items()) {
                    //std::cout << "LOG - "<< " k2: " << k2 <<  " v2: " << v2.dump() << std::endl;
                    std::string name = v2.value("name","");
                    if(name == "TARA" || name == "OCR VENCIMENTO") {
                        //std::cout << "LOG - k2: " << k2  << " update tara: " << v2.dump() << std::endl;
                        std::string key = "component_" + v2.value("component_id","");
                        update_schema(schema_json, key, v2);
                    }
                    // else if(name == "OCR VENCIMENTO") {
                    //     //std::cout << "LOG - k2: " << k2  << " update ferradura: " << v2.dump() << std::endl;
                    //     std::string key = "component_" + v2.value("component_id","");
                    //     update_schema(schema_json, key, v2);
                    // }
                    else if(name == "CUTTER") {
                        //std::cout << "LOG - k2: " << k2  << " update cutter: " << v2.dump() << std::endl;
                        
                        for (auto &output : v2["outputs"])
                        {
                            //std::cout << "LOG - "<< " output: " << output.dump() << std::endl;
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
            consolidate[std::to_string(index++)] = schema_json;
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
    //--------------------------------------------------------------------------------
    
    void Inspection::force_inspection(std::string image_id, std::string image_abs_id, std::string image_border_id)
    {
        std::cout << "m_parms: " << m_parms.dump() << std::endl;
        // Create Image object
        data_images[image_id] = new ImageAddBorder(cv::Mat());
        data_images[image_abs_id] = new ImageScaleAbs(cv::Mat());
        data_images[image_border_id] = new ImageOriginal(cv::Mat());

        start_consolidation_thread(3);

        std::cout << "force_inspection: " << image_id << " image_abs_id: " << image_abs_id << " image_border_id: " << image_border_id << std::endl;
    }
    //--------------------------------------------------------------------------------

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
    //--------------------------------------------------------------------------------

    void Inspection::start_consolidation_thread(int consolidation_interval) {
        m_stop_thread = false;

        m_consolidation_thread = std::thread([this, consolidation_interval]() {
            std::unique_lock<std::mutex> lock(m_lock_consolidate);
            while (!m_stop_thread) {
                // Aguarda até o tempo especificado ou até que add_event notifique
                if (m_condition.wait_for(lock, std::chrono::seconds(consolidation_interval)) == std::cv_status::timeout) {
                    // Tempo expirado, realiza a consolidação
                    json result = consolidate();
                    std::cout << "Consolidation completed: " << result.dump() << std::endl;
                    // load flow json from conf folder
                    std::ifstream file("/workspaces/c++/conf/671953f155784c001a41d0dc.json");
                    if (!file.is_open())
                    {
                        throw std::runtime_error("Could not open flow.json");
                    }
                    json flow_data = json::parse(file);
                    file.close();

                    ConsolidateResult *consolidate_result = new ConsolidateResult(flow_data);
                    consolidate_result->prepare_parms();
                    consolidate_result->run(result);
                    delete consolidate_result;
                    break;
                    

                }
            }
        });
        //stop_consolidation_thread();
    }
    //--------------------------------------------------------------------------------

    void Inspection::stop_consolidation_thread() {
        {
            std::lock_guard<std::mutex> lock(m_lock_consolidate);
            m_stop_thread = true;
        }
        m_condition.notify_all();
        if (m_consolidation_thread.joinable()) {
            m_consolidation_thread.join();
        }
    }
    //--------------------------------------------------------------------------------
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