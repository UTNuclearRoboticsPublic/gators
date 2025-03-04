#include "gators/game_visualizer.h"

GameVisualizer::GameVisualizer(rclcpp::Node::SharedPtr node) 
: node_(node) 
{
    RCLCPP_INFO(node->get_logger(), "[Game Visualizer] Created game visualizer.");
}

void GameVisualizer::clearVisualizer(void) 
{
    sensor_msgs::msg::PointCloud2 points;
    PointCloud::Ptr pcl_points (new PointCloud);
    pcl::toROSMsg(*pcl_points, points);
    points.header.frame_id = frame_id_;
    
    visualization_msgs::msg::Marker marker;
    marker.header.frame_id = frame_id_;
    marker.header.stamp = node_->get_clock()->now();
    marker.action = visualization_msgs::msg::Marker::DELETEALL;

    Visualizer_t *env_vis;
    for (const auto &p : envs_) {
        env_vis = &envs_.at(p.first);
        env_vis->points_pub->publish(points);
        env_vis->marker_pub->publish(marker);
    }

    Visualizer_t *player_vis;
    for (const auto &p : players_) {
        player_vis = &players_.at(p.first);
        player_vis->points_pub->publish(points);
        player_vis->marker_pub->publish(marker);
    }

    RCLCPP_INFO(node_->get_logger(), "[Game Visualizer] Cleared visuals.");
}

void GameVisualizer::clearObjects(void) 
{
    envs_.clear();
    players_.clear();
    RCLCPP_INFO(node_->get_logger(), "[Game Visualizer] Cleared objects.");
}

void GameVisualizer::addEnvironment(std::string env_id, std::vector<int16_t> points_rgb) 
{
    Visualizer_t env_vis;

    env_vis.id = env_id;
    env_vis.points_rgb.at(0) = points_rgb.at(0);
    env_vis.points_rgb.at(1) = points_rgb.at(1);
    env_vis.points_rgb.at(2) = points_rgb.at(2);

    PointCloud::Ptr pcl_points (new PointCloud);
    pcl::toROSMsg(*pcl_points, env_vis.points);

    auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(10)).transient_local();  
    env_vis.points_pub = node_->create_publisher<sensor_msgs::msg::PointCloud2>(env_vis.id + points_topic_, qos_profile);
    env_vis.marker_pub = node_->create_publisher<visualization_msgs::msg::Marker>(env_vis.id + marker_topic_, qos_profile);

    envs_.insert({env_vis.id, env_vis});

    RCLCPP_INFO(node_->get_logger(), "[Game Visualizer] Added an environment: %s", env_id.c_str());
}

void GameVisualizer::addEnvironmentMarker(std::string env_id, std::string mesh, std::vector<float_t> position, float alpha) 
{
    Visualizer_t *env_vis = &envs_.at(env_id);

    env_vis->marker.header.frame_id = frame_id_;
    env_vis->marker.ns = "environment";
    env_vis->marker.id = 0;
    env_vis->marker.type = visualization_msgs::msg::Marker::MESH_RESOURCE;
    env_vis->marker.action = visualization_msgs::msg::Marker::ADD;
    env_vis->marker.pose.position.x = position.at(0);
    env_vis->marker.pose.position.y = position.at(1);
    env_vis->marker.pose.position.z = position.at(2);
    env_vis->marker.pose.orientation.x = 0.0;
    env_vis->marker.pose.orientation.y = 0.0;
    env_vis->marker.pose.orientation.z = 0.0;
    env_vis->marker.pose.orientation.w = 1.0;
    env_vis->marker.scale.x = 1.0;
    env_vis->marker.scale.y = 1.0;
    env_vis->marker.scale.z = 1.0;
    env_vis->marker.mesh_resource = mesh;
    // env_vis->marker.mesh_use_embedded_materials = true;
    env_vis->marker.color.b = 0.50f;
    env_vis->marker.color.g = 0.50f;
    env_vis->marker.color.r = 0.50;
    env_vis->marker.color.a = alpha;
    env_vis->marker.lifetime = rclcpp::Duration(0, 0);

    RCLCPP_INFO(node_->get_logger(), "[Game Visualizer] Added environment marker: %s", env_id.c_str());
}

void GameVisualizer::publishEnvironmentMarker(std::string env_id) 
{
    Visualizer_t *env_vis = &envs_.at(env_id);

    env_vis->marker.header.stamp = node_->get_clock()->now();
    env_vis->marker_pub->publish(env_vis->marker);

    RCLCPP_INFO(node_->get_logger(), "[Game Visualizer] Published environment marker: %s", env_id.c_str());
}

void GameVisualizer::addEnvironmentPoints(std::string env_id, std::string pcd) 
{
    pcl::PCDReader reader;
    PointCloud::Ptr pcl_points (new PointCloud);
    reader.read(pcd, *pcl_points);

    addEnvironmentPoints(env_id, pcl_points);
}

void GameVisualizer::addEnvironmentPoints(std::string env_id, sensor_msgs::msg::PointCloud2 &cloud)
{
    PointCloud::Ptr pcl_points (new PointCloud);
    pcl::fromROSMsg(cloud, *pcl_points);

    addEnvironmentPoints(env_id, pcl_points);
}

void GameVisualizer::addEnvironmentPoints(std::string env_id, PointCloud::Ptr &pcl_points) 
{
    Visualizer_t *env_vis = &envs_.at(env_id);

    for (std::size_t i = 0; i < pcl_points->points.size(); i++) {
        pcl_points->points[i].r = env_vis->points_rgb.at(0);
        pcl_points->points[i].g = env_vis->points_rgb.at(1);
        pcl_points->points[i].b = env_vis->points_rgb.at(2);
    }

    pcl::toROSMsg(*pcl_points, env_vis->points);
    env_vis->points.header.frame_id = frame_id_;

    RCLCPP_INFO(node_->get_logger(), "[Game Visualizer] Added environment points: %s", env_id.c_str());
}

void GameVisualizer::publishEnvironmentPoints(std::string env_id) 
{
    Visualizer_t *env_vis = &envs_.at(env_id);

    env_vis->points_pub->publish(env_vis->points);

    RCLCPP_INFO(node_->get_logger(), "[Game Visualizer] Published environment points: %s", env_id.c_str());
}

void GameVisualizer::addPlayer(std::string player_id, std::string mesh, bool input_color, bool use_color_for_player, int type) 
{
    Visualizer_t player_vis;

    player_vis.id = player_id;

    if (input_color == true) {
        // User input case
        std::vector<int16_t> colors = inputColors();
        player_vis.points_rgb.at(0) = colors.at(0);
        player_vis.points_rgb.at(1) = colors.at(1);
        player_vis.points_rgb.at(2) = colors.at(2);
    } else if (type == -1) {
        // Random Case
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(0, 255);
        player_vis.points_rgb.at(0) = distr(gen);
        player_vis.points_rgb.at(1) = distr(gen);
        player_vis.points_rgb.at(2) = distr(gen);
    } else {
        if (type == 0) { // drone
            player_vis.points_rgb.at(0) = 252;
            player_vis.points_rgb.at(1) = 141;
            player_vis.points_rgb.at(2) = 98;
        }
        else if (type == 1) { // quadruped
            player_vis.points_rgb.at(0) = 102;
            player_vis.points_rgb.at(1) = 194;
            player_vis.points_rgb.at(2) = 165;
        }
        else { // gantry
            player_vis.points_rgb.at(0) = 141;
            player_vis.points_rgb.at(1) = 160;
            player_vis.points_rgb.at(2) = 203;
        }
    }

    player_vis.marker.header.frame_id = frame_id_;
    player_vis.marker.ns = player_vis.id;
    player_vis.marker.id = 0;
    player_vis.marker.type = visualization_msgs::msg::Marker::MESH_RESOURCE;
    player_vis.marker.action = visualization_msgs::msg::Marker::ADD;
    player_vis.marker.pose.position.x = 0.0;
    player_vis.marker.pose.position.y = 0.0;
    player_vis.marker.pose.position.z = 0.0;
    player_vis.marker.pose.orientation.x = 0.0;
    player_vis.marker.pose.orientation.y = 0.0;
    player_vis.marker.pose.orientation.z = 0.0;
    player_vis.marker.pose.orientation.w = 1.0;
    player_vis.marker.scale.x = 1.0;
    player_vis.marker.scale.y = 1.0;
    player_vis.marker.scale.z = 1.0;
    player_vis.marker.mesh_resource = mesh;
    player_vis.marker.lifetime = rclcpp::Duration(0, 0);
    if (use_color_for_player) {
        player_vis.marker.color.r = static_cast<float>(player_vis.points_rgb.at(0)) / 255;
        player_vis.marker.color.g = static_cast<float>(player_vis.points_rgb.at(1)) / 255;
        player_vis.marker.color.b = static_cast<float>(player_vis.points_rgb.at(2)) / 255;
        player_vis.marker.color.a = 1.0;
    } else {
        player_vis.marker.mesh_use_embedded_materials = true;
    }

    PointCloud::Ptr pcl_points (new PointCloud);
    pcl::toROSMsg(*pcl_points, player_vis.points);

    auto qos_profile = rclcpp::QoS(rclcpp::KeepLast(10)).transient_local();  
    player_vis.points_pub = node_->create_publisher<sensor_msgs::msg::PointCloud2>(player_vis.id + points_topic_, qos_profile);
    player_vis.marker_pub = node_->create_publisher<visualization_msgs::msg::Marker>(player_vis.id + marker_topic_, qos_profile);

    players_.insert({player_vis.id, player_vis});

    RCLCPP_INFO(node_->get_logger(), "[Game Visualizer] Added a player: %s", player_id.c_str());
}

void GameVisualizer::movePlayerMarker(std::string player_id, std::vector<float_t> position) 
{
    Visualizer_t *player_vis = &players_.at(player_id);

    player_vis->marker.pose.position.x = position.at(0);
    player_vis->marker.pose.position.y = position.at(1);
    player_vis->marker.pose.position.z = position.at(2);
    publishPlayerMarker(player_id);
}

void GameVisualizer::publishPlayerMarker(std::string player_id) 
{
    Visualizer_t *player_vis = &players_.at(player_id);
    
    player_vis->marker.header.stamp = node_->get_clock()->now();
    player_vis->marker_pub->publish(player_vis->marker);
}

std::vector<int16_t> GameVisualizer::inputColors() 
{
    std::vector<int16_t> colors{0,0,0};
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, 255);

    std::string R;
    std::cout<<"R:" <<std::endl;
    std::getline(std::cin,R);
    try{
        int R_dig = std::stoi(R);
        colors.at(0) = R_dig;
    }
    catch (...){
        colors.at(0) = distr(gen);
    }

    std::string G;
    std::cout<<"G:" <<std::endl;
    std::getline(std::cin,G);
    try{
        int G_dig = std::stoi(G);
        colors.at(1) = G_dig;
    }
    catch (...){
        colors.at(1) = distr(gen);;
    }

    std::string B;
    std::cout<<"B:" <<std::endl;
    std::getline(std::cin,B);
    try{
        int B_dig = std::stoi(B);
        if (B_dig > 255 || B_dig < 0){
            throw 1;
        }
        colors.at(2) = B_dig;
    }
    catch (...){
        colors.at(2) = distr(gen);
    }
    return colors;
}

void GameVisualizer::addPlayerPoints(std::string player_id,  sensor_msgs::msg::PointCloud2 new_points) 
{
    Visualizer_t *player_vis = &players_.at(player_id);

    PointCloud::Ptr pcl_points (new PointCloud);
    PointCloud::Ptr new_pcl_points (new PointCloud);

    pcl::fromROSMsg(player_vis->points, *pcl_points);
    pcl::fromROSMsg(new_points, *new_pcl_points);

    for (std::size_t i = 0; i < new_pcl_points->points.size(); i++) {
        new_pcl_points->points[i].r = player_vis->points_rgb.at(0);
        new_pcl_points->points[i].g = player_vis->points_rgb.at(1);
        new_pcl_points->points[i].b = player_vis->points_rgb.at(2);
    }

    *pcl_points += *new_pcl_points;

    pcl::toROSMsg(*pcl_points, player_vis->points);
    player_vis->points.header.frame_id = frame_id_;

    publishPlayerPoints(player_id);
}

void GameVisualizer::publishPlayerPoints(std::string player_id) 
{
    Visualizer_t *player_vis = &players_.at(player_id);

    player_vis->points_pub->publish(player_vis->points);
}
