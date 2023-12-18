#include "gazebo_omni_drive.hpp"


namespace gazebo_omni_drive_plugins
{
  GazeboOmniRosDrive::GazeboOmniRosDrive() : impl_(std::make_unique<GazeboOmniRosDrivePrivate>())
  {

  }
  
  GazeboOmniRosDrive::~GazeboOmniRosDrive()
  {

  }
  
  void GazeboOmniRosDrive::Load(gazebo::physics::ModelPtr _model, sdf::ElementPtr _sdf)
  {
    impl_->model_ = _model;
    
    // Initialize ROS node
    impl_->ros_node_ = gazebo_ros::Node::Get(_sdf);

    // Get QoS profiles
    const gazebo_ros::QoS & qos = impl_->ros_node_->get_qos();

    // Get number of wheel pairs in the model
    impl_->num_wheel_pairs_ = static_cast<unsigned int>(_sdf->Get<int>("num_wheel_pairs", 1).first);

    if (impl_->num_wheel_pairs_ < 1)
    {
      impl_->num_wheel_pairs_ = 1;
      RCLCPP_WARN(impl_->ros_node_->get_logger(), "Drive requires at least one pair of wheels. Setting [num_wheel_pairs] to 1");
    }

    // Dynamic properties
    impl_->max_wheel_accel_ = _sdf->Get<double>("wheelAccel", 0.0).first;
    impl_->max_wheel_torque_ = _sdf->Get<double>("WheelTorque", 5.0).first;

    // Get joints and Kinematic properties
    std::vector<gazebo::physics::JointPtr> north_joints, west_joints, south_joints, east_joints;

    for (auto north_joint_elem = _sdf->GetElement("North"); 
        north_joint_elem != nullptr; 
        north_joint_elem = north_joint_elem->GetNextElement("North"))
    {
      auto north_joint_name = north_joint_elem->Get<std::string>();
      auto north_joint = _model->GetJoint(north_joint_name);
      if (!north_joint)
      {
        RCLCPP_ERROR(impl_->ros_node_->get_logger(), "Joint [%s] not found, plugin will not work.", north_joint_name.c_str());
        impl_->ros_node_.reset();
        return;
      }
      
      north_joint->SetParam("fmax", 0, impl_->max_wheel_torque_);
      north_joints.push_back(north_joint);
    }

    for (auto west_joint_elem = _sdf->GetElement("West"); 
        west_joint_elem != nullptr; 
        west_joint_elem = west_joint_elem->GetNextElement("West"))
    {
      auto west_joint_name = west_joint_elem->Get<std::string>();
      auto west_joint = _model->GetJoint(west_joint_name);
      if (!west_joint)
      {
        RCLCPP_ERROR(impl_->ros_node_->get_logger(), "Joint [%s] not found, plugin will not work.", west_joint_name.c_str());
        impl_->ros_node_.reset();
        return;
      }
      
      west_joint->SetParam("fmax", 0, impl_->max_wheel_torque_);
      west_joints.push_back(west_joint);
    }
    
    for (auto south_joint_elem = _sdf->GetElement("South"); 
        south_joint_elem != nullptr; 
        south_joint_elem = south_joint_elem->GetNextElement("South"))
    {
      auto south_joint_name = south_joint_elem->Get<std::string>();
      auto south_joint = _model->GetJoint(south_joint_name);
      if (!south_joint)
      {
        RCLCPP_ERROR(impl_->ros_node_->get_logger(), "Joint [%s] not found, plugin will not work.", south_joint_name.c_str());
        impl_->ros_node_.reset();
        return;
      }
      
      south_joint->SetParam("fmax", 0, impl_->max_wheel_torque_);
      south_joints.push_back(south_joint);
    }

    for (auto east_joint_elem = _sdf->GetElement("East"); 
        east_joint_elem != nullptr; 
        east_joint_elem = east_joint_elem->GetNextElement("East"))
    {
      auto east_joint_name = east_joint_elem->Get<std::string>();
      auto east_joint = _model->GetJoint(east_joint_name);
      if (!east_joint)
      {
        RCLCPP_ERROR(impl_->ros_node_->get_logger(), "Joint [%s] not found, plugin will not work.", east_joint_name.c_str());
        impl_->ros_node_.reset();
        return;
      }
      
      east_joint->SetParam("fmax", 0, impl_->max_wheel_torque_);
      east_joints.push_back(east_joint);
    }

    unsigned int index;
    for (index = 0; index < impl_->num_wheel_pairs_; ++index)
    {
      impl_->joints_.push_back(north_joints[index]);
      impl_->joints_.push_back(west_joints[index]);
      impl_->joints_.push_back(south_joints[index]);
      impl_->joints_.push_back(east_joints[index]);
    }
    
    index = 0;
    impl_->wheel_w_separation_.assign(impl_->num_wheel_pairs_, 0.34);
    for (auto wheel_w_separation = _sdf->GetElement("WheelSeparationW"); 
        wheel_w_separation != nullptr;
        wheel_w_separation = wheel_w_separation->GetNextElement("WheelSeparationW"))
    {
      if (index >= impl_->num_wheel_pairs_)
      {
        RCLCPP_WARN(impl_->ros_node_->get_logger(), "Ignoring rest of specified <WheelSeparationW>");
        break;
      }
      
      impl_->wheel_w_separation_[index] = wheel_w_separation->Get<double>();
      RCLCPP_INFO(impl_->ros_node_->get_logger(), "Wheel pair %i separation width set to [%fm]", index + 1, impl_->wheel_w_separation_[index]);
      index++;
    }

    index = 0;
    impl_->wheel_l_separation_.assign(impl_->num_wheel_pairs_, 0.34);
    for (auto wheel_l_separation = _sdf->GetElement("WheelSeparationL");
        wheel_l_separation != nullptr; 
        wheel_l_separation = wheel_l_separation->GetNextElement("WheelSeparationL"))
    {
      if (index >= impl_->num_wheel_pairs_)
      {
        RCLCPP_WARN(impl_->ros_node_->get_logger(), "Ignoring rest of specified <WheelSeparationL>");
        break;
      }
      
      impl_->wheel_l_separation_[index] = wheel_l_separation->Get<double>();
      RCLCPP_INFO(impl_->ros_node_->get_logger(), "Wheel pair %i separation legth set to [%fm]", index + 1, impl_->wheel_l_separation_[index]);
      index++;
    }
    
    index = 0;
    impl_->wheel_diameter_.assign(impl_->num_wheel_pairs_, 0.15);
    for (auto wheel_diameter = _sdf->GetElement("wheelDiameter"); 
        wheel_diameter != nullptr; 
        wheel_diameter = wheel_diameter->GetNextElement("wheelDiameter"))
    {
      if (index >= impl_->num_wheel_pairs_)
      {
        RCLCPP_WARN(impl_->ros_node_->get_logger(), "Ignoring rest of specified <wheelDiameter>");
        break;
      }
      
      impl_->wheel_diameter_[index] = wheel_diameter->Get<double>();
      RCLCPP_INFO(impl_->ros_node_->get_logger(), "Wheel pair %i diameter set to [%fm]", index + 1, impl_->wheel_diameter_[index]);
      index++;
    }
    
    impl_->wheel_speed_instr_.assign(4 * impl_->num_wheel_pairs_, 0);
    impl_->desired_wheel_speed_.assign(4 * impl_->num_wheel_pairs_, 0);
    
    // Update rate
    auto update_rate = _sdf->Get<double>("odometryRate", 20.0).first;
    if (update_rate > 0.0)
    {
      impl_->update_period_ = 1.0 / update_rate;
    }
    else
    {
      impl_->update_period_ = 0.0;
    }

    impl_->cmd_topic_ = _sdf->Get<std::string>("commandTopic", "cmd_vel").first;
    impl_->last_update_time_ = _model->GetWorld()->SimTime();
    impl_->cmd_vel_sub_ = impl_->ros_node_->create_subscription<geometry_msgs::msg::Twist>(impl_->cmd_topic_, qos.get_subscription_qos(impl_->cmd_topic_, rclcpp::QoS(1)),
                                                                                           std::bind(&GazeboOmniRosDrivePrivate::OnCmdVel, impl_.get(), std::placeholders::_1));
    
    RCLCPP_INFO(impl_->ros_node_->get_logger(), "Subscribed to [%s]", impl_->cmd_vel_sub_->get_topic_name());
    // Odometry
    impl_->odometry_frame_ = _sdf->Get<std::string>("odometryFrame", "odom").first;
    impl_->robot_base_frame_ = _sdf->Get<std::string>("robotBaseFrame", "base_footprint").first;
    
    // Advertise odometry topic
    impl_->publish_odom_ = _sdf->Get<bool>("publishOdom", false).first;
    if(impl_->publish_odom_)
    {
      impl_->odometry_topic_ = _sdf->Get<std::string>("odometryTopic", "odom").first;
      impl_->odometry_pub_ = impl_->ros_node_->create_publisher<nav_msgs::msg::Odometry>(impl_->odometry_topic_, qos.get_publisher_qos(impl_->odometry_topic_, rclcpp::QoS(1)));
      RCLCPP_INFO(impl_->ros_node_->get_logger(), "Advertise odometry on [%s]", impl_->odometry_pub_->get_topic_name());
    }
    
    // Create TF broadcaster if needed
    impl_->publish_wheel_tf_ = _sdf->Get<bool>("publishWheelTF", false).first;
    impl_->publish_odom_tf_ = _sdf->Get<bool>("publishOdomTF", false).first;
    if(impl_->publish_wheel_tf_ || impl_->publish_odom_tf_)
    {
      impl_->transform_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(impl_->ros_node_);
      if(impl_->publish_odom_tf_)
      {
        RCLCPP_INFO(impl_->ros_node_->get_logger(), "Publishing odom transforms between [%s] and [%s]", impl_->odometry_frame_.c_str(),
                                                                                                        impl_->robot_base_frame_.c_str());
      }
      
      for (index = 0; index < impl_->num_wheel_pairs_; ++index)
      {
        if(impl_->publish_wheel_tf_)
        {
          RCLCPP_INFO(impl_->ros_node_->get_logger(), "Publishing wheel transforms between [%s], [%s] , [%s] , [%s], [%s]",
                                                       impl_->robot_base_frame_.c_str(),
                                                       impl_->joints_[2 * index + GazeboOmniRosDrivePrivate::NORTH]->GetName().c_str(),
                                                       impl_->joints_[2 * index + GazeboOmniRosDrivePrivate::WEST]->GetName().c_str(),
                                                       impl_->joints_[2 * index + GazeboOmniRosDrivePrivate::SOUTH]->GetName().c_str(),
                                                       impl_->joints_[2 * index + GazeboOmniRosDrivePrivate::EAST]->GetName().c_str());
        }
      }
    }

    impl_->isRollerModel_ = _sdf->Get<bool>("isRollerModel", true).first;
    impl_->covariance_[0] = _sdf->Get<double>("covariance_x", 0.00001).first;
    impl_->covariance_[1] = _sdf->Get<double>("covariance_y", 0.00001).first;
    impl_->covariance_[2] = _sdf->Get<double>("covariance_yaw", 0.001).first;
    
    // Listen to the update event (broadcast every simulation iteration)
    impl_->update_connection_ = gazebo::event::Events::ConnectWorldUpdateBegin(std::bind(&GazeboOmniRosDrivePrivate::OnUpdate, impl_.get(), std::placeholders::_1));
  }
  
  void GazeboOmniRosDrive::Reset()
  {
    impl_->last_update_time_ = impl_->joints_[GazeboOmniRosDrivePrivate::NORTH]->GetWorld()->SimTime();
    for(unsigned int i = 0; i < impl_->num_wheel_pairs_; ++i)
    {
      if (impl_->joints_[2 * i + GazeboOmniRosDrivePrivate::NORTH] && impl_->joints_[2 * i + GazeboOmniRosDrivePrivate::WEST] &&
          impl_->joints_[2 * i + GazeboOmniRosDrivePrivate::SOUTH] && impl_->joints_[2 * i + GazeboOmniRosDrivePrivate::EAST])
      {
        impl_->joints_[2 * i + GazeboOmniRosDrivePrivate::NORTH]->SetParam("fmax", 0, impl_->max_wheel_torque_);
        impl_->joints_[2 * i + GazeboOmniRosDrivePrivate::WEST]->SetParam("fmax", 0, impl_->max_wheel_torque_);
        impl_->joints_[2 * i + GazeboOmniRosDrivePrivate::SOUTH]->SetParam("fmax", 0, impl_->max_wheel_torque_);
        impl_->joints_[2 * i + GazeboOmniRosDrivePrivate::EAST]->SetParam("fmax", 0, impl_->max_wheel_torque_);
      }
    }
    
    impl_->pose_encoder_.x = 0;
    impl_->pose_encoder_.y = 0;
    impl_->pose_encoder_.theta = 0;
    impl_->target_x_ = 0;
    impl_->target_rot_ = 0;
  }

  void GazeboOmniRosDrivePrivate::OnUpdate(const gazebo::common::UpdateInfo & _info)
  {
  #ifdef IGN_PROFILER_ENABLE
    IGN_PROFILE("GazeboRosDiffDrivePrivate::OnUpdate");
  #endif
    
    double seconds_since_last_update = (_info.simTime - last_update_time_).Double();
    if(seconds_since_last_update < update_period_)
    {
      return;
    }

  #ifdef IGN_PROFILER_ENABLE
    IGN_PROFILE_END();
    IGN_PROFILE_BEGIN("PublishOdometryMsg");
  #endif
    if(publish_odom_)
    {
      PublishOdometryMsg(_info.simTime);
    }
    #ifdef IGN_PROFILER_ENABLE
    IGN_PROFILE_END();
    IGN_PROFILE_BEGIN("PublishWheelsTf");
  #endif
    if(publish_wheel_tf_)
    {
      PublishWheelsTf(_info.simTime);
    }
  #ifdef IGN_PROFILER_ENABLE
    IGN_PROFILE_END();
    IGN_PROFILE_BEGIN("PublishOdometryTf");
  #endif
    if(publish_odom_tf_)
    {
      PublishOdometryTf(_info.simTime);
    }
  #ifdef IGN_PROFILER_ENABLE
    IGN_PROFILE_END();
    IGN_PROFILE_BEGIN("UpdateWheelVelocities");
  #endif
  // Update robot in case new velocities have been requested
  UpdateWheelVelocities();
  #ifdef IGN_PROFILER_ENABLE
    IGN_PROFILE_END();
  #endif
    // Current speed
    std::vector<double> current_speed(4 * num_wheel_pairs_);
    for(unsigned int i = 0; i < num_wheel_pairs_; ++i)
    {
      current_speed[2 * i + NORTH] = joints_[2 * i + NORTH]->GetVelocity(0);
      current_speed[2 * i + WEST] = joints_[2 * i + WEST]->GetVelocity(0);
      current_speed[2 * i + SOUTH] = joints_[2 * i + SOUTH]->GetVelocity(0);
      current_speed[2 * i + EAST] = joints_[2 * i + EAST]->GetVelocity(0);
    }

    // If max_accel == 0, or target speed is reached
    for(unsigned int i = 0; i < num_wheel_pairs_; ++i)
    {
      if(max_wheel_accel_ == 0 ||
        ((fabs(desired_wheel_speed_[2 * i + NORTH] - current_speed[2 * i + NORTH]) < 0.01) &&
         (fabs(desired_wheel_speed_[2 * i + WEST] - current_speed[2 * i + WEST]) < 0.01) &&
         (fabs(desired_wheel_speed_[2 * i + SOUTH] - current_speed[2 * i + SOUTH]) < 0.01) &&
         (fabs(desired_wheel_speed_[2 * i + EAST] - current_speed[2 * i + EAST]) < 0.01)))
      {
        joints_[2 * i + NORTH]->SetParam("vel", 0, desired_wheel_speed_[2 * i + NORTH]);
        joints_[2 * i + WEST]->SetParam("vel", 0, desired_wheel_speed_[2 * i + WEST]);
        joints_[2 * i + SOUTH]->SetParam("vel", 0, desired_wheel_speed_[2 * i + SOUTH]);
        joints_[2 * i + EAST]->SetParam("vel", 0, desired_wheel_speed_[2 * i + EAST]);
      }
      else
      {
        if(desired_wheel_speed_[2 * i + NORTH] >= current_speed[2 * i + NORTH])
        {
          wheel_speed_instr_[2 * i + NORTH] += fmin(desired_wheel_speed_[2 * i + NORTH] - current_speed[2 * i + NORTH], max_wheel_accel_ * seconds_since_last_update);
        }
        else
        {
          wheel_speed_instr_[2 * i + NORTH] += fmax(desired_wheel_speed_[2 * i + NORTH] - current_speed[2 * i + NORTH], -max_wheel_accel_ * seconds_since_last_update);
        }

        if(desired_wheel_speed_[2 * i + WEST] >= current_speed[2 * i + WEST])
        {
          wheel_speed_instr_[2 * i + WEST] += fmin(desired_wheel_speed_[2 * i + WEST] - current_speed[2 * i + WEST], max_wheel_accel_ * seconds_since_last_update);
        }
        else
        {
          wheel_speed_instr_[2 * i + WEST] += fmax(desired_wheel_speed_[2 * i + WEST] - current_speed[2 * i + WEST], -max_wheel_accel_ * seconds_since_last_update);
        }

        if (desired_wheel_speed_[2 * i + SOUTH] > current_speed[2 * i + SOUTH])
        {
          wheel_speed_instr_[2 * i + SOUTH] += fmin(desired_wheel_speed_[2 * i + SOUTH] - current_speed[2 * i + SOUTH], max_wheel_accel_ * seconds_since_last_update);
        }
        else
        {
          wheel_speed_instr_[2 * i + SOUTH] += fmax(desired_wheel_speed_[2 * i + SOUTH] - current_speed[2 * i + SOUTH], -max_wheel_accel_ * seconds_since_last_update);
        }

        if (desired_wheel_speed_[2 * i + EAST] > current_speed[2 * i + EAST])
        {
          wheel_speed_instr_[2 * i + EAST] += fmin(desired_wheel_speed_[2 * i + EAST] - current_speed[2 * i + EAST], max_wheel_accel_ * seconds_since_last_update);
        }
        else
        {
          wheel_speed_instr_[2 * i + EAST] += fmax(desired_wheel_speed_[2 * i + EAST] - current_speed[2 * i + EAST], -max_wheel_accel_ * seconds_since_last_update);
        }

        joints_[2 * i + NORTH]->SetParam("vel", 0, wheel_speed_instr_[2 * i + NORTH]);
        joints_[2 * i + WEST]->SetParam("vel", 0, wheel_speed_instr_[2 * i + WEST]);
        joints_[2 * i + SOUTH]->SetParam("vel", 0, wheel_speed_instr_[2 * i + SOUTH]);
        joints_[2 * i + EAST]->SetParam("vel", 0, wheel_speed_instr_[2 * i + EAST]);
      }
    }

    if(!isRollerModel_)
    {
      ignition::math::Pose3d pose = model_->WorldPose();
      float yaw = pose.Rot().Yaw();
      calkinematics(cal_LineVel_);
      model_->SetLinearVel(ignition::math::Vector3d(cal_LineVel_.vel_x * cosf(yaw) - cal_LineVel_.vel_y * sinf(yaw), 
                                                     cal_LineVel_.vel_y * cosf(yaw) + cal_LineVel_.vel_x * sinf(yaw), 0));
      model_->SetAngularVel(ignition::math::Vector3d(0, 0, cal_LineVel_.vel_th));
    }

    last_update_time_ = _info.simTime;
  }

  void GazeboOmniRosDrivePrivate::UpdateWheelVelocities()
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);

    double vx = target_x_;
    double vy = target_y_;
    double va = target_rot_;

    for (unsigned int i = 0; i < num_wheel_pairs_; ++i)
    {
      desired_wheel_speed_[2 * i + NORTH] = (vx + vy - va * (wheel_w_separation_[i] + wheel_l_separation_[i]) / 2.0) / wheel_diameter_[i] * 2;
      desired_wheel_speed_[2 * i + WEST] = (vx - vy - va * (wheel_w_separation_[i] + wheel_l_separation_[i]) / 2.0) / wheel_diameter_[i] * 2;
      desired_wheel_speed_[2 * i + SOUTH] = (vx + vy + va * (wheel_w_separation_[i] + wheel_l_separation_[i]) / 2.0) / wheel_diameter_[i] * 2;
      desired_wheel_speed_[2 * i + EAST] = (vx - vy + va * (wheel_w_separation_[i] + wheel_l_separation_[i]) / 2.0) / wheel_diameter_[i] * 2;
    }
  }

  void GazeboOmniRosDrivePrivate::OnCmdVel(const geometry_msgs::msg::Twist::SharedPtr _msg)
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    target_x_ = _msg->linear.x;
    target_y_ = _msg->linear.y;
    target_rot_ = _msg->angular.z;
  }

  void GazeboOmniRosDrivePrivate::PublishOdometryTf(const gazebo::common::Time & _current_time)
  {
    geometry_msgs::msg::TransformStamped msg;
    msg.header.stamp = gazebo_ros::Convert<builtin_interfaces::msg::Time>(_current_time);
    msg.header.frame_id = odometry_frame_;
    msg.child_frame_id = robot_base_frame_;
    msg.transform.translation =
    gazebo_ros::Convert<geometry_msgs::msg::Vector3>(odom_.pose.pose.position);
    msg.transform.rotation = odom_.pose.pose.orientation;

    transform_broadcaster_->sendTransform(msg);
  }

  void GazeboOmniRosDrivePrivate::PublishWheelsTf(const gazebo::common::Time & _current_time)
  {
    for (unsigned int i = 0; i < 4 * num_wheel_pairs_; ++i)
    {
      auto pose_wheel = joints_[i]->GetChild()->RelativePose();

      geometry_msgs::msg::TransformStamped msg;
      msg.header.stamp = gazebo_ros::Convert<builtin_interfaces::msg::Time>(_current_time);
      msg.header.frame_id = joints_[i]->GetParent()->GetName();
      msg.child_frame_id = joints_[i]->GetChild()->GetName();
      msg.transform.translation = gazebo_ros::Convert<geometry_msgs::msg::Vector3>(pose_wheel.Pos());
      msg.transform.rotation = gazebo_ros::Convert<geometry_msgs::msg::Quaternion>(pose_wheel.Rot());

      transform_broadcaster_->sendTransform(msg);
    }
  }

  void GazeboOmniRosDrivePrivate::PublishOdometryMsg(const gazebo::common::Time & _current_time)
  {
    auto pose = model_->WorldPose();
    odom_.pose.pose.position = gazebo_ros::Convert<geometry_msgs::msg::Point>(pose.Pos());
    odom_.pose.pose.orientation = gazebo_ros::Convert<geometry_msgs::msg::Quaternion>(pose.Rot());

    // Get velocity in odom frame
    auto linear = model_->WorldLinearVel();
    odom_.twist.twist.angular.z = model_->WorldAngularVel().Z();

    // Convert velocity to child_frame_id(aka base_footprint)
    float yaw = pose.Rot().Yaw();
    odom_.twist.twist.linear.x = cosf(yaw) * linear.X() + sinf(yaw) * linear.Y();
    odom_.twist.twist.linear.y = cosf(yaw) * linear.Y() - sinf(yaw) * linear.X();
    // Set covariance
    odom_.pose.covariance[0] = covariance_[0];
    odom_.pose.covariance[7] = covariance_[1];
    odom_.pose.covariance[14] = 1000000000000.0;
    odom_.pose.covariance[21] = 1000000000000.0;
    odom_.pose.covariance[28] = 1000000000000.0;
    odom_.pose.covariance[35] = covariance_[2];

    odom_.twist.covariance[0] = covariance_[0];
    odom_.twist.covariance[7] = covariance_[1];
    odom_.twist.covariance[14] = 1000000000000.0;
    odom_.twist.covariance[21] = 1000000000000.0;
    odom_.twist.covariance[28] = 1000000000000.0;
    odom_.twist.covariance[35] = covariance_[2];

    // Set header
    odom_.header.frame_id = odometry_frame_;
    odom_.child_frame_id = robot_base_frame_;
    odom_.header.stamp = gazebo_ros::Convert<builtin_interfaces::msg::Time>(_current_time);

    // Publish
    odometry_pub_->publish(odom_);
  }

  void GazeboOmniRosDrivePrivate::calkinematics(linear_vel &line_vel)
  {
    double wheel_encoder[4];
    wheel_encoder[NORTH] = joints_[NORTH]->GetVelocity(0);
    wheel_encoder[WEST] = joints_[WEST]->GetVelocity(0);
    wheel_encoder[SOUTH] = joints_[SOUTH]->GetVelocity(0);
    wheel_encoder[EAST] = joints_[EAST]->GetVelocity(0);

    double l = 1.0 / (2 * (wheel_w_separation_[0] + wheel_l_separation_[0]));

    line_vel.vel_x = (wheel_encoder[NORTH] + wheel_encoder[WEST] + wheel_encoder[SOUTH] + wheel_encoder[EAST]) * wheel_diameter_[0] / 2;
    line_vel.vel_y = (wheel_encoder[NORTH] - wheel_encoder[WEST] + wheel_encoder[SOUTH] - wheel_encoder[EAST]) * wheel_diameter_[0] / 2;
    line_vel.vel_th = (-wheel_encoder[NORTH] - wheel_encoder[WEST] + wheel_encoder[SOUTH] + wheel_encoder[EAST]) * l * wheel_diameter_[0] / 2;
  }

  GZ_REGISTER_MODEL_PLUGIN(GazeboOmniRosDrive)
}  // namespace gazebo_plugins