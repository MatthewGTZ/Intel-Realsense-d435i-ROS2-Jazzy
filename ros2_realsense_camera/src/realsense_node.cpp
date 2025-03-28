#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/opencv.hpp>
#include <librealsense2/rs.hpp>

class RealSensePublisher : public rclcpp::Node {
public:
    RealSensePublisher(bool use_ir_only) 
        : Node("realsense_publisher"), use_ir_only_(use_ir_only) {
        
        // Crear el publisher adecuado según el modo
        if (use_ir_only_) {
            ir_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("/camera/image", 10);
        } else {
            rgb_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("/camera/image", 10);
        }

        rs2::config cfg;
        if (use_ir_only_) {
            cfg.enable_stream(RS2_STREAM_INFRARED, 1, 1280, 720, RS2_FORMAT_Y8, 30);
        } else {
            cfg.enable_stream(RS2_STREAM_COLOR, 1280, 720, RS2_FORMAT_RGB8, 30);
        }

        try {
            pipeline_.start(cfg);
            std::this_thread::sleep_for(std::chrono::seconds(2));  // Espera 2 segundos para evitar errores
            
            // Obtener el sensor de la cámara y deshabilitar el emisor láser
            rs2::device device = pipeline_.get_active_profile().get_device();
            for (auto&& sensor : device.query_sensors()) {
                if (sensor.supports(RS2_OPTION_EMITTER_ENABLED)) {
                    sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0);
                }
            }


            if (use_ir_only_) {
                RCLCPP_INFO(this->get_logger(), "Cámara RealSense iniciada en modo INFRARROJO.");
            } else {
                RCLCPP_INFO(this->get_logger(), "Cámara RealSense iniciada en modo RGB.");
            }
        } catch (const rs2::error &e) {
            RCLCPP_ERROR(this->get_logger(), "Error al iniciar RealSense: %s", e.what());
            rclcpp::shutdown();
            return;
        }

        timer_ = this->create_wall_timer(std::chrono::milliseconds(100),
            std::bind(&RealSensePublisher::capture_frame, this));
    }

private:
    void capture_frame() {
        rs2::frameset frames;
        try {
            frames = pipeline_.wait_for_frames();
        } catch (const rs2::error &e) {
            RCLCPP_WARN(this->get_logger(), "No se recibió un frame a tiempo: %s", e.what());
            return;
        }

        if (use_ir_only_) {
            rs2::frame ir_frame = frames.get_infrared_frame();
            if (!ir_frame) {
                RCLCPP_WARN(this->get_logger(), "Frame infrarrojo no disponible.");
            } else {
                process_ir_frame(ir_frame);
            }
        } else {
            rs2::frame color_frame = frames.get_color_frame();
            if (!color_frame) {
                RCLCPP_WARN(this->get_logger(), "Frame de color no disponible.");
            } else {
                process_rgb_frame(color_frame);
            }
        }
    }

    void process_rgb_frame(const rs2::frame &color_frame) {
        int width = color_frame.as<rs2::video_frame>().get_width();
        int height = color_frame.as<rs2::video_frame>().get_height();

        cv::Mat image(cv::Size(width, height), CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);  // Convertir RGB a BGR para OpenCV

        auto msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", image).toImageMsg();
        rgb_publisher_->publish(*msg);

        //cv::imshow("RealSense RGB", image);
        //cv::waitKey(1);
    }

    void process_ir_frame(const rs2::frame &ir_frame) {
        int width = ir_frame.as<rs2::video_frame>().get_width();
        int height = ir_frame.as<rs2::video_frame>().get_height();

        cv::Mat ir_image(cv::Size(width, height), CV_8UC1, (void*)ir_frame.get_data(), cv::Mat::AUTO_STEP);
        cv::flip(ir_image,ir_image,0);


        auto msg = cv_bridge::CvImage(std_msgs::msg::Header(), "mono8", ir_image).toImageMsg();
        ir_publisher_->publish(*msg);

        //cv::imshow("RealSense Infrarrojo", ir_image);
        //cv::waitKey(1);
    }

    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr rgb_publisher_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr ir_publisher_;
    rs2::pipeline pipeline_;
    rclcpp::TimerBase::SharedPtr timer_;
    bool use_ir_only_; // Variable para definir si solo usamos la imagen infrarroja
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);

    bool use_ir_only = false;
    if (argc > 1 && std::string(argv[1]) == "mono") {
        use_ir_only = true;
    }

    auto node = std::make_shared<RealSensePublisher>(use_ir_only);

    if (!rclcpp::ok()) {
        RCLCPP_ERROR(node->get_logger(), "ROS 2 no se inicializó correctamente.");
        return 1;
    }

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
