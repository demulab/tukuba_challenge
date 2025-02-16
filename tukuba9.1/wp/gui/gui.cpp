/*** WayPoint Maker 2011 http://demura.net ***/

 //! GUIプログラム
 /*! QTを使ったGUIプログラム

     \file   gui.cpp
     \author Kosei Demura
     \date   2011-11-03
 */

#include "../robot/robot.h"
#include "../include/MyDefine.h"
#include "../include/common.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gui.h"

#include <QWidget>
#include <QScrollArea>
#include <QtGui>
#include <QtGui/QPainter>
#include <QtGui/QStatusBar>
#include <QtGui/QPixmap>
#include <QtGui/QMainWindow>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <iostream>


using namespace std;

extern double GPS_LAT1, GPS_LONG1;

extern RobotClass  *robot;
extern GuiClass    *gui;
extern Ui::MainWindow *ui;
extern MainWindow *gwindow;

//! コンストラクタ
GuiClass::GuiClass(QWidget * iParent, Qt::WindowFlags iFlags)
        : QWidget(iParent, iFlags)
{
    move_coeff = 1.0;

    robot->tukuba = true; // true, or false;
    //robot->map_movable = true; // 地図は移動しない
    map_movable = true;
    scale = 1.0; // 縮尺１倍

    setupEnv();

}


//! デストラクタ
GuiClass::~GuiClass()
{

}


//! 経度をピクセルに変換
int GuiClass::lonToPixel(double lon)
{
    double k2;
    const double *mlon;
    const int *mp;

    if (robot->tukuba)
    {
        mp   = TUKUBA_MAP_POINT_X;
        mlon = TUKUBA_MAP_LON;
    }
    else
    {
        mp   = KIT_MAP_POINT_X;
        mlon = KIT_MAP_LON;
    }

    if (mlon[1] - mlon[0] != 0.0) {
        k2 = (double) (mp[1] - mp[0])/(mlon[1] - mlon[0]);
        return mp[0] + (int) (k2 * (lon - mlon[0]));
    }
    else {
        return -1;
    }
}

//! 緯度をピクセルに変換
int GuiClass::latToPixel(double lat)
{
    double k2;
    const double *mlat;
    const int *mp;

    if (robot->tukuba)
    {
        mp   = TUKUBA_MAP_POINT_Y;
        mlat = TUKUBA_MAP_LAT;

    }
    else
    {
        mp   = KIT_MAP_POINT_Y;
        mlat = KIT_MAP_LAT;
    }

    if (mlat[1] - mlat[0] != 0.0) {
        k2 = (double) (mp[1] - mp[0])/(mlat[1] - mlat[0]);
        return mp[0] + (int) (k2 * (lat - mlat[0]));
    }
    else {
        return -1;
    }
}

//! 経度をメートルに変換
double GuiClass::lonToMeter(double lon)
{
    if (robot->tukuba) return lon * TUKUBA_LON_TO_METER;
    else               return lon * KIT_LON_TO_METER;
}

//! 緯度をメートルに変換
double GuiClass::latToMeter(double lat)
{
    if (robot->tukuba) return lat * TUKUBA_LAT_TO_METER;
    else               return lat * KIT_LAT_TO_METER;
}

//kit.jpg  (136.62664711475372, 36.53195885528937 )  -> (128,  93) pix
//kit.jpg  (136.62862122058868, 36.53078425045223 )  -> (867, 641)
//!  ピクセルから経度への変換
double GuiClass::pixToLon(int pix_x)
{
    double k1;
    const double *mlon;
    const int *mp;


    if (robot->tukuba)
    {
        mp   = TUKUBA_MAP_POINT_X;
        mlon = TUKUBA_MAP_LON;

    }
    else
    {
        mp   = KIT_MAP_POINT_X;
        mlon = KIT_MAP_LON;
    }

    if (mp[1] - mp[0] != 0) {
        k1 = (mlon[1] - mlon[0])/(mp[1] - mp[0]);
        return mlon[0] + k1 * (pix_x - mp[0]);
    }
    else {
        return -1;
    }
}

//!  ピクセルから緯度への変換
double GuiClass::pixToLat(int pix_y)
{
    double k1;
    const double *mlat;
    const  int *mp;

    if (robot->tukuba)
    {
        mp   = TUKUBA_MAP_POINT_Y;
        mlat = TUKUBA_MAP_LAT;
    }
    else
    {
       mp   =  KIT_MAP_POINT_Y;
       mlat =  KIT_MAP_LAT;
    }

    if (mp[1] - mp[0] != 0) {
        k1 = (mlat[1] - mlat[0])/(mp[1] - mp[0]);
        return mlat[0] + k1 * (pix_y - mp[0]);
    }
    else {
        return 0;
    }
}

//! ウインドウサイズを返す
QSize GuiClass::sizeHint() const
{
    return QSize(WINDOW_WIDTH, WINDOW_HEIGHT);
}

//! マウスボタンが押されたときのイベント処理
void GuiClass::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        leftClickPos = event->pos();
        robot->leftClickPos.x   = (double) leftClickPos.x();
        robot->leftClickPos.y   = (double) leftClickPos.y();
        robot->draw_x0 +=  (int) (0.5 * WINDOW_WIDTH) - leftClickPos.x();
        robot->draw_y0 +=  (int) (0.5 * WINDOW_HEIGHT) - leftClickPos.y();
        // clickColor = (pixmap.toImage()).pixel(clickPos);
        // event->accept();
    }
    else
    {
        if (event->button() == Qt::RightButton)
        {
            rightClickPos = event->pos();
            robot->rightClickPos.x  = (double) rightClickPos.x();
            robot->rightClickPos.y  = (double) rightClickPos.y();
            // clickColor = (pixmap.toImage()).pixel(clickPos);
            // event->accept();
        }
    }
}

//! マウスが移動したときのイベント処理
void GuiClass::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton)
    {
        leftClickPos = event->pos();
        robot->draw_x0 +=  (int) (0.5 * WINDOW_WIDTH)  - leftClickPos.x();
        robot->draw_y0 +=  (int) (0.5 * WINDOW_HEIGHT) - leftClickPos.y();
        //robot->clickPos.latitude   = (double) clickPos.y();
        // robot->clickPos.longitude = (double) clickPos.x();
        //event->accept();
    }
}

//! ウェイポイントとロボット情報の表示
void GuiClass::printWayPoints(int sx, int sy, int dx, int dy, QPainter &painter)
{
    //int tw = 350,  th = 400;  // tmp width and height
    QPen pen_red(Qt::red);
    QPen pen_black(Qt::black);
    pen_red.setWidth(3);
    pen_black.setWidth(3);


    //QColor white2(255, 255, 255, 250); // r,g,b, alpha

    //QRect  rect(sx, sy, tw,  th); // posx, posy, width, height

    //if (robot->hide_waypoints_info) painter.fillRect(rect, white2);

    // ウェイポイントの表示
    for (int i = 0; i < WAYPOINTS_NUM; i++)
    {
         QString s1 = "WP"+ QString::number(i);

         if (robot->hide_waypoints_info) {
            // 文字による表示

            // 緯度経度表示
            //QString s2 =  " Lat : " + QString::number(robot->waypoint[i].latitude,  'g',10);
            //QString s3 = ", Lon: "  + QString::number(robot->waypoint[i].longitude, 'g',10);

            // ピクセル表示
            // QString s3 =  " y: " + QString::number(latToPixel(robot->waypoint[i].latitude),  'g',10);
            // QString s2 = ", x: "  + QString::number(lonToPixel(robot->waypoint[i].longitude), 'g',10);

            // メートル表示
            double tmpx = lonToMeter(robot->waypoint[i].longitude - robot->waypoint[START_WAYPOINT].longitude);
            double tmpy = latToMeter(robot->waypoint[i].latitude - robot->waypoint[START_WAYPOINT].latitude);
            if (fabs(tmpx) > 2000) tmpx = NAN;
            if (fabs(tmpy) > 2000) tmpy = NAN;
            QString s3 = "  y:"  + QString::number(tmpy, 'f',2);
            QString s2 = ", x: " + QString::number(tmpx, 'f',2);

            painter.setPen(pen_black);
            if (i < WAYPOINTS_NUM/2) {
                painter.drawText(sx +  10,  sy + 15 + i * 15,  s1+s2 );
                painter.drawText(sx +  120, sy + 15 + i * 15,  s3 );
            }
            else {
                painter.drawText(sx +  190, sy + 15 + (i - (WAYPOINTS_NUM/2)) * 15,  s1+s2 );
                painter.drawText(sx +  290, sy + 15 + (i - (WAYPOINTS_NUM/2)) * 15,  s3 );
            }
        }

        // ウェイポイントを点で表示
        // draw points of the waypoints  should be changed when the map moved
        // however, the value of the waypoint should not be changed
        //int tx = lonToPixel(robot->waypoint[i].longitude) +tmp_x +offset_x;
        //int ty = latToPixel(robot->waypoint[i].latitude)  +tmp_y +offset_y;
        int tx = lonToPixel(robot->waypoint[i].longitude);
        int ty = latToPixel(robot->waypoint[i].latitude);
        painter.setPen(pen_red);

        //int tmp_x2 = scale * tx + move_coeff*tmp_x + offset_x;
        //int tmp_y2 = scale * (ty - 8) + move_coeff*tmp_y + offset_y;

        int tmp_x2, tmp_y2;
        int tmp_offset_x, tmp_offset_y;

        if (robot->tukuba) {
            tmp_offset_x = TUKUBA_OFFSET_X;
            tmp_offset_y = TUKUBA_OFFSET_Y;
        }
        else {
            tmp_offset_x = KIT_OFFSET_X;
            tmp_offset_y = KIT_OFFSET_Y;
        }
        tmp_x2 = robot->draw_x0 + scale * (tx + tmp_offset_x) - dx;
        tmp_y2 = robot->draw_y0 + scale * (ty + tmp_offset_y) - dy; // 点と重ならないように8ピクセル

        painter.drawText(tmp_x2, tmp_y2, s1 );
        painter.drawPoint(QPoint(tmp_x2, tmp_y2));
    }
}

//! ロボット情報の表示
void GuiClass::printInfo(int sx, int sy, QPainter &painter)
{
    QPen pen_red(Qt::red);
    QPen pen_black(Qt::black), pen_black2(Qt::black);
    pen_red.setWidth(3);
    pen_black.setWidth(3);
    pen_black2.setWidth(2);

    QColor white2(255, 255, 255, 250); // r,g,b, alpha

     // 情報表示
    if (robot->hide_info) {
        // Show data
        QFont font;
        font.setPointSize(12);
        painter.setFont(font);

        painter.setPen(pen_black);
        //QRect  rect2(sx, sy-5, 300, 100); // posx, posy, width, height
        //painter.fillRect(rect2, white2);  // 情報表示の背景

        painter.drawText(sx+5,  sy+=5, "Usage");
        painter.drawText(sx+5,  sy+=18, "1. Set OFF");
        painter.drawText(sx+5,  sy+=18, "2. Set WP No.");
        painter.drawText(sx+5,  sy+=18, "3. Set Pose");
        painter.drawText(sx+5,  sy+=18, "4. Move cursor to the waypoint");
        painter.drawText(sx+5,  sy+=18, "5. Click right mouse button");
        painter.drawText(sx+5,  sy+=18, "6. Set ON");
    }
}


//! 位置（航跡）の表示
void GuiClass::drawPos(int x, int y, QPainter &painter, QPolygon &points_saved)
{
    // store the points in the map coordinate
    points_saved.push_back(QPoint(x, y));

    // draw points
    if (scale != 1.0) {
        int count = points_saved.count();
        for (int i = 0; i < count; i++) {
            painter.drawPoint(QPoint(points_saved[i] * scale));
        }
    }
    else {
        painter.drawPoints(points_saved);
    }
}


//! 地図とロボット位置の表示  show the map and the robot position
void GuiClass::drawMapAndRobotPos(int offset_x, int offset_y)
{
    QPainter painter(this);

    QPen pen_red(Qt::red);
    QPen pen_blue(Qt::blue);
    QPen pen_green(Qt::green);
    QPen pen_black(Qt::black);
    QPen pen_black2(Qt::black);
    QPen pen_yellow(Qt::yellow);
    QPen pen_magenta(Qt::magenta);
    QPen pen_cyan(Qt::cyan);

    pen_red.setWidth(2);
    pen_blue.setWidth(2);
    pen_black.setWidth(2);
    pen_black2.setWidth(1);
    pen_green.setWidth(2);
    pen_yellow.setWidth(2);
    pen_cyan.setWidth(2);

    static double tmp_lat0 = robot->waypoint[START_WAYPOINT].latitude;
    static double tmp_lon0 = robot->waypoint[START_WAYPOINT].longitude;
    static double tmp_lat  = robot->waypoint[START_WAYPOINT].latitude;
    static double tmp_lon  = robot->waypoint[START_WAYPOINT].longitude;
    static double old_lat  = tmp_lat; //　１時刻前
    static double old_lon  = tmp_lon;
    // static int old_mode = 0; // 1時刻前のモード(tukuba or KIT)

    // tmp_lat, tmp_lon: ロボットの自己位置を入れる。最終的にはrobot->latitude, robot->longitude
    tmp_lat = robot->getLatitude(); // robot->latitude;
    tmp_lon = robot->getLongitude(); //robot->longitude;

    // 地図のモード切り替え
    int dx, dy; // 地図の移動量（画面座標系）

    //if (robot->map_movable) {
    if (map_movable) {
        // ロボットの現在位置が画面からはみでないように移動
        // デッドレコニングで移動した分(dx, dy)だけ画面座標系の原点draw_x0, draw_y0を移動
        //double tmpx = imcs01->getX();
        //double tmpy = imcs01->getY();
        double tmpx = 0, tmpy = 0;

        dx =   tmpx * METER_TO_PIX;
        dy =   tmpy * METER_TO_PIX; // 画面座標系は上下

        dx *= scale;    // 縮尺の切り替え用
        dy *= scale;
        robot->draw_x0 += dx;
        robot->draw_y0 += dy;
        //cout << "map moveble" << endl;
    }
    else {
        // 地図が固定の場合は、地図を移動しないのでdx, dyを０にする。
        dx = 0;
        dy = 0;
        //cout << "map fixed" << endl;
    }

    old_lat = tmp_lat;
    old_lon = tmp_lon;

    // 地図のオフセット量とマウスのクリックによる移動量で地図の位置を移動する
    // move the map according to the offset of the map and the positions of the left mouse button clicked

    int tmp_x = (int) (0.5 * WINDOW_WIDTH)  - robot->leftClickPos.x;
    int tmp_y = (int) (0.5 * WINDOW_HEIGHT) - robot->leftClickPos.y;

    painter.setWindow(-robot->draw_x0 + dx, -robot->draw_y0 + dy, WINDOW_WIDTH , WINDOW_HEIGHT);
    painter.drawPixmap(0, 0, pixmap); // draw the given pixmap at position (x ,y )


    if (robot->play_log == false) {
        // ログ再生モードでないとき.つまり、リアルタイムでデータ計測時
        /*** パーティクルの表示　***/
        painter.setPen(pen_magenta);

        /*** A100の位置表示　show the position of the robot ***/
        //tmp_x = lonToPixel(a100->getLongitude());
        //tmp_y = latToPixel(a100->getLatitude());

        painter.setPen(pen_green);
        drawPos(tmp_x, tmp_y, painter, gps_points_saved); // changed
        // drawPos(tmp_x+dx, tmp_y+dy, painter, gps_points_saved);

        /*** Ublox-5の位置表示　show the GPS, ublox-5, position of the robot ***/
        int tmp2_x = 0, tmp2_y = 0;
        //int tmp2_x = lonToPixel(ublox5->getLongitude());
        //int tmp2_y = latToPixel(ublox5->getLatitude());
        painter.setPen(pen_green);
        //drawPos(tmp2_x+dx, tmp2_y+dy, painter, gps2_points_saved); // changed
        drawPos(tmp2_x, tmp2_y, painter, gps2_points_saved);


        /*** デッドレコニングの位置表示 show the dead reckoning of the robot ***/
        // x=経度, y=緯度
        // ロボット座標系を絶対座標系（地図座標系）に変換する
        double tmplon = 0, tmplat = 0;
        //tmplon = imcs01->getLongitude();
        //tmplat = imcs01->getLatitude();

        int x0, y0;

        if (robot->tukuba) {
            x0 = scale * (lonToPixel(tmplon) + TUKUBA_OFFSET_X);
            y0 = scale * (latToPixel(tmplat) + TUKUBA_OFFSET_Y);
        }
        else {
            x0 = scale * (lonToPixel(tmplon)  + KIT_OFFSET_X);
            y0 = scale * (latToPixel(tmplat)  + KIT_OFFSET_Y);
        }

        painter.setPen(pen_blue);
        //drawPos(x0+dx, y0+dy, painter, reckoning_points_saved); // changed
        drawPos(x0, y0, painter, reckoning_points_saved);

    }
    else {
        // ログ再生モード
        // playLog(painter);
    }


    // ウインドウの中心位置表示　draw the center of the window
    painter.setPen(pen_black);
    painter.setWindow(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    painter.drawLine(WINDOW_WIDTH/2 - 5, WINDOW_HEIGHT/2, WINDOW_WIDTH/2 + 5, WINDOW_HEIGHT/2) ;
    painter.drawLine(WINDOW_WIDTH/2,      WINDOW_HEIGHT/2 -5, WINDOW_WIDTH/2, WINDOW_HEIGHT/2 + 5);

    // 縮尺バー
    QString s100 = QString::number(PIX_TO_METER * 100, 'f', 0) + "m";
    painter.setPen(pen_black2);
    painter.drawLine(10,  20, 110, 20);
    painter.drawLine(10,  20,  10, 10);
    painter.drawLine(110, 20,  110, 10);
    painter.drawText(50, 15, s100);

    // QPoint p = widget->mapFromGlobal(QCursor::pos());
    /* if (robot->tukuba) {
        double tlon = TUKUBA_MAP_LON[1] - TUKUBA_MAP_LON[0];
        double pix_x = tlon * TUKUBA_LON_TO_METER;

        double tlat = TUKUBA_MAP_LAT[1] - TUKUBA_MAP_LAT[0];
        double pix_y = tlat * TUKUBA_LAT_TO_METER;

        double tpix = TUKUBA_MAP_POINT_X[1] - TUKUBA_MAP_POINT_X[0];
        double tmeter2 = tpix * METER_TO_PIX;

        printf("Tukua pix_x=%f pix_y=%f tmeter=%f \n",pix_x, pix_y,tmeter2);
    }
    else {
        double tlon = KIT_MAP_LON[1] - KIT_MAP_LON[0];
        double pix_x = tlon * KIT_LON_TO_METER;

        double tlat = KIT_MAP_LAT[1] - KIT_MAP_LAT[0];
        double pix_y = tlat * KIT_LAT_TO_METER;

        double tpix = KIT_MAP_POINT_X[1] - KIT_MAP_POINT_X[0];
        double tmeter2 = tpix * METER_TO_PIX;

        printf("KIT pix_x=%f pix_y=%f tmeter=%f \n",pix_x, pix_y,tmeter2);
    } */

    // 方位バー
    // painter.setPen(pen_black2);
    int ay = 130;
    painter.drawLine(40,  ay, 140, ay);
    painter.drawLine(90,  ay-50, 90, ay+50);

    if (robot->tukuba) {
      double tmpa = TUKUBA_INIT_POSE;
      QString s101 = QString::number(TUKUBA_INIT_POSE, 'f', 0) + "[°]";
      if (TUKUBA_INIT_POSE < 180) tmpa += 360;
      double tmpb = tmpa + 90;
      if (tmpb > 360) tmpb -= 360;
      double tmpc = tmpb + 90;
      if (tmpc > 360) tmpc -= 360;
      double tmpd = tmpc + 90;
      if (tmpd > 360) tmpd -= 360;

      QString s102 = QString::number(tmpb, 'f', 0);
      QString s103 = QString::number(tmpc, 'f', 0);
      QString s104 = QString::number(tmpd, 'f', 0);

      painter.drawText(135, ay+5, ">"+s101);
      painter.drawText( 80, ay+65, s102);
      painter.drawText( 20, ay+5,  s103);
      painter.drawText( 80, ay-55, s104);

    }
    else {
      double tmpa = KIT_INIT_POSE;
      QString s111 = QString::number(KIT_INIT_POSE, 'f', 0) + "[°]";
      if (KIT_INIT_POSE < 180) tmpa += 360;
      tmpa -= 180;
      QString s112 = QString::number(tmpa, 'f', 0);
      painter.drawText(160, ay+5, s111);
      painter.drawText( 10, ay+5, s112);
    }


    // カーソルの座標表示
    QPoint p = gwindow->map->mapFromGlobal(QCursor::pos());
    if ((p.x()>=0) && (p.y()>=0)) {
        QString sx = QString::number(p.x(), 'f', 0);
        QString sy = QString::number(p.y(), 'f', 0);
        QString sxy = "(x=" + sx + ", y=" + sy + ")";
        painter.drawText(10, 40, "Cursor");
        painter.drawText(55, 40, sxy);
    }

    // ウェイポイントの位置と数値を画面に表示
    //printWayPoints(offset_x-dx, offset_y-dy, painter);
    printWayPoints(offset_x, offset_y, 0, 0, painter);

    // 情報の表示
    printInfo(0, 200, painter);

    tmp_lat0 = tmp_lat;
    tmp_lon0 = tmp_lon;
}


//! 表示イベント
void GuiClass::paintEvent(QPaintEvent* event)
{
    if (robot->tukuba) {
        drawMapAndRobotPos(TUKUBA_OFFSET_X, TUKUBA_OFFSET_Y);
    }
    else {
        drawMapAndRobotPos(KIT_OFFSET_X, KIT_OFFSET_Y);
    }

    update();
}

//! ウェイポイントの読み込み Read waypoints
void GuiClass::readFile(const QString &file_name)
{
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly))
    {
        cerr << " cannot open file for reading: "
        << qPrintable(file.errorString()) << endl;
        return;
    }
    QTextStream in(&file);

    int i = 0;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList fields = line.split(',');

        if (fields.size() >= 4)
        {
            fields.takeFirst().toDouble();
            robot->waypoint[i].latitude  = fields.takeFirst().toDouble();
            robot->waypoint[i].longitude = fields.takeFirst().toDouble();
            //robot->waypoint[i].offset    = fields.takeFirst().toDouble();
            robot->waypoint[i].pose      = fields.takeFirst().toDouble();
            robot->waypoint[i].x = lonToMeter(robot->waypoint[i].longitude - robot->waypoint[START_WAYPOINT].longitude);
            robot->waypoint[i].y = latToMeter(robot->waypoint[i].latitude  - robot->waypoint[START_WAYPOINT].latitude);

            i++;
        }
    }
}


void GuiClass::openFile(const QString &file)
{
    //pixmap = QPixmap();
    pixmap.load(file);
}


//! 環境設定 Set the environment, read the way points file
void GuiClass::setupEnv()
{
    pixmap = QPixmap();
    QPainter painter(this);

    //robot->tukuba      = true;
    //robot->tukuba      = false;
    //robot->map_movable = true;
    map_movable = true;
    robot->leftClickPos.x = (int) (0.5 * WINDOW_WIDTH);
    robot->leftClickPos.y = (int) (0.5 * WINDOW_HEIGHT);

    if (robot->tukuba)
    {
        //robot->map_movable = true;
        pixmap.load(TUKUBA_MAP);
        robot->draw_x0 = TUKUBA_OFFSET_X;
        robot->draw_y0 = TUKUBA_OFFSET_Y;

        // read the way points file
        // readFile("waypoints/TUKUBA_WAY_POINTS.txt");
        readFile(TUKUBA_WAYPOINTS_FILE);
        //printWayPoints(TUKUBA_OFFSET_X, TUKUBA_OFFSET_Y,painter);
    }
    else
    {
        //robot->map_movable = true;
        pixmap.load(KIT_MAP);
        robot->draw_x0 = KIT_OFFSET_X;
        robot->draw_y0 = KIT_OFFSET_Y;

        // read the way points file
        //readFile("waypoints/KIT_WAY_POINTS.txt");
        readFile(KIT_WAYPOINTS_FILE);
        //printWayPoints(KIT_OFFSET_X, KIT_OFFSET_Y,painter);
     }
}

//! 地図縮尺の取得
double GuiClass::getScale()
{
    return scale;
}

//! 地図縮尺の設定
void GuiClass::setScale(double value)
{
    scale = (double) value/ 100.0;

    pixmap = QPixmap();
    QPixmap pixmap2;

    if (robot->tukuba)
    {
        pixmap2.load(TUKUBA_MAP);
        pixmap = pixmap2.scaled((int) 3288 * scale, (int) 3288 * scale,Qt::KeepAspectRatio,Qt::FastTransformation);
    }
    else
    {
        pixmap2.load(KIT_MAP);
        pixmap = pixmap2.scaled((int) 1000 * scale, (int)  740 * scale,Qt::KeepAspectRatio,Qt::FastTransformation);
     }
}

//地図の横幅
int GuiClass::getWidth()
{
    QPixmap pixmap2;
	
    if (robot->tukuba)
    {
        pixmap2.load(TUKUBA_MAP);
    }
    else
    {
        pixmap2.load(KIT_MAP);
    }
	return 	pixmap2.width();
}

//地図の縦幅
int GuiClass::getHeight()
{
    QPixmap pixmap2;
	
    if (robot->tukuba)
    {
        pixmap2.load(TUKUBA_MAP);
    }
    else
    {
        pixmap2.load(KIT_MAP);
    }
	return 	pixmap2.height();
}
