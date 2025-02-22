// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4A_ROS_DEVICE_H
#define K4A_ROS_DEVICE_H

// System headers
//
#include <atomic>
#include <mutex>
#include <thread>

// Library headers
//
#include <azure_kinect_ros_driver/AzureKinectParamsConfig.h>
#include <dynamic_reconfigure/server.h>
#include <image_transport/image_transport.h>
#include <k4a/k4a.h>
#include <ros/ros.h>
#include <sensor_msgs/CameraInfo.h>
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/Temperature.h>
#include <k4a/k4a.hpp>
#include <camera_info_manager/camera_info_manager.h>

// Project headers
//
#include "azure_kinect_ros_driver/k4a_calibration_transform_data.h"
#include "azure_kinect_ros_driver/k4a_ros_device_params.h"

class K4AROSDevice
{
 public:
  K4AROSDevice(const ros::NodeHandle& n = ros::NodeHandle(), const ros::NodeHandle& p = ros::NodeHandle("~"));

  ~K4AROSDevice();

  k4a_result_t startCameras();

  void stopCameras();

  k4a_result_t getDepthFrame(const k4a::capture& capture, sensor_msgs::ImagePtr& depth_frame, bool rectified);

  k4a_result_t getPointCloud(const k4a::capture& capture, sensor_msgs::PointCloud2Ptr& point_cloud);

  k4a_result_t getRgbPointCloudInRgbFrame(const k4a::capture& capture, sensor_msgs::PointCloud2Ptr& point_cloud);
  k4a_result_t getRgbPointCloudInDepthFrame(const k4a::capture& capture, sensor_msgs::PointCloud2Ptr& point_cloud);

  k4a_result_t getRbgFrame(const k4a::capture& capture, sensor_msgs::ImagePtr& rgb_frame, bool rectified);

  k4a_result_t getIrFrame(const k4a::capture& capture, sensor_msgs::ImagePtr& ir_image);

 private:
  k4a_result_t renderBGRA32ToROS(sensor_msgs::ImagePtr& rgb_frame, k4a::image& k4a_bgra_frame);
  k4a_result_t renderDepthToROS(sensor_msgs::ImagePtr& depth_image, k4a::image& k4a_depth_frame);
  k4a_result_t renderIrToROS(sensor_msgs::ImagePtr& ir_image, k4a::image& k4a_ir_frame);

  k4a_result_t fillPointCloud(const k4a::image& pointcloud_image, sensor_msgs::PointCloud2Ptr& point_cloud);
  k4a_result_t fillColorPointCloud(const k4a::image& pointcloud_image, const k4a::image& color_image,
                                   sensor_msgs::PointCloud2Ptr& point_cloud);

  void framePublisherThread();

  // Gets a timestap from one of the captures images
  std::chrono::microseconds getCaptureTimestamp(const k4a::capture& capture);

  // Converts a k4a_image_t timestamp to a ros::Time object
  ros::Time timestampToROS(const std::chrono::microseconds& k4a_timestamp_us);

  // Converts a k4a_imu_sample_t timestamp to a ros::Time object
  ros::Time timestampToROS(const uint64_t& k4a_timestamp_us);

  // Updates the timestamp offset (stored as start_time_) between the device time and ROS time.
  // This is a low-pass filtered update based on the system time from k4a, which represents the
  // time the message arrived at the USB bus.
  void updateTimestampOffset(const std::chrono::microseconds& k4a_device_timestamp_us,
                             const std::chrono::nanoseconds& k4a_system_timestamp_ns);
  // Make an initial guess based on wall clock. The best we can do when no image timestamps are
  // available.
  void initializeTimestampOffset(const std::chrono::microseconds& k4a_device_timestamp_us);

  // Dynamic reconfigure callback for device
  void reconfigureCallback(azure_kinect_ros_driver::AzureKinectParamsConfig &config, uint32_t level);

  // ROS Node variables
  ros::NodeHandle node_;
  ros::NodeHandle private_node_;
  ros::NodeHandle node_rgb_;
  ros::NodeHandle node_ir_;

  image_transport::ImageTransport image_transport_;

  image_transport::Publisher rgb_raw_publisher_;
  ros::Publisher rgb_raw_camerainfo_publisher_;

  image_transport::Publisher depth_raw_publisher_;
  ros::Publisher depth_raw_camerainfo_publisher_;

  image_transport::Publisher depth_rect_publisher_;
  ros::Publisher depth_rect_camerainfo_publisher_;

  image_transport::Publisher rgb_rect_publisher_;
  ros::Publisher rgb_rect_camerainfo_publisher_;

  image_transport::Publisher ir_raw_publisher_;
  ros::Publisher ir_raw_camerainfo_publisher_;


  ros::Publisher pointcloud_publisher_;

  std::shared_ptr<camera_info_manager::CameraInfoManager> ci_mngr_rgb_, ci_mngr_ir_;

  dynamic_reconfigure::Server<azure_kinect_ros_driver::AzureKinectParamsConfig> reconfigure_server_;

  // Parameters
  K4AROSDeviceParams params_;

  std::string serial_number_;

  // K4A device
  k4a::device k4a_device_;
  K4ACalibrationTransformData calibration_data_;

  std::chrono::nanoseconds device_to_realtime_offset_{0};

  // Thread control
  volatile bool running_;

  // Threads
  std::thread frame_publisher_thread_;
};

void printTimestampDebugMessage(const std::string& name, const ros::Time& timestamp);

#endif  // K4A_ROS_DEVICE_H
