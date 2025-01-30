#include <iostream>
#include <nlohmann/json.hpp>
#include "glog/logging.h"
#include <filesystem>
#include "image.server.h"
#include "manager.inspection.h"
#include <Python.h>


void call_python_class(const std::string &flow_data, const std::string &results)
    {
        // Py_Initialize(); // Inicializa o interpretador Python

        PyObject *pName, *pModule, *pClass, *pInstance, *pFunc, *pArgs, *pValue;

        PyRun_SimpleString("import sys; sys.path.append('../py')");

        // Nome do arquivo Python (sem extensão .py)
        pName = PyUnicode_DecodeFSDefault("test2"); // Nome do script Python
        pModule = PyImport_Import(pName);                        // Importa o módulo
        Py_DECREF(pName);

        if (pModule != nullptr)
        {
            // Obtém a classe 'ConsolidateResults'
            pClass = PyObject_GetAttrString(pModule, "ConsolidateResults");

            if(pClass == nullptr)
            {
                PyErr_Print();
                return;
            }

            if (pClass && PyCallable_Check(pClass))
            {
                // Chama o construtor da classe com o fluxo de dados
                pArgs = PyTuple_Pack(1, PyUnicode_FromString(flow_data.c_str()));
                pInstance = PyObject_CallObject(pClass, pArgs);
                Py_DECREF(pArgs);

                if (pInstance != nullptr)
                {
                    // Chama o método __call__ da classe 'ConsolidateResults'
                    pFunc = PyObject_GetAttrString(pInstance, "__call__");

                    if (pFunc && PyCallable_Check(pFunc))
                    {
                        // Passa o 'results' como argumento
                        pArgs = PyTuple_Pack(1, PyUnicode_FromString(results.c_str()));
                        pValue = PyObject_CallObject(pFunc, pArgs);
                        Py_DECREF(pArgs);

                        if (pValue != nullptr)
                        {
                            std::cout << "Resultado do Python: " << PyUnicode_AsUTF8(pValue) << std::endl;
                            Py_DECREF(pValue);
                        }
                        else
                        {
                            PyErr_Print();
                        }
                    }
                    else
                    {
                        PyErr_Print();
                    }
                    Py_DECREF(pFunc);
                    Py_DECREF(pInstance);
                }
                else
                {
                    PyErr_Print();
                }
            }
            Py_DECREF(pClass);
            Py_DECREF(pModule);
        }
        else
        {
            PyErr_Print();
        }

        // Py_Finalize(); // Finaliza o interpretador Python
    }


void add_event(std::string path,std::string inspection_id,std::string image_id,copa::InspectionManager &inspectionManager) {
     for (const auto & entry : std::filesystem::directory_iterator(path))
    {
        std::cout << entry.path() << std::endl;
        /// abrir arquivo json
        std::ifstream i(entry.path());
        json j = json::parse(i);
        std::cout << j.dump(4) << std::endl;

        inspectionManager.add_data_event(inspection_id,image_id,j);
    }
}

int main(int argc, char* argv[])
{
    Py_Initialize(); // Inicializa o interpretador Python
    // Create log directory if it doesn't exist
    std::cout << "Create log directory if it doesn't exist file: " << argv[0] <<  std::endl;
  // Diretório para armazenar logs
    const std::string log_dir = "/workspaces/c++/log";

    // Verifica se o diretório existe, caso contrário, cria
    if (!std::filesystem::exists(log_dir)) {
        std::cout << "Log directory does not exist. Creating: " << log_dir << std::endl;
        std::filesystem::create_directories(log_dir);
    }

    // Inicializa o Google Logging
    google::InitGoogleLogging("hello_app");
    FLAGS_log_dir = "/workspaces/c++/log";

    // Habilita log no console além do arquivo
    FLAGS_logtostderr = false;      // Logs não são exibidos apenas no console
    FLAGS_alsologtostderr = true;   // Logs aparecem no console **e** no arquivo
    FLAGS_minloglevel = 0;   

    // std::ifstream file("/workspaces/c++/conf/671953f155784c001a41d0dc.json");
    // if (!file.is_open())
    // {
    //     throw std::runtime_error("Could not open flow.json");
    // }
    // json flow_data = json::parse(file);
    // file.close();

    // call_python_class(flow_data.dump(),"teste");

    ///load json from  folder conf file parms.json
    std::ifstream i("/workspaces/c++/conf/parms.json");
    json parms = json::parse(i);
    LOG(INFO) << parms.dump();

    copa::InspectionManager &inspectionManager = copa::InspectionManager::get_instance();
    inspectionManager.set_parms(parms);

    LOG(INFO) <<  "InspectionManager name: " << inspectionManager.get_name();

  
    copa::ImageServer server("0.0.0.0", 8002);
    server.setupRoutes();
    server.start();

    //sleep(60);

    std::string frame_id_orig =  "9dadac7c-7884-496c-b59f-00006b5c12e132d617f3";
    std::string frame_id_abs =  "6a994b0b-2ad6-42fb-bee1-0000ea4f2f59d05e798d";
    std::string frame_id_border =  "5de62f61-973a-473d-9cae-000048505f7c90edc62b";
    std::string inspection_id = "1737405909";

    inspectionManager.force_inspection(inspection_id,frame_id_orig,frame_id_abs,frame_id_border);

    /// List dir de files em json ./events
    std::string path = "/workspaces/c++/events/image_original";
    add_event(path,inspection_id,frame_id_orig,inspectionManager);

    path = "/workspaces/c++/events/image_abs";
    add_event(path,inspection_id,frame_id_abs,inspectionManager);

    path = "/workspaces/c++/events/image_border";
    add_event(path,inspection_id,frame_id_border,inspectionManager);

    //inspectionManager.consolidate_inspection(inspection_id);

    std::cout << "Server is running in a separate thread. Press Enter to exit..." << std::endl;
    std::cin.get();

    google::ShutdownGoogleLogging();

    Py_Finalize(); // Finaliza o interpretador Python
    return 0;

}