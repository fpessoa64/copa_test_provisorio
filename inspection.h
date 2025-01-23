
#include <iostream>
#include <nlohmann/json.hpp>
#include "glog/logging.h"
#include <mutex>
#include <opencv2/opencv.hpp>
#include "httplib.h"
#include <unordered_set>

using json = nlohmann::json;

namespace copa
{

    class ImageSender
    {
    private:
        std::string inspection_id;
        std::string input_endpoint_url;

    public:
        ImageSender(const std::string &inspection_id, const std::string &url)
            : inspection_id(inspection_id), input_endpoint_url(url) {}

        void sendImage(const cv::Mat &img, const std::string &image_id)
        {
            // Verifica se a imagem é válida
            if (img.empty())
            {
                throw std::runtime_error("Imagem está vazia.");
            }

            /// save image
            std::string imagePath = "/workspaces/c++/temp/" + inspection_id + ".jpg";
            cv::imwrite(imagePath, img);

            // Codifica a imagem para JPG
            std::vector<uchar> img_encoded;
            if (!cv::imencode(".jpg", img, img_encoded))
            {
                throw std::runtime_error("Falha ao codificar a imagem.");
            }

            std::cout << "Enviando imagem " << image_id << " para " << input_endpoint_url << std::endl;

            // Prepara os cabeçalhos
            httplib::Headers headers = {
                {"Content-Type", "application/octet-stream"},
                {"inspection_id", inspection_id},
                {"image_id", image_id}};

            // Configura o cliente HTTP
            httplib::Client cli(input_endpoint_url);

            try
            {
                // Envia a imagem como bytes no corpo da requisição POST
                auto res = cli.Post("/upload_image", headers, reinterpret_cast<const char *>(img_encoded.data()), img_encoded.size(), "application/octet-stream");
                if (!res)
                {
                    std::cerr << "Erro: Falha ao realizar a requisição" << std::endl;
                    return;
                }

                std::cout << "Resposta: " << res->status << std::endl;
                if (res && res->status == 200)
                {
                    // Parse da resposta JSON
                    json response_data = nlohmann::json::parse(res->body);
                }
                else
                {
                    throw std::runtime_error("Erro na resposta HTTP: " + std::to_string(res->status));
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Erro ao enviar a imagem " << image_id << ": " << e.what() << std::endl;
            }
        }
    };

    class Image
    {
    protected:
        cv::Mat image;

    private:
        std::vector<json> events;
        std::mutex lock_image;

    public:
        Image(const cv::Mat &img) : image(img) {}

        virtual ~Image() = default;

        virtual cv::Mat getOriginalImage() const
        {
            return image;
        }

        virtual void fixAnnotations(json &frameData) const = 0;

        virtual cv::Mat getImage() const
        {
            return image;
        }

        json consolidate()
        {
            json result;
            std::lock_guard<std::mutex> lock(lock_image);
            for (const auto &event : events)
            {
                result.push_back(event);
            }
            return result;
        }

        // virtual void add_event(const std::string &event) const = 0;

        void add_event(const json &event)
        {
            std::lock_guard<std::mutex> lock(lock_image);
            events.push_back(event);
            std::cout << "Event added size: " << events.size() << std::endl;
            std::cout << "Event added: " << event.dump() << std::endl;
        }
    };

    class ImageAddBorder : public Image
    {
    private:
        int borderSize;
        int imageWidth;
        int imageHeight;

    public:
        ImageAddBorder(const cv::Mat &img, int borderSize = 25)
            : Image(img), borderSize(borderSize)
        {
            imageWidth = img.cols;
            imageHeight = img.rows;
        }

        ~ImageAddBorder()
        {
            std::cout << "ImageAddBorder destructor called." << std::endl;
            image.release();
        }

        void fixAnnotations(json &frameData) const override
        {
            try
            {
                if(frameData.empty())
                {
                    return;
                }
                // Itera pelas chaves que começam com "component"
                for (auto &item : frameData.items())
                {
                    if (item.key().find("component") == 0)
                    {
                        auto &component = item.value();
                        std::string componentType = component["type"];

                        if (componentType == "cutter" || componentType == "annotator")
                        {
                            for (auto &classOutput : component["outputs"].items())
                            {
                                for (auto &output : classOutput.value())
                                {
                                    // Ajusta as coordenadas dos bounding boxes
                                    output["bbox"]["x_min"] = output["bbox"]["x_min"].get<int>() - borderSize;
                                    output["bbox"]["y_min"] = output["bbox"]["y_min"].get<int>() - borderSize;
                                    output["bbox"]["x_max"] = output["bbox"]["x_max"].get<int>() - borderSize;
                                    output["bbox"]["y_max"] = output["bbox"]["y_max"].get<int>() - borderSize;

                                    // Ajusta as coordenadas normalizadas
                                    output["bbox_normalized"]["x_min"] = static_cast<float>(output["bbox"]["x_min"]) / imageWidth;
                                    output["bbox_normalized"]["y_min"] = static_cast<float>(output["bbox"]["y_min"]) / imageHeight;
                                    output["bbox_normalized"]["x_max"] = static_cast<float>(output["bbox"]["x_max"]) / imageWidth;
                                    output["bbox_normalized"]["y_max"] = static_cast<float>(output["bbox"]["y_max"]) / imageHeight;
                                }
                            }
                        }
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "ImageAddBorder fixAnnotations error: " << e.what() << std::endl;
            }
        }

        cv::Mat getImage() const override
        {
            cv::Mat borderedImage;
            cv::copyMakeBorder(image, borderedImage, borderSize, 0, borderSize, 0, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
            return borderedImage;
        }
    };

    class ImageScaleAbs : public Image
    {
    public:
        ImageScaleAbs(const cv::Mat &img) : Image(img) {}

        ~ImageScaleAbs()
        {
            std::cout << "ImageScaleAbs destructor called." << std::endl;
            image.release();
        }

        cv::Mat getImage() const override
        {
            cv::Mat result;
            cv::convertScaleAbs(image.clone(), result, 2.0, 2.0);
            return result;
        }
        void fixAnnotations(json &frameData) const override {}
    };

    class ImageOriginal : public Image
    {
    public:
        ImageOriginal(const cv::Mat &img) : Image(img) {}

        ~ImageOriginal()
        {
            std::cout << "ImageOriginal destructor called." << std::endl;
            image.release();
        }
        cv::Mat getImage() const override
        {
            return image;
        }
        void fixAnnotations(json &frameData) const override {};
    };

    class Inspection
    {
    public:
        Inspection(const std::string &inspection_id, const json parms);
        ~Inspection()
        {
            std::cout << "Inspection destructor called." << std::endl;
            for (auto &image : data_images)
            {
                delete image.second;
            }
            data_images.clear();
        }

        void add_event(const std::string &key, json &value);
        void update_data(const std::string &key, const json &value);
        void run_inspection(cv::Mat &img);
        void force_inspection(std::string image_id, std::string image_abs_id, std::string image_border_id);
        json get_data();
        json consolidate();

    private:
        std::string m_inspection_id;
        json inspection_data;
        std::mutex lock_;
        std::map<std::string, Image *> data_images;
        json m_parms;
        std::condition_variable m_condition;
        std::thread m_consolidation_thread;
        bool m_stop_thread = false;
        std::mutex m_lock_consolidate;
        std::string generate_id();
        void send_images();
        json prepare();
        // void extract_classes(const json &node, std::unordered_set<std::string> &classes);
        // void update_tara(json &json_schema, const json &value);
        // void update_ferradura(json &json_schema, const json &value);
        // void update_brand(json &json_schema, const json &value);
        void update_schema(json &jsonData, const std::string &key, const json &newValue);
        void update_brand(std::string name, json &jsonData, const std::string &key, const json &newValue);
        void start_consolidation_thread(int consolidation_interval) ;
        void stop_consolidation_thread();
    };
}