#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class MainCutterDetection {
private:
    std::string type;
    json component_data;
    json centroid;
    std::string ocr_component_key;
    std::vector<std::vector<json>> ocr_detections_list;

    double getIoU(const json& bbox1, const json& bbox2) {
        double x_min = std::max(bbox1["x_min"].get<double>(), bbox2["x_min"].get<double>());
        double y_min = std::max(bbox1["y_min"].get<double>(), bbox2["y_min"].get<double>());
        double x_max = std::min(bbox1["x_max"].get<double>(), bbox2["x_max"].get<double>());
        double y_max = std::min(bbox1["y_max"].get<double>(), bbox2["y_max"].get<double>());

        double intersection = std::max(0.0, x_max - x_min) * std::max(0.0, y_max - y_min);
        double area1 = (bbox1["x_max"].get<double>() - bbox1["x_min"].get<double>()) * (bbox1["y_max"].get<double>() - bbox1["y_min"].get<double>());
        double area2 = (bbox2["x_max"].get<double>() - bbox2["x_min"].get<double>()) * (bbox2["y_max"].get<double>() - bbox2["y_min"].get<double>());
        double union_area = area1 + area2 - intersection;

        return intersection / union_area;
    }

    // Add the function get_distance here
    std::pair<double,double> get_center(json &bbox) {

        double center_x = (bbox.value("x_min",0.0) + bbox.value("x_max",0.0)) / 2;
        double center_y = (bbox.value("y_min",0.0) + bbox.value("y_max",0.0)) / 2;
        return std::make_pair(center_x,center_y);
    }

    /**
     def remove_overlapping_detections(detections):

    if len(detections) < 2:
        return detections, []

    detections.sort(key=lambda x: x["confidence"], reverse=True)

    filtered_detections = []
    removed_detections = []
    for detection in detections:
        for filtered_detection in filtered_detections:
            if get_iou(detection["bbox"], filtered_detection["bbox"]) > 0.8:
                removed_detections.append(detection)
                break
        else:
            filtered_detections.append(detection)


    return filtered_detections, removed_detections
 
     */
    std::pair<json,json> remove_overlapping_detections(json & detections) {
        if (detections.size() < 2) {
            return std::make_pair(detections,json::array());
        }
        // detections.sort(key=lambda x: x["confidence"], reverse=True)
        std::sort(detections.begin(), detections.end(), [](const json& a, const json& b) {
            return a["confidence"].get<double>() > b["confidence"].get<double>();
        });

        json filtered_detections;
        json removed_detections;
        for (const auto& detection : detections) {
            bool is_overlapping = false;
            for (const auto& filtered_detection : filtered_detections) {
                if (getIoU(detection["bbox"], filtered_detection["bbox"]) > 0.8) {
                    removed_detections.push_back(detection);
                    is_overlapping = true;
                    break;
                }
            }
            if (!is_overlapping) {
                filtered_detections.push_back(detection);
            }
        }

        return std::make_pair(filtered_detections, removed_detections);
    }

public:
    MainCutterDetection(const json& component_data, const std::string& ocr_component_key)
        : component_data(component_data),
          ocr_component_key(ocr_component_key) {
        type = component_data["type"];
        centroid = component_data["centroid"];
        add_ocr_detection(component_data);
    }

    std::string get_type() const {
        return type;
    }

    json get_detection() const {
        return {
            {"class", component_data["class"]},
            {"label", component_data["label"]},
            {"bbox_normalized", component_data["bbox_normalized"]},
            {"bbox", component_data["bbox"]},
            {"color", component_data["color"]},
            {"confidence", component_data["confidence"]}
        };
    }

    json get_centroid() const {
        return centroid;
    }

    std::vector<std::vector<json>> get_ocr_detections() const {
        return ocr_detections_list;
    }

    void add_ocr_detection(const json& component_data) {
        if (component_data.contains(ocr_component_key)) {
            std::vector<json> ocr_detections;
            for (const auto& [character, detections] : component_data[ocr_component_key]["outputs"].items()) {
                ocr_detections.insert(ocr_detections.end(), detections.begin(), detections.end());
            }
            std::sort(ocr_detections.begin(), ocr_detections.end(), [](const json& a, const json& b) {
                return a["bbox"]["x_min"] < b["bbox"]["x_min"];
            });
            ocr_detections_list.push_back(ocr_detections);
        } else {
            for (const auto& [key, value] : component_data.items()) {
                if (key.rfind("component_", 0) == 0) { // Checks if key starts with "component_"
                    for (const auto& [class_id, detections] : value["outputs"].items()) {
                        for (const auto& detection : detections) {
                            add_ocr_detection(detection);
                        }
                    }
                }
            }
        }
    }

    void append_characters(std::map<std::string, std::vector<json>>& character_position, double avg_width_multiplier = 0.95) {
        if (type == "tare") {
            for (const auto& ocr_detections : ocr_detections_list) {
                if (ocr_detections.empty() || ocr_detections.size() > 4) {
                    continue;
                }

                double detection_width_sum = 0;
                for (const auto& detection : ocr_detections) {
                    detection_width_sum += detection["bbox"]["x_max"].get<double>() - detection["bbox"]["x_min"].get<double>();
                }
                double avg_width = detection_width_sum / ocr_detections.size();

                std::vector<json> character_data_list;
                for (size_t i = 0; i < ocr_detections.size(); ++i) {
                    character_data_list.push_back(ocr_detections[i]);
                    if (i != ocr_detections.size() - 1) {
                        double gap = ocr_detections[i + 1]["bbox"]["x_min"].get<double>() - ocr_detections[i]["bbox"]["x_max"].get<double>();
                        if (gap > avg_width * avg_width_multiplier) {
                            character_data_list.push_back({{"class", "X"}, {"confidence", 0}});
                        }
                    }
                }

                if (character_data_list.size() == 3 && ocr_detections.size() == 3) {
                    if (character_data_list[0]["class"] == "1") {
                        character_data_list.push_back({{"class", "X"}, {"confidence", 0}});
                    } else {
                        character_data_list.insert(character_data_list.begin(), {{"class", "X"}, {"confidence", 0}});
                    }
                }

                if (character_data_list.size() == 4) {
                    for (size_t index = 0; index < character_data_list.size(); ++index) {
                        character_position[std::to_string(index + 1)].push_back(character_data_list[index]);
                    }
                }
            }
        }
        // Additional logic for "expiration_date" type can be implemented similarly.
    }
};
