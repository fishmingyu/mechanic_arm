#include <iostream>
#include <cmath>
#include <string>
#include <stdio.h>
#include <vector>
#include <iomanip>
#include "Matrix.h"
#include "Angle.hpp"
#include "Servo.hpp"

const double pi = 3.1415926;    //pi

Angle angle[6];    //舵机角度，与舵机名称一一对应,角度群部位相对轴的角度
Servo servo[6];    //舵机名称，分别为0，1，2，3，4，5号，全代码统一
double X = 0, Y = 0, Z = 0, L = 0 ,Zp = 0; //以腰部舵机中心建立坐标系，直角坐标系与柱坐标系相互转换
//X,Y,Z代表物体体心坐标，假设抓取足够准确，使体心坐标与机械手位于统一坐标平面之内
int angle_alpha; //机械手相对竖直方向z轴的俯仰角，逆时针方向，0-180,根据物体状态判别输入
int theta;       //底盘的极角，极轴以底盘舵机0˚方向为标准，theta为角度制，0-180˚
std::string Z_store = "", L_store = "", theta_store = "", STORE = "", alpha_store = "";

/*用机械臂长度近似舵机中心之间的距离*/
double arm_l1 = 105.14;    // 机械臂腰部到肩部的距离，单位mm(杆轴中心点到中心点，精确，下同）
double arm_l2 = 89.90;     // 机械臂的大臂长度
double arm_l3 = 169.74;   //156 数据更改 103.74 + 65 = 169.74
double up_to_land = 91.80; //1号舵机中心距离地面的高度,近似
bool catch_valid = false;  //判断是否可以抓取，有输入才可以启动抓取(debug模式)
Matrix ServoDH[5];
Matrix TT;
Matrix v1;

void setup()
{
    //mega2560开发板串口
    servo[0].attach(2, 500, 2500); //底盘，控制旋转，改变X,Y
    servo[1].attach(3, 500, 2500); //腰部舵机，改变Y,Z
    servo[2].attach(4, 500, 2500); //肩部舵机，改变Y,Z
    servo[3].attach(5, 500, 2500); //大臂关节舵机,改变Y,Z
    servo[4].attach(6, 500, 2500); //小臂关节舵机,不改变X,Y,Z，仅改变俯仰角
    servo[5].attach(7, 500, 2500); //手部关节舵机,小幅度内改变X,Y,Z
    //角度初始化,已调试
    angle[0] = { 90, 90 };
    angle[1] = { 90, 90 };
    angle[2] = { 90, 90 };
    angle[3] = { 90, 90 };
    angle[4] = { 90, 90 };
    angle[5] = { 0, 0 };
    //写入角度
    for (int i = 0; i < 6; i++)
    {
        angle[i].angleReach(servo[i]);
    }
    catch_valid = false;
    std::vector<double> r;
    r.push_back(0);
    v1.mArray.clear();
    for(int i = 0;i < 3;i++)
    {
        v1.mArray.push_back(r);
    }
    r.clear();
    r.push_back(1);
    v1.mArray.push_back(r);
    v1.wrRowCol(4, 1);
    std::cout << "******Begin!*******" << std::endl;
}

void loop()
{
    void catchit();
    void input_scheme();
    input_scheme(); //输入调试时的参数，3个分别为：Z轴方向的坐标Z,极径长度L,极角theta
    if (catch_valid)
        catchit(); //不断抓取物品
}

//逆运动学求解
void catchit() //控制机械手实现抓取
{
    angle[5].changeAngle(100);  //张开舵机开始抓取
    angle[5].angleReach(servo[5]);
    int angle_tmp[4] = { 0 };
    Y = L * sin(theta * pi / 180); //直角坐标与柱坐标之间的换算
    X = L * cos(theta * pi / 180);
    Zp = Z - up_to_land; //坐标换算
    double m = 0, n = 0;                      //换元
    double arm1_square = arm_l1 * arm_l1;
    double arm2_square = arm_l2 * arm_l2;
    double arc_angle1, arc_angle2; //弧度值暂存
    
    //理论计算结果代码化
    m = Zp - arm_l3 * cos(angle_alpha * pi / 180); // 一般大于0
    if(m > arm_l2 + arm_l1)
    {
        std::cerr<<"input error"<<std::endl;
        return;
    }
    n = L - arm_l3 * sin(angle_alpha * pi / 180); // 一定大于0
    double m_square = m * m;
    //简化计算
    double n_square = n * n;
    arc_angle2 = acos((m_square + n_square - arm1_square - arm2_square) / (2 * arm_l1 * arm_l2));
    if (arc_angle2 > (0.5 * pi) && arc_angle2 < pi)
    {
        arc_angle2 = pi - arc_angle2;
    }
    angle_tmp[2] = (arc_angle2 / pi) * 180;    //暂存解析法求解第二个主运动臂的角度
    double angle_auxiliary = asin((m_square + n_square + arm1_square - arm2_square) / (2 * arm_l1 * sqrt(m_square + n_square)));
    if (angle_auxiliary < 0 && angle_auxiliary > -pi / 2)
    {
        angle_auxiliary = -angle_auxiliary;// 反转
    }
    arc_angle1 = angle_auxiliary - atan(m / n);
    angle_tmp[1] = arc_angle1 * 180 / pi;         //解析法求解第一个主运动臂的角度
    angle_tmp[3] = angle_alpha - angle_tmp[1] - angle_tmp[2];     //根据运动分析求解出最后的角度,暂存
    printf("The angle 1 is: ");
    printf("%d\n", angle_tmp[1]);
    printf("The angle 2 is: ");
    printf("%d\n", angle_tmp[2]);
    printf("The angle 3 is: ");
    printf("%d\n", angle_tmp[3]);
    /*
    double T0[4][4] =
    {
        cos(theta * pi / 180),-sin(theta * pi / 180),0,0,
        sin(theta * pi / 180),cos(theta * pi / 180),0,0,
        0,0,1,0,
        0,0,0,1
    };
    double T1[4][4] =
    {
        cos(angle_tmp[1] * pi / 180),-sin(angle_tmp[1] * pi / 180),0,0,
        0,0,-1,-up_to_land,
        sin(angle_tmp[1] * pi / 180),cos(angle_tmp[1] * pi / 180),0,0,
        0,0,0,1
    };
    double T2[4][4] =
    {
        cos(angle_tmp[2] * pi / 180),-sin(angle_tmp[2] * pi / 180),0,arm_l1,
        sin(angle_tmp[2] * pi / 180),cos(angle_tmp[2] * pi / 180),0,0,
        0,0,1,0,
        0,0,0,1
    };
    double T3[4][4] =
    {
        cos(angle_tmp[3] * pi / 180),-sin(angle_tmp[3] * pi / 180),0,arm_l2,
        sin(angle_tmp[3] * pi / 180),cos(angle_tmp[3] * pi / 180),0,0,
        0,0,1,0,
        0,0,0,1
    };
    double T4[4][4] =
    {
        1,0,0,arm_l3,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    ServoDH[0].input(T0);
    ServoDH[1].input(T1);
    ServoDH[2].input(T2);
    ServoDH[3].input(T3);
    ServoDH[4].input(T4);
    TT = ServoDH[0];
    for(int i = 1;i < 5;i++)
    {
        TT = TT * ServoDH[i];
    }
    TT = TT * v1;
    */
    double x_compute = 0,y_compute = 0,z_compute = 0,l_compute = 0;
    l_compute = sin(angle_tmp[1] * pi / 180) * arm_l1 + sin((angle_tmp[1] + angle_tmp[2]) * pi /180) * arm_l2 + sin((angle_tmp[1] + angle_tmp[2] + angle_tmp[3]) * pi /180) * arm_l3;
    z_compute = cos(angle_tmp[1] * pi / 180) * arm_l1 + cos((angle_tmp[1] + angle_tmp[2]) * pi /180) * arm_l2 + cos((angle_tmp[1] + angle_tmp[2] + angle_tmp[3]) * pi /180) * arm_l3 + up_to_land;
    x_compute = l_compute * cos(theta * pi / 180);
    y_compute = l_compute * sin(theta * pi / 180);
    
    std::cout << "最后的三维坐标实际是"<< "X: " <<x_compute<< "\nY: "<<y_compute<<"\nZ: "<<z_compute<<std::endl;
    angle[0].changeAngle(theta);
    angle[1].changeAngle(angle_tmp[1] + 90);
    angle[2].changeAngle(-angle_tmp[2] + 90);
    angle[3].changeAngle(angle_tmp[3] + 90);
    for (int i = 0; i < 6; i++)
    {
        angle[i].angleReach(servo[i]);
    }
}

void input_scheme() //调试时使用的向Serial中输入
{
    std::cout<<"请输入柱坐标系三个坐标以及俯仰角度"<<std::endl;
    double stringToDouble(const std::string&);
    std::cin >> STORE;
    int data_length;
    int index[3] = { 0 };
    if (STORE.length() > 0) //如果已经读取，且不为空
    {
        int j = 0;
        std::cout << STORE << std::endl;
        data_length = static_cast<double>(STORE.length());
        for (int i = 0; i < data_length; i++)
        {
            if (STORE[i] == ',')
            {
                index[j] = i; //记录index的位置
                j++;
            }
        }
        Z_store = STORE.substr(0, index[0]);//读取子字符串
        L_store = STORE.substr(index[0] + 1, index[1]);
        theta_store = STORE.substr(index[1] + 1,index[2]);
        alpha_store = STORE.substr(index[2] + 1, data_length - 1);
        Z = stringToDouble(Z_store);
        L = stringToDouble(L_store);
        theta = stringToDouble(theta_store);
        angle_alpha = stringToDouble(alpha_store);
        printf("Z is: ");
        printf("%f\n", Z);
        printf("L is: ");
        printf("%f\n", L);
        printf("theta is: ");
        printf("%d\n", theta);
        printf("alpha is: ");
        printf("%d\n", angle_alpha);
        catch_valid = true;
    }
    else
    {
        catch_valid = false;
    }
    STORE = ""; //置为空，等待下一次读取
    
}

double stringToDouble(const std::string& str) //将string转换成为字符串，调试时使用
{
    double returnValue = 0;
    int index = 0;
    int dotIndex = (int)str.length();
    for (; index < str.length(); index++)
    {
        if (str[index] == '.')
        {
            dotIndex = index;
            index++;
            break;
        }
        if (str[index] < '0' || str[index] > '9')
        {
            return returnValue;
        }
        returnValue = 10 * returnValue + str[index] - '0';
    }
    for (; index < str.length(); index++)
    {
        if (str[index] < '0' || str[index] > '9')
        {
            return returnValue;
        }
        returnValue += double(str[index] - '0') / (double)pow(10, (index - dotIndex));
    }
    return returnValue;
}


int main()
{
    setup();
    loop();
    return 0;
}
