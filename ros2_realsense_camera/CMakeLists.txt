cmake_minimum_required(VERSION 3.10)
project(ros2_realsense_camera)

# Encontrar dependencias
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(OpenCV REQUIRED)
find_package(realsense2 REQUIRED)
find_package(cv_bridge REQUIRED)  # 🔹 Asegurar que se encuentra cv_bridge

# Añadir ejecutable
add_executable(realsense_node src/realsense_node.cpp)

# Vincular librerías necesarias
target_link_libraries(realsense_node realsense2::realsense2 ${OpenCV_LIBS})
target_include_directories(realsense_node PRIVATE ${OpenCV_INCLUDE_DIRS} /opt/ros/jazzy/include)  # 🔹 Asegurar inclusión

# Exportar dependencias
ament_target_dependencies(realsense_node rclcpp sensor_msgs OpenCV realsense2 cv_bridge)  # 🔹 Asegurar que cv_bridge está aquí

# Instalar ejecutable
install(TARGETS
  realsense_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
