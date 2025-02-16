#include<iostream>
#include"My_method.hpp"
#include"ros/ros.h"
#include"../../fog/srv_gen/cpp/include/fog/FOG.h"
#include"../../wheel/srv_gen/cpp/include/wheel/WHEEL.h"
#include <nav_msgs/Odometry.h>
#include <tf/transform_broadcaster.h>
#include <geometry_msgs/Pose2D.h>
#include <tf/transform_listener.h>
#include "message_filters/subscriber.h"
#include "sensor_msgs/PointCloud.h"
#include <std_msgs/Int64.h>
#include <std_msgs/UInt32.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/PoseStamped.h>
#include <cstdlib>
#include <math.h>
#include <fstream>

using namespace std;

//#define USE_LASER2MAPPING
#define LASER2MAPPING

#define AXLE_L 0.54			//タイヤ間距離(実測値)

#define FOG_ON		//FOGを使用する
#define WHEEL_ON	//アカデミックパックを使用する
#define PUBLISH_ON	//デッドレコニングの結果をPublishする
#define SUBSCRIBE_ON//ナビゲーションからの命令を受信する

//#define WHEEL_COUT	//ホイールの結果を表示
#define ODOM_COUT	//デッドレコニングの値を表示

//デッドレコニングの値を計算する関数
void calc_odometry(double *odom_x, double *odom_y, double *odom_theta, double right_speed, double left_speed, double angle, double time, double *total_distance);
double odom_x = 0.0;
double odom_y = 0.0;
geometry_msgs::Quaternion odom_quat;

//ナビゲーションからの指令値を受信する関数
#ifdef SUBSCRIBE_ON
void naviCallback(const geometry_msgs::Twist navi);
void gps_odomCallback(const nav_msgs::Odometry gps);
void diff_lengthCallback(const std_msgs::UInt32 length);
void next_wpCallback(const geometry_msgs::PoseStamped n_wp);
#endif

double gps_x = 0.0;
double gps_y = 0.0;
geometry_msgs::Quaternion gps_quat;
double gps_vx = 0.0;
double gps_vy = 0.0;
double gps_vz = 0.0;
int gps_flag = 0;
double diff_length = 0.0;
double next_wp = 0.0;
//ナビゲーションからの指令値
double navi_vel_x=0.0;
double navi_vel_y=0.0;
double navi_vel_theta=0.0;

// 移動距離算出用
namespace {
	ofstream odom_out("distance.dat");
}

int main(int argc, char **argv)
{

	ros::init(argc, argv, "Odom");
	
	ros::NodeHandle n;
	//ros::Rate r(100);
	//PUBLISHの設定
#ifdef PUBLISH_ON
	#ifdef LASER2MAPPING
	//Hirai laser_to_mapping用
	nav_msgs::Odometry odom_rad;
	ros::Publisher pub_odom_rad = n.advertise<nav_msgs::Odometry>("odom_rad", 100);
	#endif
	//navigation(move_base)用odometryデータ
	nav_msgs::Odometry odom;
	ros::Publisher pub_odom = n.advertise<nav_msgs::Odometry>("odom", 100);
	//odometryの差分
	nav_msgs::Odometry diff_msg;
	ros::Publisher pub_d_odom = n.advertise<nav_msgs::Odometry>("odom_diff", 100);
	double old_x=0;
	double old_y=0;
	double old_theta=0;

	ros::Time current_time , last_time; 
	current_time = ros::Time::now();
	last_time = ros::Time::now();
	

#endif
	//SUBSCRIBEの設定
#ifdef SUBSCRIBE_ON
	//ros::Subscriber navi = n.subscribe("cmd_vel", 100, naviCallback);
	//ros::Subscriber sub_odom = n.subscribe("gps_odom", 100, gps_odomCallback);
	ros::Subscriber sub_diff_length = n.subscribe("diff_length", 100, diff_lengthCallback);
	ros::Subscriber sub_next_wp = n.subscribe("nextWayPoint", 100, next_wpCallback);
#endif

#ifdef FOG_ON
	ros::ServiceClient client_fog = n.serviceClient<fog::FOG>("FOG");
#endif
#ifdef WHEEL_ON
	ros::ServiceClient client_wheel = n.serviceClient<wheel::WHEEL>("WHEEL");	
#endif

#ifdef FOG_ON
	fog::FOG Fog;
#endif
#ifdef WHEEL_ON
	wheel::WHEEL Wheel;
	double wheelTheta=0;//FOGを使わない場合の姿勢
#endif

	int fog_step = 0;		//FOGの状態を示す変数（ステップごとにFOGに送る信号を変える）
	int last_fog_step = 0;	//前回のFOGの状態を示す変数	
	int wheel_step = 0;		//アカデミックパックの状態を示す変数(ステップごとにアカデミックパックに送る信号を変える)
	//処理速度の計算用
	double start=0;
	double end=0;
	double time=0;
	

	int cout_count = 0;

#ifdef FOG_ON
	Fog.request.order = fog_step;
#endif
#ifdef WHEEL_ON
	Wheel.request.order = wheel_step;
#endif
	start = getETime();

	//デッドレコニングの結果
	double odom_theta = 0.0;
	
	//総距離
	double total_distance = 0.0;

	int zero_count = 0;//FOGの値が0である回数を数えるカウント（カウントが溜まると、もう一度オフセットからやり直す）
	bool fog_start_flag =false;//FOGの準備が完了したことを示すフラグ
#ifndef FOG_ON
	fog_start_flag =true;
#endif

	ros::Rate r(100);

	while(ros::ok())
	{
		usleep(1000*30);
#ifdef ODOM_COUT		
		if(cout_count > 40)
		{
			//std::cout<<"odom_x , odom_y, odom_theta, ditance"<<std::endl;
			//std::cout<<odom_x<<"[m] , "<<odom_y<<"[m] , "<<odom_theta*180.0/M_PI<<"[deg] , "<<total_distance<<"[m]"<<endl;
			//
			cout << "before:" << "\t" << odom_x << "\t" << odom_y << "\t" << odom_theta*180.0/M_PI << "\t" << total_distance << "\t" << endl;
			/*
			ROS_INFO("odom_x = %lf", odom_x);
			ROS_INFO("odom_y = %lf", odom_y);
			ROS_INFO("odom_theta = %lf", odom_theta*180.0/M_PI);
			ROS_INFO("total_distance = %lf", total_distance);
			*/
			cout_count = 0;
		}	
		cout_count++;
#endif
#ifdef FOG_ON
		Fog.request.order = fog_step;
#endif
#ifdef WHEEL_ON	
		Wheel.request.order = wheel_step;
#endif
		//周期の計算
		end = getETime();
		time = fabs(start - end);
		start = getETime();

#ifdef FOG_ON
		if(client_fog.call(Fog))
		{
#endif
#ifdef WHEEL_ON
			if(client_wheel.call(Wheel))
			{
#endif
#ifdef FOG_ON
				//FOGとのやりとり
				if((int)Fog.response.response_header == 0x02)
				{
					if(Fog.response.theta == 0)
					{	
						zero_count++;
						if((zero_count > 150)&&(fog_start_flag == false))
						{	fog_step = 0;	zero_count = 0;	}
					}
					else
					{
						fog_start_flag = true;
					}
				}
				if((int)Fog.response.response_header == 0x06)
				{
					last_fog_step = fog_step;
					if(fog_step == 1)
					{
						std::cout << "オフセット中です。ロボットに触れないでください" << std::endl;;
					}
					if(fog_step<3)
					{	fog_step++;	}

				}
#endif
				//アカデミックパックとのやりとり
#ifdef WHEEL_ON
				if((int)Wheel.response.state == 0x02)
				{
#ifdef WHEEL_COUT
					ROS_INFO("right_wheel = %lf", (double)Wheel.response.right_wheel_speed);
					ROS_INFO("left_wheel = %lf", (double)Wheel.response.left_wheel_speed);
#endif
				}
				else if((int)Wheel.response.state == 0x01)
				{
					if(wheel_step < 1)
					{	wheel_step++;	}
				}
#endif

#ifdef WHEEL_ON	
//FOG使用時
#if defined FOG_ON
				//デッドレコニングの計算を行う
				calc_odometry(&odom_x, &odom_y, &odom_theta, 
					(double)Wheel.response.right_wheel_speed, (double)Wheel.response.left_wheel_speed, (double)Fog.response.theta, 
					time, &total_distance);				
//エンコーダのみ
#else 
				//左右のエンコーダの差分をとって回転量を計算
				wheelTheta += ((double)Wheel.response.right_wheel_speed*time - (double)Wheel.response.left_wheel_speed*time) / AXLE_L;
	
				wheelTheta = changeAngle(wheelTheta);
				//デッドレコニングの計算を行う
				calc_odometry(&odom_x, &odom_y, &odom_theta, 
					(double)Wheel.response.right_wheel_speed, (double)Wheel.response.left_wheel_speed, wheelTheta, 
					time, &total_distance);				
#endif

#ifdef PUBLISH_ON
				//デッドレコニングの結果をパブリッシュする
				//ROSの座標系に合わせるために、odom_thetaの符号を反転
				//geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(-odom_theta);
				odom_quat = tf::createQuaternionMsgFromYaw(odom_theta);
				odom.header.stamp = current_time;
				//odom.header.stamp = ros::Time::now();
				odom.header.frame_id = "odom";
				odom.child_frame_id = "base_link";
				odom.pose.pose.position.x = odom_x;
				odom.pose.pose.position.y = odom_y;
				odom.pose.pose.orientation = odom_quat;
				if(gps_flag == 1 && diff_length < 6){
				//if(gps_flag == 1){
					if((4 <= next_wp && next_wp <= 7) || 13 < next_wp){
						cout << "after:" << "\t" << gps_x << "\t" << gps_y << endl;
						double diff_gps = sqrt(pow(gps_x-odom_x, 2.0) + pow(gps_y-odom_y, 2.0));
						if(11 > diff_gps && diff_gps > 2){
							cout << "gps diff x : " << fabs(gps_x - odom_x) << endl; 
							cout << "gps diff y : " << fabs(gps_y - odom_y) << endl; 
							odom.pose.pose.position.x = gps_x;
							odom.pose.pose.position.y = gps_y;
							odom_x = gps_x;
							odom_y = gps_y;
						
							odom.twist.twist.linear.x = gps_vx;
							odom.twist.twist.linear.y = gps_vy;
							odom.twist.twist.angular.z = gps_vz;
							/*
							if(fabs(gps_quat - odom_quat) > (50*M_PI/180)) {
								cout << "gps diff quat :" << fabs(gps_quat - odom_quat) << endl;
								odom.pose.pose.orientation = gps_quat;
								odom_quat = gps_quat;
							}
							*/
							//odom.pose.pose.orientation = gps_quat;
							//odom_quat = gps_quat;
						}
					}
					gps_flag = 0;
					//cout << "set gps odom data : x is " << gps_x << " y is " << gps_y << " quat is " << gps_quat << endl; 
				}
				
				pub_odom.publish(odom);

				#ifdef LASER2MAPPING
				//Hirai laser_to_mapping用
				odom_rad.header.stamp = current_time;
				odom_rad.header.frame_id = "odom_rad";
				odom_rad.child_frame_id = "base_link";
				odom_rad.pose.pose.position.x = odom_x;
				odom_rad.pose.pose.position.y = odom_y;
				odom_rad.pose.pose.orientation.w = odom_theta * (180/M_PI);
				pub_odom_rad.publish(odom_rad);
				
				//ROS_INFO("odom_rad.pose.pose.position.x = %lf", odom_rad.pose.pose.position.x );
				//ROS_INFO("odom_rad.pose.pose.position.y = %lf", odom_rad.pose.pose.position.y );
				//ROS_INFO("odom_rad.pose.pose.orientation.w = %lf", odom_rad.pose.pose.orientation.w );
				
				#endif
				last_time = current_time;
				
				diff_msg.twist.twist.linear.x = odom_x - old_x;
				diff_msg.twist.twist.linear.y = odom_y - old_y;
				double d_theta = odom_theta - old_theta;
				d_theta = changeAngle(d_theta);
				diff_msg.twist.twist.angular.z = d_theta;
				pub_d_odom.publish(diff_msg);
				//差分を取るために、前回データを保存
				old_x = odom_x;
				old_y = odom_y;
				old_theta = odom_theta;
#endif

#endif
			}
			else
			{
				wheel_step = 0;
				ROS_ERROR("Failed to call service Wheel");
			}
			usleep(10*1000);

			if(fog_step == 3 && last_fog_step == 2)
			{
				usleep(5*1000*1000);
			}
#if defined(FOG_ON) || defined(WHEEL_ON)	
#ifdef SUBSCRIBE_ON
			ros::spinOnce();
#endif
#endif
		}		
#ifdef FOG_ON
		else
		{
			ROS_ERROR("Failed to call service FOG");
		}
#endif
#ifdef FOG_ON
		//r.sleep();
	}
#endif

	odom_out << total_distance << endl;
	return 0;
}


////////////////////////////////////////////////////////////////////////
//移動量[m]&[rad]を算出（x, y, thetaを求める）
///////////////////////////////////////////////////////////////////////
void calc_odometry(double *odom_x, double *odom_y, double *odom_theta, double right_speed, double left_speed, double angle, double time, double *total_distance)
{	
	//右タイヤ移動量
	double now_r = right_speed * time;
	//左タイヤ移動量
	double now_l = left_speed * time;
	//前回からの姿勢の移動量
	double now_theta = 0.0;
	static double last_theta =0.00000;
	static double last_inc_x =0.00000;
	static double last_inc_y =0.00000;
	double inc_x =0.00000;
	double inc_y =0.00000;
	static double last_now_theta =0.00000;

	now_theta = angle - last_theta;
	if((isnan(now_theta) == false)&&(isinf(now_theta) == false))
	{	last_now_theta = now_theta;	}
	else
	{	now_theta = last_now_theta;	}

	//回転半径
	double p = 0;
	//移動距離
	double L = 0;

	if(fabs(now_theta * M_PI) > 0.1)
	{
		p = (AXLE_L/2.0)*((now_r+now_l) / (now_l-now_r));	
		if(now_l - now_r == 0)
		{	p = 0;	}
		if(fabs(now_theta * M_PI) > 0.5)
		{		L = 2 * p * sin(now_theta/2.0);	}
		else
		{	L = p * now_theta;	}
	}
	else
	{	L = (now_r + now_l)/2.0;	}

	inc_x = L * cos(*odom_theta + (now_theta/2.0));
	inc_y = L * sin(*odom_theta + (now_theta/2.0));

	if((isnan(inc_x) == false)&&(isinf(inc_x) == false))
	{	last_inc_x = inc_x;	}
	else
	{	inc_x = last_inc_x;	}

	if((isnan(inc_y) == false)&&(isinf(inc_y) == false))
	{	last_inc_y = inc_y;	}
	else
	{	inc_y = last_inc_y;	}
	
	//移動量算出
	*odom_x += inc_x;
	*odom_y += inc_y;
	*odom_theta += now_theta;
	//総距離算出
	*total_distance += (fabs(inc_x) + fabs(inc_y));

	if(*odom_theta > M_PI)
	{	*odom_theta = *odom_theta - M_PI*2.0;	}
	else if(*odom_theta < -M_PI)
	{	*odom_theta = M_PI*2.0 + *odom_theta;	}
	
	last_theta = *odom_theta;
}

//SUBSCRIBEのコールバック関数
#ifdef SUBSCRIBE_ON
void naviCallback(const geometry_msgs::Twist navi)
{
	//cout << "navi.linear.x  = " << navi.linear.x << endl;
	//cout << "navi.linear.y  = " << navi.linear.y << endl;
	//cout << "navi.linear.z = " << navi.linear.z << endl;
	//cout << "navi.angular.x = " << navi.angular.x << endl;
	//cout << "navi.angular.y = " << navi.angular.y << endl;
	//cout << "navi.angular.z = " << navi.angular.z << endl;
	navi_vel_x = navi.linear.x;
	navi_vel_y = navi.linear.y;
	navi_vel_theta =-(navi.angular.z);
}

void gps_odomCallback(const nav_msgs::Odometry gps)
{
	gps_x = gps.pose.pose.position.x;	
	gps_y = gps.pose.pose.position.y;
	gps_quat = gps.pose.pose.orientation;
	gps_vx = gps.twist.twist.linear.x;
	gps_vy = gps.twist.twist.linear.y;
	gps_vz = gps.twist.twist.angular.z;
	if( !isnan(gps.pose.pose.position.x) ||
		!isnan(gps.pose.pose.position.y) ||
		gps.pose.pose.position.x != 0 ||
		gps.pose.pose.position.y != 0 ){
		//cout << "gps odom : "<< "\t" << gps_x << "\t" << gps_y << endl;
		gps_flag = 1;
	}
}

void diff_lengthCallback(const std_msgs::UInt32 length)
{
	diff_length = length.data;
	//cout << "				diff_length = " << diff_length << endl;
}

void next_wpCallback(const geometry_msgs::PoseStamped n_wp)
{
	next_wp = n_wp.pose.position.z;
}
#endif

