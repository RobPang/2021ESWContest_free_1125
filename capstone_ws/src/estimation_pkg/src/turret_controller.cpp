#pragma once
#include "../include/estimation_pkg/turret_controller.hpp"

turret_controller_interface::turret_controller_interface(ros::NodeHandle &nh_,double hz_):
    rate_(hz_),
    position_estimator()
{
    ////////   publisher ////////////////////////////////

    visualization_pub_ = nh_.advertise<visualization_msgs::MarkerArray>("/VisionX/markers", 100);

    /////////  subsciber  ////////////////////////////////

    yolo_detection_sub = nh_.subscribe("/darknet_ros_3d/bounding_boxes",10, &turret_controller_interface::vision_cb, this);

    ////////  msgs filter Sub ////////////////////////////

    // aligned_image_sub.subscribe(nh_,"/camera/aligned_depth_to_color/image_raw",1);
    // aligned_info_sub.subscribe(nh_,"/camera/aligned_depth_to_color/camera_info",1);
    
    // sync = &message_filters::TimeSynchronizer<sensor_msgs::Image, sensor_msgs::CameraInfo>(aligned_image_sub,aligned_info_sub,10);
    // sync->registerCallback(boost::bind(&turret_controller_interface::object_track, _1, _2));

    // nh_.param("robot_description", robot_desc_string, std::string());
    // joint_name ={"base_link","Yaw_Link","Rail_Link","Pitch_Link"
    //             ,"Camera_Pitch_Link","Camera_Link"};

    ///////    parmeter Initialize ////////////////////////

    droneConfig.resize(4);
    thread_num = 2;
    thread_condition  = false;

    //////     set Thread         /////////////////////////

    threads.push_back(std::thread(&KF_drone_estimator::excute_timerThread, &position_estimator));
    threads.push_back(std::thread(&turret_controller_interface::SerailThread, this));
}

turret_controller_interface::~turret_controller_interface()
{
    thread_condition = true;
    position_estimator.setTheadCondition(true);
    for(auto & thread : threads)
    {   
        thread.join();
    }
}

void turret_controller_interface::vision_cb(const gb_visual_detection_3d_msgs::BoundingBoxes3dConstPtr &pose)
{
    // at least one object detected by YOLO KF filter estimate pose
    if(pose->bounding_boxes.size() <= 0){
        std::cout<< "callback not ok " << std::endl;
        return;
    }   
   // std::cout<< "callback ok 1" << std::endl;
    Eigen::Vector3d curConfig;
    curConfig.resize(3);

    curConfig(0) = (pose->bounding_boxes.at(0).xmax + pose->bounding_boxes.at(0).xmin)/2.;
    curConfig(1) = (pose->bounding_boxes.at(0).ymax + pose->bounding_boxes.at(0).ymin)/2.;
    curConfig(2) = (pose->bounding_boxes.at(0).zmax + pose->bounding_boxes.at(0).zmin)/2.;
       // std::cout<< "callback ok 2" << std::endl;

    position_estimator.AddObservation(curConfig);
}
void turret_controller_interface::SerailThread() {
    while(!thread_condition)
    {
        Turret_serial_.m_target.position_Y = position_estimator.getTarget_Y();
        Turret_serial_.m_target.position_P = position_estimator.getTarget_P();
        Camera_serial_.m_target.position_P = position_estimator.getTarget_Tilt();
        
        Turret_serial_.Execute();
        Camera_serial_.Execute();
        static int count;
        if(count % 10){
            count = 0;
            std::cout<< Turret_serial_.m_sendPacket.data.pos_Y << std::endl;
         //   std::cout<< Turret_serial_.m_sendPacket.data.pos_P << std::endl;
            std::cout << "Target_pos_Y = " << Turret_serial_.m_target.position_Y << std::endl;
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    }
}

void turret_controller_interface::killProcess()
{
    thread_condition = true;
    position_estimator.setTheadCondition(true);
    for(auto & thread : threads)
    {   
        thread.join();
    }
}

/*
void turret_controller_interface::object_track()
{
    //obj tracker Model
    //droneConfig = position_estimator.GetPosition();
    droneConfig(3) = 1;

    // trans form to word Model
    Eigen::Affine3f T_CB;
    T_CB.translation() << 0, 0 , 0;
    T_CB.linear() = Eigen::Quaternionf(0,0,0,1).toRotationMatrix();

    // droneConfig = T_CB*droneConfig;
    double target_pen = -atan2(droneConfig(0),droneConfig(1));
     
}*/