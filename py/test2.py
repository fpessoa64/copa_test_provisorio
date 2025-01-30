import os
import json
import math
import datetime
import logging as log

import numpy as np
import cv2

log.basicConfig(level=log.DEBUG,
   format="{asctime} - {levelname} - {message}",
     style="{",
    datefmt="%Y-%m-%d %H:%M",
 )

class ConsolidateResults:

    def __init__(self, flow_data,main_cutter_name="CUTTER", cylinder_tare_ocr_name="TARA", cylinder_expiration_date_ocr_name="OCR VENCIMENTO", tare_class_label="TARA", expiration_date_class_label="FERRADURA",color_class_label="COR BOTIJAO"):
        self.flow_data = json.loads(flow_data)
        self.main_cutter_name = main_cutter_name
        self.cylinder_tare_ocr_name = cylinder_tare_ocr_name
        self.cylinder_expiration_date_ocr_name = cylinder_expiration_date_ocr_name
        self.brands = []
        log.info(f'flow_data: {flow_data}')
        self.node_components = self.get_node_components_data(
            self.flow_data,
            main_cutter_name=main_cutter_name,
            cylinder_tare_ocr_name=cylinder_tare_ocr_name,
            cylinder_expiration_date_ocr_name=cylinder_expiration_date_ocr_name,
            tare_class_label=tare_class_label,
            expiration_date_class_label=expiration_date_class_label,
            color_class_label=color_class_label

        )
        log.info(f'node_components log: {self.node_components}')

        self.max_centroid_distance = 100
        log.info("Classe ConsolidateResults inicializada!")

    def get_node_components_data(self,
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
            # log.info(f'comp: {comp}')
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


    def __call__(self, results):
        print(f"Chamado com: {results}")
        return "ok"