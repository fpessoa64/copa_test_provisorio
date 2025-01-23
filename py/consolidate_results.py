import os
import json
import math
import datetime
import logging as log

#from eyeflow_sdk.log_obj import log
import numpy as np
import cv2

log.basicConfig(level=log.DEBUG,
   format="{asctime} - {levelname} - {message}",
     style="{",
    datefmt="%Y-%m-%d %H:%M",
 )

# =========================================================================================================================
def get_node_components_data(
        flow_data,
        main_cutter_name,
        cylinder_tare_ocr_name,
        cylinder_expiration_date_ocr_name,
        tare_class_label,
        expiration_date_class_label,
        color_class_label
    ):
    
    log.info(f'cylinder_tare_ocr_name: {cylinder_tare_ocr_name}')
    log.info(f'cylinder_expiration_date_ocr_name: {cylinder_expiration_date_ocr_name}')
    log.info(f'main_cutter_name: {main_cutter_name}')
    log.info(f'tare_class_label: {tare_class_label}')
    log.info(f'expiration_date_class_label: {expiration_date_class_label}')
    log.info(f'color_class_label: {color_class_label}')
    

    node_components = {}
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
                

    if "ocr_tare_component" not in node_components:
        raise Exception(f'Did not find ocr_tare_component in flow')

    if "ocr_expiration_date_component" not in node_components:
        raise Exception(f'Did not find ocr_expiration_date_component in flow')

    if "main_cutter" not in node_components:
        raise Exception(f'Did not find main_cutter in flow')

    if "tare_class_id" not in node_components:
        raise Exception(f'Did not find tare_class_id in flow')
    
    log.info(f'node_components: {node_components}')

    # if "color_class_component" not in node_components:
    #     raise Exception(f'Did not find color_class_component in flow')

    return node_components

# =========================================================================================================================
def get_iou(bbox1, bbox2):

    x_min = max(bbox1["x_min"], bbox2["x_min"])
    y_min = max(bbox1["y_min"], bbox2["y_min"])
    x_max = min(bbox1["x_max"], bbox2["x_max"])
    y_max = min(bbox1["y_max"], bbox2["y_max"])

    intersection_area = max(0, x_max - x_min) * max(0, y_max - y_min)

    bbox1_area = (bbox1["x_max"] - bbox1["x_min"]) * (bbox1["y_max"] - bbox1["y_min"])
    bbox2_area = (bbox2["x_max"] - bbox2["x_min"]) * (bbox2["y_max"] - bbox2["y_min"])

    iou = intersection_area / (bbox1_area + bbox2_area - intersection_area)

    return iou

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

# =========================================================================================================================

def get_center(bbox):

    center_x = (bbox["x_min"] + bbox["x_max"]) / 2
    center_y = (bbox["y_min"] + bbox["y_max"]) / 2
    return center_x, center_y


def get_center_of_the_circle(points):

    points = np.array(points, dtype=np.float32)
    A = np.column_stack((-2 * points[:, 0], -2 * points[:, 1], np.ones(len(points))))
    result = np.linalg.lstsq(A, -1 * (points[:, 0]**2 + points[:, 1]**2), rcond=None)
    center_x, center_y, r_squared = result[0]
    return center_x, center_y


def sort_counter_clockwise(detections):

    points = [get_center(detection["bbox"]) for detection in detections if get_center(detection["bbox"]) is not None]

    if not points:
        return [], None, None

    center_x, center_y = get_center_of_the_circle(points)
    angles = [np.arctan2(y - center_y, x - center_x) for x, y in points]
    # if the center of the circle is to the left of the points, we need to add 2pi to the negative angles to make them positive, or the sort will be wrong
    if center_x > np.mean([x for x, _ in points]):
        angles = [angle + 2 * np.pi if angle < 0 else angle for angle in angles]

    sorted_detections = [d for _, d in sorted(zip(angles, detections), key=lambda x: -x[0])]
    return sorted_detections, center_x, center_y

# =========================================================================================================================
def consolidate_characters(character_list):

    character_data = {}
    for character in character_list:
        if character["class"] not in character_data:
            character_data[character["class"]] = {
                "confidence_sum": 0,
                "count": 0,
                "max_confidence": 0,
            }

        character_data[character["class"]]["confidence_sum"] += character["confidence"]
        character_data[character["class"]]["count"] += 1
        if character["confidence"] > character_data[character["class"]]["max_confidence"]:
            character_data[character["class"]]["max_confidence"] = character["confidence"]

    if character_data:
        # consolidated = max(character_data, key=lambda x: character_data[x]["confidence_sum"] * character_data[x]["count"])
        # consolidated = max(character_data, key=lambda x: character_data[x]["count"])
        # consolidated = max(character_data, key=lambda x: character_data[x]["max_confidence"])
        consolidated = max(character_data, key=lambda x: character_data[x]["confidence_sum"])  #This has the best results
    else:
        consolidated = "X"

    return consolidated
# =========================================================================================================================

class MainCutterDetection:

    def __init__(self, component_data, ocr_component_key):

        self.type = component_data["type"]
        self.component_data = component_data
        self.centroid = component_data["centroid"]

        self.ocr_component_key = ocr_component_key
        self.ocr_detections_list = []
        self.add_ocr_detection(component_data)


    def get_type(self):
            
            return self.type


    def get_detection(self):

            return {
                "class": self.component_data["class"],
                "label": self.component_data["label"],
                "bbox_normalized": self.component_data["bbox_normalized"],
                "bbox": self.component_data["bbox"],
                "color": self.component_data["color"],
                "confidence": self.component_data["confidence"],
            }


    def get_centroid(self):
            
        return self.centroid


    def get_ocr_detections(self):

        return self.ocr_detections_list


    def add_ocr_detection(self, component_data):

        if self.ocr_component_key in component_data:
            ocr_detections = []
            for character, detections in component_data[self.ocr_component_key]["outputs"].items():
                ocr_detections += detections
            ocr_detections.sort(key=lambda x: x["bbox"]["x_min"])
            self.ocr_detections_list.append(ocr_detections)
        else:
            for component_key in [key for key in component_data.keys() if key.startswith("component_")]:
                for class_id, detections in component_data[component_key]["outputs"].items():
                    for detection in detections:
                        self.add_ocr_detection(detection)


    def append_characters(self, character_position, avg_width_multiplier=0.95):

        if "tare" == self.type:

            for ocr_detections in self.ocr_detections_list:

                if len(ocr_detections) == 0 or len(ocr_detections) > 4:
                    # log.warning(f"Invalid number of characters for tare: {len(ocr_detections)}. Detected characters: {ocr_detections}. Skipping...")
                    continue

                detection_width_sum = 0
                for detection in ocr_detections:
                    detection_width_sum += detection["bbox"]["x_max"] - detection["bbox"]["x_min"]
                avg_width = detection_width_sum / len(ocr_detections)

                character_data_list = []
                len_ocr_detections = len(ocr_detections)

                for i in range(len_ocr_detections):
                    character_data_list.append(ocr_detections[i])
                    if i != len_ocr_detections - 1:
                        if ocr_detections[i + 1]["bbox"]["x_min"] - ocr_detections[i]["bbox"]["x_max"] > avg_width*avg_width_multiplier:
                            character_data_list.append({"class": "X", "confidence": 0})

                if len(character_data_list) == 3 and len_ocr_detections == 3:
                    if character_data_list[0]["class"] == "1":
                        character_data_list.append({"class": "X", "confidence": 0})
                    else:
                        character_data_list.insert(0, {"class": "X", "confidence": 0})

                if len(character_data_list) == 4:
                    for index, character_data in enumerate(character_data_list, start=1):
                        character_position[str(index)].append(character_data)

        elif "expiration_date" == self.type:

            for ocr_detections in self.ocr_detections_list:

                filtered_detections, removed_detections = remove_overlapping_detections(ocr_detections)
                if len(filtered_detections) < 3:
                    continue

                sorted_detections, center_x, center_y = sort_counter_clockwise(filtered_detections)

                if len(sorted_detections) == 3 and sorted_detections[0]["class"] == "0":
                    sorted_detections.insert(0, {"class": "2", "confidence": 0})
                else:
                    min_distance = float('inf')
                    for i in range(len(sorted_detections) - 1):
                        distance = math.dist(
                            get_center(sorted_detections[i]["bbox"]),
                            get_center(sorted_detections[i + 1]["bbox"])
                        )
                        if distance < min_distance:
                            min_distance = distance

                    for i in range(len(sorted_detections) - 1):
                        distance = math.dist(
                            get_center(sorted_detections[i]["bbox"]),
                            get_center(sorted_detections[i + 1]["bbox"])
                        )
                        if distance > min_distance * 1.4:
                            sorted_detections.insert(i + 1, {"class": "X", "confidence": 0})
                            break

                if len(sorted_detections) == 3 and "X" not in [detection["class"] for detection in sorted_detections]:
                    sorted_detections.append({"class": "X", "confidence": 0})

                # DEBUG ===========================================
                # show_image = True
                # if show_image:
                #     offset = 300
                #     # create image with points and show
                #     image = np.zeros((1400, 1400, 3), dtype=np.uint8)
                #     for index, detection in enumerate(removed_detections):
                #         point = get_center(detection["bbox"])
                #         cv2.circle(image, (int(point[0]) - offset, int(point[1]) - offset), 5, (0, 255, 255), -1)
                #         cv2.putText(image, detection["class"], (int(point[0]) - offset - 10, int(point[1]) - offset + 40), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 255), 2)
                #         cv2.putText(image, detection["class"], (20 * (index+1), 80), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 255), 2)

                #     for index, detection in enumerate(sorted_detections):
                #         if "bbox" in detection:
                #             point = get_center(detection["bbox"])
                #             cv2.circle(image, (int(point[0]) - offset, int(point[1]) - offset), 5, (0, 255, 0), -1)
                #             cv2.putText(image, detection["class"], (int(point[0]) - offset - 10, int(point[1]) - offset + 40), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                #         cv2.putText(image, detection["class"], (20 * (index+1), 40), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)


                #     # draw center
                #     cv2.circle(image,  (int(center_x) - offset, int(center_y) - offset), 5, (255, 0, 0), -1)
                #     cv2.imshow("image", image)
                #     cv2.waitKey(0)
                #     cv2.destroyAllWindows()
                # DEBUG END =======================================

                if len(sorted_detections) != 4:
                    continue

                for index, detection in enumerate(sorted_detections, start=1):
                    character_position[str(index)].append(detection)
# =========================================================================================================================

class ConsolidateResults:


    def __init__(self, flow_data, main_cutter_name="CUTTER", cylinder_tare_ocr_name="TARA", cylinder_expiration_date_ocr_name="OCR VENCIMENTO", tare_class_label="TARA", expiration_date_class_label="FERRADURA",color_class_label="COR BOTIJAO"):

        self.flow_data = flow_data
        self.main_cutter_name = main_cutter_name
        self.cylinder_tare_ocr_name = cylinder_tare_ocr_name
        self.cylinder_expiration_date_ocr_name = cylinder_expiration_date_ocr_name
        self.brands = []
        self.node_components = get_node_components_data(
            flow_data,
            main_cutter_name=main_cutter_name,
            cylinder_tare_ocr_name=cylinder_tare_ocr_name,
            cylinder_expiration_date_ocr_name=cylinder_expiration_date_ocr_name,
            tare_class_label=tare_class_label,
            expiration_date_class_label=expiration_date_class_label,
            color_class_label=color_class_label

        )
        log.info(f'node_components log: {self.node_components}')
        self.max_centroid_distance = 100

  
    def __call__(self, results):

        classifier_detections = {
            "color": {},
            "exists": False
        }
        main_cutter_detections = {
            "tare": [],
            "expiration_date": []
        }
        brand_detections = {
            "exists": False,
            "brand": {

            }
        }
        tare_plate_dectected = False
        log.info(f'results: {results}')

        for index in results:
            result = results[index]
            log.info(f"Processing result: {result}")
            for component_key in [key for key in result.keys() if key.startswith("component_")]:
                log.info(f"Processing component_key: {component_key}")
                if component_key != self.node_components["main_cutter"]:
                    if component_key == self.node_components["color_class_component"]:
                        classifier_detections["color"] = result[component_key]["outputs"]
                        classifier_detections["exists"] = True
                    continue

                component = result[component_key]
                log.info(f'component: {component}')
                valid_class_id_list = [class_id for class_id in component["outputs"].keys() if class_id in [self.node_components["tare_class_id"], self.node_components["expiration_date_class_id"]]]
                log.info(f'valid_class_list: {valid_class_id_list}')
                brands = []
                for key in self.node_components.keys():
                    log.info(f'valid_class_list: {key} value: {self.node_components[key]}')
                    if key != 'main_cutter' and key != 'tare_class_id' \
                        and key != 'expiration_date_class_id' and key != 'ocr_tare_component' and key != 'ocr_expiration_date_component':
                        brands.append(self.node_components[key])
                        valid_class_id_list.append(self.node_components[key])



                log.info(f'brands: {brands} ')

                for class_id in valid_class_id_list:
                     for detection in component["outputs"][class_id]:
                        log.info(f'detection: {detection} ')
                        tare_plate_dectected |= detection['label'] == "TARA"


                        if detection['class'] in brands:
                            log.info(f'detection valida: {detection}')
                            detection["type"] = "brand"
                            brand_detections['brand'] = detection
                            brand_detections['exists'] = True

                            #main_cutter_detections['brand'].append(MainCutterDetection(detection, class_id))

                        detection["centroid"] = [
                            (detection["bbox"]["x_min"] + detection["bbox"]["x_max"]) / 2,
                            (detection["bbox"]["y_min"] + detection["bbox"]["y_max"]) / 2
                        ]
                        if class_id == self.node_components["expiration_date_class_id"]:
                            detection["type"] = "expiration_date"
                        else:
                            detection["type"] = "tare"

                        
                        for cutter_detection in main_cutter_detections[detection["type"]]:
                            distance = math.dist(detection["centroid"], cutter_detection.get_centroid())
                            # if same detection
                            if cutter_detection.get_type() == detection["type"] and distance < self.max_centroid_distance:
                                cutter_detection.add_ocr_detection(detection)
                                break
                        else:
                            component_key = self.node_components["ocr_tare_component"] if detection["type"] == "tare" else self.node_components["ocr_expiration_date_component"]
                            main_cutter_detections[detection["type"]].append(MainCutterDetection(detection, component_key))

        final_result = {
            "tare": {
                "detections": [],
                "score": 100,
                "consolidated_text": 0,
                "text": "",
                "missing_character": False,
                "cutter_detected": False,
                "character_position": {
                    "1": [],
                    "2": [],
                    "3": [],
                    "4": []
                }
            },
            "expiration_date": {
                "detections": [],
                "score": 100,
                "consolidated_text": 0,
                "text": "",
                "missing_character": False,
                "cutter_detected": False,
                "character_position": {
                    "1": [],
                    "2": [],
                    "3": [],
                    "4": []
                }
            }
        }

        for detection_type in main_cutter_detections:
            for index, cutter_detection in enumerate(main_cutter_detections[detection_type]):
                final_result[detection_type]["detections"].append(cutter_detection.get_detection())
                cutter_detection.append_characters(final_result[detection_type]["character_position"])
               
            final_result[detection_type]["cutter_detected"] = len(final_result[detection_type]["detections"]) > 0

        # consolidate tare
        consolidated_first_digit = 1
        consolidated_second_digit = consolidate_characters(final_result["tare"]["character_position"]["2"])
        consolidated_third_digit = consolidate_characters(final_result["tare"]["character_position"]["3"])
        consolidated_fourth_digit = consolidate_characters(final_result["tare"]["character_position"]["4"])

        if consolidated_second_digit == "X" or consolidated_third_digit == "X":
            final_result["tare"]["missing_character"] = True
        else:
            final_result["tare"]["consolidated_text"] = int(consolidated_second_digit + consolidated_third_digit)

        final_result["tare"]["text"] = f"{consolidated_first_digit}{consolidated_second_digit}{consolidated_third_digit}{consolidated_fourth_digit}"

        # consolidate expiration date
        consolidated_first_digit = 2
        consolidated_second_digit = 0
        consolidated_third_digit = consolidate_characters(final_result["expiration_date"]["character_position"]["3"])
        consolidated_fourth_digit = consolidate_characters(final_result["expiration_date"]["character_position"]["4"])

        if consolidated_first_digit == "X" or consolidated_second_digit == "X" or consolidated_third_digit == "X" or consolidated_fourth_digit == "X":
            final_result["expiration_date"]["missing_character"] = True
        else:
            final_result["expiration_date"]["consolidated_text"] = int(f"{consolidated_first_digit}{consolidated_second_digit}{consolidated_third_digit}{consolidated_fourth_digit}")

        final_result["expiration_date"]["text"] = f"{consolidated_first_digit}{consolidated_second_digit}{consolidated_third_digit}{consolidated_fourth_digit}"
        
        if classifier_detections["exists"]:
            log.info(f"classifier_detections: {classifier_detections['color']}")
            final_result["tare"]["detections"].append(classifier_detections["color"])

        if brand_detections["exists"]:
            final_result["brand"] = brand_detections["brand"]["label"]
        else:
            final_result["brand"] = "NOT FOUND"

        final_result["tare_plate_dectected"] = tare_plate_dectected 
        
        return final_result


if __name__ == "__main__":
    flow_id = "671953f155784c001a41d0dc"
    #flow_id = "632b68c276fafd001b164115"
    flow_file = os.path.join("/workspaces/c++/conf", flow_id + ".json")
    log.info(f'flow_file: {flow_file}')
    with open(flow_file, 'r', newline='', encoding='utf8') as fp:
        flow_data = json.load(fp)

    consolidate_results = ConsolidateResults(flow_data)

# if __name__ == "__main__":

#     from test.tests_utils import get_log_data
#     import cv2

#     script_dir = os.path.dirname(os.path.realpath(__file__))
#     results_dir = os.path.join(script_dir, "test", "results")
#     log_data = get_log_data(os.path.join(script_dir, "test", "logs"), ["KC-20231129"])
#     image_dir = "/opt/eyeflow/data/copa/fotos_20231129"
#     # show_failed_images = True
#     show_failed_images = False

#     # flow_id = "6499aa26de3a0c001b73cf7c"    # Tara e Vencimento Dev https://app.eyeflow.ai/app/6099613b20da17001941822e/flow/6499aa26de3a0c001b73cf7c/edit
#     flow_id = "632b68c276fafd001b164115"

#     flow_file = os.path.join("/opt/eyeflow/data/flow", flow_id + ".json")
#     with open(flow_file, 'r', newline='', encoding='utf8') as fp:
#         flow_data = json.load(fp)

#     consolidate_results = ConsolidateResults(flow_data)

#     count_ok = 0
#     count_nok = 0
#     count_has_tare = 0
#     for result_file in os.listdir(results_dir):
#         file_id = result_file.split(".")[0]
#         if file_id not in log_data:
#             continue

#         with open(os.path.join(results_dir, result_file), 'r', newline='', encoding='utf8') as fp:
#             results = json.load(fp)
#             final_result = consolidate_results(results)
#             tare_consolidated = final_result["tare"]["consolidated_text"]
#             expiratio_date_consolidated = final_result["expiration_date"]["consolidated_text"]
#             text = final_result["tare"]["text"]
#             tare_in_log = int(log_data[file_id]["tare"])

#             if final_result["tare"]["cutter_detected"]:
#                 count_has_tare += 1

#             if tare_consolidated == tare_in_log or (text.endswith("5") and tare_consolidated + 1 == tare_in_log):
#                 count_ok += 1
#             else:
#                 count_nok += 1
#                 if show_failed_images:
#                     image_file = os.path.join(image_dir, file_id + ".bmp")
#                     image = cv2.imread(image_file)
#                     cv2.imshow("image", image)
#                     cv2.waitKey(0)
#                     cv2.destroyAllWindows()
#                     pass

#     print(f"count_ok: {count_ok}. count_nok: {count_nok}. count_has_tare: {count_has_tare}")

