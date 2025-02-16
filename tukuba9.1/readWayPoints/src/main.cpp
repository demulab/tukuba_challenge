/*      read waypoints						*/
/*      author shohei TAKESAKO				*/
/*      last update : 2012.11.14			*/


#include <iostream>
#include "MyMethod.h"
#include <ros/ros.h>
#include <std_msgs/UInt32.h>
#include <geometry_msgs/PoseStamped.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>
#include "readWayPoints/AllWayPoints.h"


using namespace std;
#define HZ 1

int nowWayPointNum=0;

#define WAYPOINT_PARAMETER_COUT

//コールバック関数
//現在のウェイポイント番号を取得する関数
void wayPointCallback(const std_msgs::UInt32 nowWp)
{
	nowWayPointNum = nowWp.data;
	//cout<< nowWp.data <<endl;
}

//何番目のウェイポイントから始めるかの確認関数
//小森谷先輩作成のkit_waypoint_start.cppより移植
int setStartWayPointNum(int wpMax)
{
	bool req = false;
	int ret=0;
	std::cout << " ------- Setting start point --------- " << endl;
	while(!req){
		cout << endl << "Please enter 'WayPoint' >> ";
		cin >> ret;
		while(!cin){
			cout << "Set waypoint error !!" << endl;
			cin.clear();
			cin.ignore(INT_MAX,'\n');
			cin >> ret;
		}
		if(ret > wpMax){
			cout << "Set waypoint error !!" << endl;
			req = false;
		}
		else{
			req = true;
		}
		sleep(0.1);
	}
	return ret;
}

//ウェイポイントベースで走行させるかの確認
//小森谷先輩作成のkit_waypoint_start.cppより移植
bool StartRobot()
{
	char cmd[50];
	while(1){
		cout<<"Robot Navigation Start"<<endl;;
		cout<<"Ready ?? input 's' -> start"<<endl;
		std::cin.getline(cmd,50);
		if(cmd[0] != 's'){
			cout<<"unknown command :"<<cmd <<endl;
			continue;
		}

		else if(cmd[0] == 's'){
			cout<<"----------------------"<<endl;
			cout<<"----------------------"<<endl;
			cout<<"    Robot Start !!    "<<endl;
			cout<<"----------------------"<<endl;
			cout<<"----------------------"<<endl;
			return true;
		}
	}
}

//コールバック関数
////現在のウェイポイント番号を取得する関数
std::vector<pose> all_way_points;
bool sendAllWayPoints(readWayPoints::AllWayPoints::Request &request,
		readWayPoints::AllWayPoints::Response &response)
{
	response.waypoints.resize(all_way_points.size());
	for(int i = 0, s = all_way_points.size(); i < s; i++) {
		response.waypoints[i].position.x = all_way_points[i].x;
		response.waypoints[i].position.y = all_way_points[i].y;

		geometry_msgs::Quaternion q = tf::createQuaternionMsgFromYaw(all_way_points[i].theta);
		response.waypoints[i].orientation = q;
	}
	return true;
}



int main(int argc, char **argv)
{
	//ROSの初期化
	ros::init(argc, argv, "ReadWayPoints");
	ros::NodeHandle n;
	ros::NodeHandle nh("~");
	ros::NodeHandle all_way_points_nh("~");

	//Publishの設定
	geometry_msgs::PoseStamped nextWayPoint;
	ros::Publisher pubNextWayPoints = n.advertise<geometry_msgs::PoseStamped>("nextWayPoint", 100);
	std::string file_pass;
	nh.param("file_pass", file_pass, std::string(""));
	cout<< "file pass --->" << file_pass.c_str() <<endl;

	//Subscribeの設定
	ros::Subscriber nowWp = n.subscribe("nowWayPointNum", 100, wayPointCallback);

	// Seviceの設定
	ros::ServiceServer all_way_points_server = all_way_points_nh.advertiseService("AllWayPoints", sendAllWayPoints);
	
	//変数の宣言
	pose *wayPoints;//ウェイポイントデータ群
	int nextWp=0;//目標とするウェイポイント番号

	//waypointの読み込み
	FILE *fp;
	int wayPointNumMax=0;
	//std::string f_name("waypoint/fmt_challenge.txt");
	std::string f_name("waypoint/fmt_test.txt");
	file_pass += f_name;
	if ((fp = fopen(file_pass.c_str(), "r")) == NULL)
	{
		std::cout<< "file open error!! : KIT.txt (Can't find waypoints data)"  << std::endl;
		exit(-1);
	}
	else
	{
		double num=0, x=0, y=0, theta=0;
		while(1)
		{
			if( fscanf(fp, "%lf,%lf,%lf,%lf,",&num, &x, &y, &theta) == EOF)
			{   break;  }
			else
			{   wayPointNumMax++;  }
		}
		rewind(fp);
		wayPoints = new pose[wayPointNumMax];
		memset(wayPoints, 0, wayPointNumMax * sizeof(pose));

		for(int i=0; i<wayPointNumMax; i++)
		{
			if( fscanf(fp, "%lf,%lf,%lf,%lf,",&num, &wayPoints[i].x, &wayPoints[i].y, &wayPoints[i].theta) == EOF)
			{
				std::cout<< "file read error!! : waypoint.txt"  << std::endl;
				break;
			}
			all_way_points.push_back(wayPoints[i]);
#ifdef WAYPOINT_PARAMETER_COUT
			std::cout<< "--WayPoint--------  " << i  << "  --"<< std::endl;
			std::cout<< "x = " << wayPoints[i].x  << std::endl;
			std::cout<< "y = " << wayPoints[i].y  << std::endl;
			std::cout<< "theta = " << wayPoints[i].theta  << std::endl;
#endif
		}
	}
	fclose(fp);

	nextWp = setStartWayPointNum(wayPointNumMax)+1;
	
	//開始の確認
	StartRobot();

	//最初に目指すウェイポイントをpublishする
	nextWayPoint.header.frame_id = "next_waypoint";

	nextWayPoint.pose.position.x = wayPoints[nextWp].x;
	nextWayPoint.pose.position.y = wayPoints[nextWp].y;
	nextWayPoint.pose.position.z = nextWp;
	//geometry_msgs::Quaternion wp_quat = tf::createQuaternionMsgFromYaw(-wayPoints[nextWp].theta);
	geometry_msgs::Quaternion wp_quat = tf::createQuaternionMsgFromYaw(wayPoints[nextWp].theta);
	nextWayPoint.pose.orientation = wp_quat;

	pubNextWayPoints.publish(nextWayPoint);
	std::cout<< "--nextWayPoint-------- " << nextWp  << "  --"<< std::endl;
	std::cout<< "x = " << wayPoints[nextWp].x  << std::endl;
	std::cout<< "y = " << wayPoints[nextWp].y  << std::endl;
	std::cout<< "theta = " << wayPoints[nextWp].theta  << std::endl;
	std::cout<< "wayPointNumMax = " << wayPointNumMax << std::endl;

	//繰り返し処理
	while(ros::ok())
	{
		nextWayPoint.header.seq = 1;
		cout<< "nextWp = " << nextWp <<endl;
		cout<< "nowWayPointNum = " << nowWayPointNum <<endl;
		if(nextWp == nowWayPointNum)
		{	nextWp++;	}
        else if(nextWp < nowWayPointNum){
            nextWp = nowWayPointNum+1;
        }
		if(nextWp >= wayPointNumMax)
		{	
			nextWayPoint.pose.position.x = wayPoints[wayPointNumMax-1].x;
			nextWayPoint.pose.position.y = wayPoints[wayPointNumMax-1].y;
			nextWayPoint.pose.position.z = -1;
			wp_quat = tf::createQuaternionMsgFromYaw(wayPoints[wayPointNumMax-1].theta*(M_PI/180.0));
			nextWayPoint.pose.orientation = wp_quat;
			
			if(nowWayPointNum < 0)
			{	break;	}
		}
		else
		{ 
			nextWayPoint.pose.position.x = wayPoints[nextWp].x;
			nextWayPoint.pose.position.y = wayPoints[nextWp].y;
			nextWayPoint.pose.position.z = nextWp;
			wp_quat = tf::createQuaternionMsgFromYaw(wayPoints[nextWp].theta*(M_PI/180.0));
			nextWayPoint.pose.orientation = wp_quat;

			std::cout<< "--nextWp-------- " << nextWp  << "  --"<< std::endl;
			std::cout<< "x = " << wayPoints[nextWp].x  << std::endl;
			std::cout<< "y = " << wayPoints[nextWp].y  << std::endl;
			std::cout<< "theta = " << wayPoints[nextWp].theta  << std::endl;
		}
		pubNextWayPoints.publish(nextWayPoint);
		ros::Rate loop_rate(HZ);
		loop_rate.sleep();
		ros::spinOnce();
	}
	ROS_INFO("readWayPoint.pkg is end.");
	return 0;
}

