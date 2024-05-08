#include <vector>
#include <thread>
#include <array>
#include <queue>
#include <map>
#include "AI.h"
#include "constants.h"
#include <math.h>
#include <algorithm>

#define pi 3.14159265358979323846
// 注意不要使用conio.h，Windows.h等非标准库
// 为假则play()期间确保游戏状态不更新，为真则只保证游戏状态在调用相关方法时不更新，大致一帧更新一次
extern const bool asynchronous = true;

// 选手需要依次将player1到player4的船类型在这里定义
extern const std::array<THUAI7::ShipType, 4> ShipTypeDict = {
    THUAI7::ShipType::CivilianShip,
    THUAI7::ShipType::CivilianShip,
    THUAI7::ShipType::MilitaryShip,
    THUAI7::ShipType::FlagShip,
};

// 可以在AI.cpp内部声明变量与函数

// 常量申明
const int map_size = 50;
int dx[] = {-1, 0, 1, 0};
int dy[] = {0, -1, 0, 1};
int count1 = 0;
int count2 = 0;
bool temp1 = false;
const double SPEED_CIVIL_MS = 3.00;
const double SPEED_MILIT_MS = 2.80;
const double SPEED_FLAG_MS = 2.70;

struct Point
{
    int x, y;
};
struct Point direction[1000];


// 定义四个类，用于执行相关操作
class my_Resource
{
public:
    int HP=16000;
    int x;
    int y;
    // 开采点位
    int x_4p;
    int y_4p;

    // 是否已经开采过了
    bool produce;
    my_Resource(int x_, int y_,int x4,int y4)
    {
        x = x_;
        y = y_;
        x_4p = x4;
        y_4p = y4;
        produce = false;
    }
};

class my_Home
{
public:
    int x;
    int y;
    int HP = 10000;

    // 1己方 2敌方
    int group = 0;

    // 可以用于防卫家的点位
    int x_4p;
    int y_4p;
    my_Home(int i, int j, int gp)
    {
        x = i;
        y = j;
        group = gp;
    }
    my_Home()
    {
        x = 0;
        y = 0;
    }
};
std::vector<my_Home> home_vec;

class my_Construction
{
public:
    THUAI7::ConstructionType type=THUAI7::ConstructionType::NullConstructionType;
    int x;
    int y;
    int home_dist;

    // 建造点位
    int x_4c;
    int y_4c;
    int HP=0;
    // 0为无 1为己方 2为敌方
    int group=0;

    // 新建bool参数：是否被建造
    bool build = false;

    my_Construction(int i, int j,int i_4c,int j_4c)
    {
        x = i;
        y = j;
        x_4c = i_4c;
        y_4c = j_4c;
        home_dist = (i - home_vec[0].x) * (i - home_vec[0].x) + (j - home_vec[0].y) * (j - home_vec[0].y);
    }
    my_Construction()
    {
        x = 0;
        y = 0;
    }
};



class my_Wormhole
{
public:
    int x;
    int y;
    int HP = 18000;

    my_Wormhole(int i, int j, int hp)
    {
        x = i;
        y = j;
        HP = hp;
    }
};

std::vector<my_Resource> resource_vec;
std::vector<my_Construction> construction_vec;
my_Construction closest_2_home;
std::vector<my_Wormhole> wormhole_vec;

std::vector<std::vector<int>> Map_grid(map_size, std::vector<int>(map_size, 1));
struct NeedModule
{
    int number;
    THUAI7::ModuleType moduletype;
};
NeedModule modl1;




// 以下是调用的函数列表
void Judge(IShipAPI& api);                         // 判断应当进行哪个操作(攻击/获取资源等)
void Base_Operate(ITeamAPI& api);                  // 基地的操作
void AttackShip(IShipAPI& api);                    // 攻击敌方船只
void Install_Module(ITeamAPI& api, int number, int type);  // 为船只安装模块 1:Attack 2:Construct 3:Comprehensive
bool GoCell(IShipAPI& api);                        // 移动到cell中心
bool attack(IShipAPI& api);                                // 防守反击+判断敌人
void hide(IShipAPI& api);                                  // 丝血隐蔽


// 以下是寻路相关的函数
bool isValid(IShipAPI& api, int x, int y);
std::vector<std::vector<int>> Get_Map(IShipAPI& api);                            // 得到一个二维vector，包含地图上可走/不可走信息
const std::vector<Point> findShortestPath(const std::vector<std::vector<int>>& grid, Point start, Point end, IShipAPI& api);  // 广度优先搜索寻路
bool GoPlace(IShipAPI& api, int des_x, int des_y);
void GoPlace_Loop(IShipAPI& api,int des_x,int des_y);
bool Path_Release(std::vector<Point> Path, IShipAPI& api,int count); //实现路径(未使用)

// 以下是开采资源、建设相关函数

// 暴力遍历所有己方资源
void Get_Resource(IShipAPI& api);

// 贪心算法开采己方资源
void Greedy_Resource(IShipAPI& api);

// 在选定的位置建设选定的建筑物
void Build_Specific(IShipAPI& api, THUAI7::ConstructionType type, my_Construction construction);

// 在所有位置建设选定的建筑物
void Build_ALL(IShipAPI& api, THUAI7::ConstructionType type);

void Greedy_Build(IShipAPI& api, THUAI7::ConstructionType type)
{
    // 贪心算法按照路径进行建造
    auto selfinfo = api.GetSelfInfo();
    int gridx = selfinfo->x;
    int gridy = selfinfo->y;
    int cellx = api.GridToCell(gridx);
    int celly = api.GridToCell(gridy);

    int IntendedHp = 6000;
    if (type == THUAI7::ConstructionType::Factory)
    {
        IntendedHp = 8000;
    }
    else if (type == THUAI7::ConstructionType::Fort)
    {
        IntendedHp = 12000;
    }
    else if (type == THUAI7::ConstructionType::Community)
    {
        IntendedHp = 6000;
    }


    int size = construction_vec.size();
    if (size == 0)
    {
        api.Print("Please Get Construction ! ");
        return;
    }
    // minimum表示路径最小值
    int minimum = 1000;
    // order表示最小对应的编号
    int order = -1;
    int x;
    int y;
    for (int i = 0; i < size; i++)
    {
        x = construction_vec[i].x;
        y = construction_vec[i].y;

        // Greedy算法找到离自己最近的资源
        if ((cellx <= 25 && x <= 25) || (cellx >= 27 && x >= 27) && construction_vec[i].build == false)
        {
            auto path = findShortestPath(Map_grid, {cellx, celly}, {construction_vec[i].x_4c, construction_vec[i].y_4c}, api);
            int size = path.size();
            if (size < minimum && size > 0)
            {
                minimum = size;
                order = i;
            }
        }
    }
    if (order == -1)
    {
        // order==-1表示未发现符合要求的
        api.Print("Finished!");
        return;
    }

    // 前往建造
    x = construction_vec[order].x;
    y = construction_vec[order].y;
    GoPlace_Loop(api, construction_vec[order].x_4c, construction_vec[order].y_4c);
    int hp = api.GetConstructionHp(x,y);

    int count = 0;
    while (hp < IntendedHp && attack(api))
    {  // 没达到预期Hp 就继续建造
        api.Construct(type);
        count++;
        if (count % 10 == 0)
        {
            // 每十次进行一次判断与返回
            api.Wait();
            hp = api.GetConstructionHp(x, y);
            construction_vec[order].HP = hp;
            count = 0;
        }
    }
    if (hp > IntendedHp / 2)
    { // 如果达到了预期建筑物血量的一半，就标记为已经建造好了
        construction_vec[order].build= true;
        Greedy_Build(api, type);
    }
    else
    {
        api.Print("Abrupted!");
        return;
    }
}

// 以下是大本营管理相关函数

// 安装开采模组
void Produce_Module(ITeamAPI& api, int shipno,int type=3);

// 安装建造模组
void Construct_Module(ITeamAPI& api, int shipno,int type = 3);

// 建船函数
void Build_Ship(ITeamAPI& api, int shipno, int birthdes);


/*
以下是通信接口的定义： 

船船间：第一位"1" 表示遇到敌人 "0"表示正常
第二三、四五位表示自身所在的位置"0120" 默认全为0

船基间：
第一位表示所需的装备类型 "0":Sheild "1":Armor "2":Weapon
第二位表示装备的编号 "1""2""3"等
对于武器
1默认Laser Gun 2为Plasma 3为Shell 4为missle 5为Arc

基船间：
发送任意文字都表示遇袭
*/

void Decode_Me(IShipAPI& api);

void Send_Me(IShipAPI& api, std::string str);

void Decode_Me(ITeamAPI& api);

void Send_Me(ITeamAPI& api);

void AI::play(IShipAPI& api)
{
    if (api.GetFrameCount() < 5)
    {
        // 当且仅当 帧数为 1  调用Get_Map 
        // 节省运算时间
        Get_Map(api);
    }
    if (this->playerID == 1)
    {
        // 1号民船定位 挖矿
        Greedy_Resource(api);
        api.PrintSelfInfo();
    }
    else if (this->playerID == 2)
    {
        // 2号民船定位 建工厂
        Build_Specific(api, THUAI7::ConstructionType::Fort, closest_2_home);
        Build_ALL(api, THUAI7::ConstructionType::Factory);
        api.PrintSelfInfo();
    }
    else if (this->playerID == 3)
    {
        // 3号军船 ？？？
        attack(api);
        api.PrintSelfInfo();
    }
    else if (this->playerID == 4)
    {
        // 4号旗舰 偷家(不是)
        api.PrintSelfInfo();
        GoPlace_Loop(api, home_vec[1].x + 1, home_vec[1].y);

        api.Attack(pi);
    }
}

void AI::play(ITeamAPI& api)  // 默认team playerID 为0
{
    Base_Operate(api);
    api.PrintSelfInfo();
    api.PrintTeam();

    // 二号船装采集模块
    // 三号船装建造模块
    Produce_Module(api, 1, 3);
    Construct_Module(api, 2, 3);

    // 按照ShipTypeDict定义的船型
    // 在出生点位0(默认)进行建造
    Build_Ship(api, 2, 0);
    Build_Ship(api, 3, 0);
    Build_Ship(api, 4, 0);
}






bool GoCell(IShipAPI& api)
{
    auto selfinfo = api.GetSelfInfo();
    int gridx = selfinfo->x;
    int gridy = selfinfo->y;
    int cellx = api.GridToCell(gridx);
    int celly = api.GridToCell(gridy);

    double speed;
    if (selfinfo->shipType == THUAI7::ShipType::CivilianShip)
    {
        speed = SPEED_CIVIL_MS;
    }
    else if (selfinfo->shipType == THUAI7::ShipType::FlagShip)
    {
        speed = SPEED_FLAG_MS;
    }
    else if (selfinfo->shipType == THUAI7::ShipType::MilitaryShip)
    {
        speed = SPEED_MILIT_MS;
    }

    //返回一些关于CELL坐标信息
    std::string strx = std::to_string(cellx);
    std::string stry = std::to_string(celly);
    api.Print("GoCell Called");
    api.Print("This is current cell_X");
    api.Print(strx);
    api.Print("This is current cell_Y");
    api.Print(stry);

    int Gcellx = 1000 * cellx + 500;
    int Gcelly = 1000 * celly + 500;

    if (abs(Gcellx - gridx) <= 30 && abs(Gcelly - gridy) <= 30)
    {
        api.Print("Already Finished!");
        return true;
    }
    else
    {
        if (Gcellx < gridx)
        {
            api.MoveUp((gridx - Gcellx)/speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        }
        else if (Gcellx > gridx)
        {
            api.MoveDown((Gcellx - gridx)/speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        }
        if (Gcelly < gridy)
        {
            api.MoveLeft((gridy - Gcelly)/speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        }
        else if (Gcelly > gridy)
        {
            api.MoveRight((Gcelly - gridy)/speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        }
    }
    api.Wait();
    selfinfo = api.GetSelfInfo();
    gridx = selfinfo->x;
    gridy = selfinfo->y;
    cellx = api.GridToCell(gridx);
    celly = api.GridToCell(gridy);
    Gcellx = 1000 * cellx + 500;
    Gcelly = 1000 * celly + 500;
    
    if (abs(Gcellx - gridx) <= 30 && abs(Gcelly - gridy) <= 30)
    {
        api.Print("GoCell Finished !");
        return true;
    }
    else
    {
        api.Print("GoCell Did not Finish");
        return false; 
    }
}




void Base_Operate(ITeamAPI& api)
{
    auto selfships = api.GetShips();
    int number = selfships.size();
    if (number == 1)
    {
        Build_Ship(api, 2, 0);
    }
    else if (number == 2)
    {
        Build_Ship(api, 3, 0);
    }
    else if (number == 3)
    {
        Build_Ship(api, 4, 0);
    }
    return;
}

void Install_Module(ITeamAPI& api, int number, int type)
{
    // 对于number编号的船只安装对应的装备
    auto selfships = api.GetShips();
    int size = selfships.size();
    if (type == 1)
    {
        // 一号类型装备(Attack)
        api.InstallModule(number, THUAI7::ModuleType::ModuleMissileGun);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
        api.InstallModule(number, THUAI7::ModuleType::ModuleArmor2);
    }
    else if (type == 2)
    {
        // 二号类型装备(Construct)
        api.InstallModule(number, THUAI7::ModuleType::ModuleArmor1);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
        api.InstallModule(number, THUAI7::ModuleType::ModuleConstructor2);
    }
    else if (type == 3)
    {
        // 三号类型装备(Comprehensive)
        api.InstallModule(number, THUAI7::ModuleType::ModuleArcGun);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
        api.InstallModule(number, THUAI7::ModuleType::ModuleArmor1);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
        api.InstallModule(number, THUAI7::ModuleType::ModuleConstructor1);
    }
    return;
}

void AttackShip(IShipAPI& api)
{
    // 攻击敌方船只的函数
    int gridx = api.GetSelfInfo()->x;
    int gridy = api.GetSelfInfo()->y;

    auto Enemys = api.GetEnemyShips();
    int size = Enemys.size();
    // atan是math.h里面的函数(反三角正切)
    // 用于得到敌方船只的方位
    int* Enemyx = new int[size];
    int* Enemyy = new int[size];
    int* disx = new int[size];
    int* disy = new int[size];
    double* angle = new double[size];
    double* distance = new double[size];
    for (int i = 0; i < size; i++)
    {
        Enemyx[i] = Enemys[i]->x;
        Enemyy[i] = Enemys[i]->y;
        disx[i] = Enemyx[i] - gridx;
        disy[i] = Enemyy[i] - gridy;
        if (disx[i] == 0)
            if (disy[i] > 0)
                angle[i] = pi / 2;
            else
                angle[i] = -pi / 2;
        else if (disx[i] > 0)
            angle[i] = atan(disy[i] / disx[i]);
        else
            angle[i] = atan(disy[i] / disx[i]) + pi;
        distance[i] = disy[i] * disy[i] + disx[i] * disx[i];
    }

    // 先攻击视野内近的
    int flag = -1;
    for (int i = 0; i < size; i++)
    {
        if (Enemys[i] == nullptr)
            continue;
        else if (flag == -1)
            flag = i;
        else if (distance[i] < distance[flag])
            flag = i;
    }
    if (flag != -1)
        api.Attack(angle[flag]);
    delete[] distance;
    delete[] angle;
    delete[] disy;
    delete[] disx;
    delete[] Enemyy;
    delete[] Enemyx;
}

void hide(IShipAPI& api)
{
    int gridx = api.GetSelfInfo()->x;
    int gridy = api.GetSelfInfo()->y;
    int cellx = api.GridToCell(gridx);
    int celly = api.GridToCell(gridy);
    auto map = api.GetFullMap();
    int HP = api.GetSelfInfo()->hp;

    auto enemyships = api.GetEnemyShips();
    int size = enemyships.size();
    bool enemyflag = false;
    int first_en=-1;
    for (int i = 0; i < size; i++)
        if (enemyships[i] != nullptr)
        {
            enemyflag = true;
            first_en = i;
            break;
        }
    // 下面是丝血隐蔽（进shadow）
    for (int i = cellx - 10 < 0 ? 0 : cellx - 10; i < (cellx + 11 > 50 ? 50 : cellx + 11)&& map[cellx][celly]!=THUAI7::PlaceType::Shadow ; i++)  // 需要加逃离敌人+到视野外判断
    {
        for (int j = celly - sqrt(10 * 10 - (i - cellx) * (i - cellx)) < 0 ? 0 : celly - sqrt(10 * 10 - (i - cellx) * (i - cellx)); j < (celly + sqrt(10 * 10 - (i - cellx) * (i - cellx)) + 1 > 50 ? 50 : celly + sqrt(10 * 10 - (i - cellx) * (i - cellx)) + 1); j++)
        {
            if (map[i][j] == THUAI7::PlaceType::Shadow)
            {
                GoPlace_Loop(api, i, j);
                api.Wait();
                enemyships = api.GetEnemyShips();
                gridx = api.GetSelfInfo()->x;
                gridy = api.GetSelfInfo()->y;
                cellx = api.GridToCell(gridx);
                celly = api.GridToCell(gridy);
                size = enemyships.size();
                enemyflag = false;
                for (int i = 0; i < size; i++)
                    if (enemyships[i] != nullptr)
                    {
                        enemyflag = true;
                        break;
                    }
                if (enemyflag && map[cellx][celly] != THUAI7::PlaceType::Shadow)  // 或者underattack，在想判定
                    return hide(api);
                else
                    return;
            }
        }
    }
    if(enemyflag)//目前只是远离了第一个敌人，不然要SVM了，会比较抽象
    {
        auto Enemy1x = enemyships[first_en]->x;
        auto Enemy1y = enemyships[first_en]->y;
        int dis1x = Enemy1x - gridx;
        int dis1y = Enemy1y - gridy;
        double angle1 = atan(dis1y / dis1x);
        double distance1 = sqrt(dis1x * dis1x + dis1y * dis1y);
        GoPlace(api,cellx+(int)((8-distance1) * cos(angle1+pi)), celly+(int)((8-distance1) * sin(angle1+pi)));
    }
        // 以下是告知base要装装备，要定义全局变量传递信息，base内的函数可以再进行判断和决策，信息传递要加逻辑和判断
        /* modl1.number = api.GetSelfInfo()->playerID;
        modl1.moduletype = THUAI7::ModuleType::ModuleShield3;*/
}

bool attack(IShipAPI& api)  // 正在改，缺陷还很多
{
    int gridx = api.GetSelfInfo()->x;
    int gridy = api.GetSelfInfo()->y;
    int cellx = api.GridToCell(gridx);
    int celly = api.GridToCell(gridy);
    auto map = api.GetFullMap();
    int HP = api.GetSelfInfo()->hp;

    auto enemyships = api.GetEnemyShips();
    int totalenenmyhp = 0;
    int size = enemyships.size();

    for (int i = 0; i<size; i++)
        totalenenmyhp += enemyships[i]->hp;

    if (size)       //要增加shadow中受攻击的条件
    {
        if (HP < 2000 && totalenenmyhp > HP && map[cellx][celly]!=THUAI7::PlaceType::Shadow)  // 条件可能还要改，丝血隐蔽
        {
            hide(api);
            api.Wait();
        }
        // 攻击
        if (api.GetSelfInfo()->weaponType != THUAI7::WeaponType::NullWeaponType)
        {
            AttackShip(api);
        }
        else
        {
            hide(api);  // 暂时空着，后面用逃离函数替代，可能与上面丝血隐蔽合并代码
        }
        return false;
    }
    else
        return true;
}

void Judge(IShipAPI& api)  // 攻击判断不写在judge里，改为在采集/建造函数内部判断，但移动过程怎么办，写移动里面可能会死循环
{
    // 进行判断，讨论应当进行什么操作，攻击还是获取资源
    int gridx = api.GetSelfInfo()->x;
    int gridy = api.GetSelfInfo()->y;
    int cellx = api.GridToCell(gridx);
    int celly = api.GridToCell(gridy);
    auto map = api.GetFullMap();
    int HP = api.GetSelfInfo()->hp;

    // 资源&建造(To Be Edited)
    //  map 是整个地图的(cell坐标) 也就是说[i][j]的取值在0-49(50*50个格子)
    //  目前搜索范围和视野范围一样
    if (api.GetSelfInfo()->shipType == THUAI7::ShipType::CivilianShip || api.GetSelfInfo()->shipType == THUAI7::ShipType::FlagShip)
    {
        if (true)
            Get_Resource(api);
        else
            Build_ALL(api,THUAI7::ConstructionType::Factory);
    }
    // 缺少装备模块的条件，函数也不知道是哪个，还是说要到对象里面写？
}

std::vector<std::vector<int>> Get_Map(IShipAPI& api)
{
    auto map = api.GetFullMap();
    auto selfinfo = api.GetSelfInfo();
    int cellx = api.GridToCell(selfinfo->x);

    if (selfinfo->teamID == 0)
    {
        home_vec.push_back(my_Home(3, 46, 1));
        home_vec[0].x_4p = 3;
        home_vec[0].y_4p = 47;
        home_vec.push_back(my_Home(46, 3, 2));
        home_vec[0].HP = api.GetHomeHp();
    }
    else if (selfinfo->teamID == 1)
    {
        home_vec.push_back(my_Home(46, 3, 1));
        home_vec[0].x_4p = 46;
        home_vec[0].y_4p = 4;
        home_vec.push_back(my_Home(3, 46, 2));
        home_vec[0].HP = api.GetHomeHp();
    }

    for (int i = 0; i < map_size; i++)
    {
        for (int j = 0; j < map_size; j++)
        {
            if (map[i][j] == THUAI7::PlaceType::Space || map[i][j] == THUAI7::PlaceType::Shadow || map[i][j] == THUAI7::PlaceType::Wormhole)
            {  // 可经过的地点为0，默认(不可以途径)为1
                Map_grid[i][j] = 0;
                if (map[i][j] == THUAI7::PlaceType::Space || map[i][j] == THUAI7::PlaceType::Shadow)
                {
                    continue;
                }
                auto hp = api.GetWormholeHp(i, j);
                if (j == 24 || j == 25)
                {
                    if (hp == -1)
                    {
                        wormhole_vec.push_back(my_Wormhole(i, j, 18000));
                        hp = 18000;
                    }
                    else
                    {
                        wormhole_vec.push_back(my_Wormhole(i, j, hp));
                        if (hp <= 9000)
                        {
                            Map_grid[i][j] = 1;
                        }
                    }
                }
                else
                {
                    wormhole_vec.push_back(my_Wormhole(i, j, hp));
                    if (hp <= 9000)
                    {
                        Map_grid[i][j] = 1;
                    }
                }
                //std::string str_1 = "WormHole Found : (" + std::to_string(i) + "," + std::to_string(j) + ")\n";
                //std::string str_2 = "WormHole HP :" + std::to_string(hp) + "\n";
                //api.Print(str_1);
                //api.Wait();
                //api.Print(str_2);
            }
            else if (map[i][j] == THUAI7::PlaceType::Resource)
            {
                if (map[i + 1][j] == THUAI7::PlaceType::Space || map[i + 1][j] == THUAI7::PlaceType::Shadow)
                {
                    resource_vec.push_back(my_Resource(i, j, i + 1, j));
                    continue;
                }
                else if (map[i - 1][j] == THUAI7::PlaceType::Space || map[i - 1][j] == THUAI7::PlaceType::Shadow)
                {
                    resource_vec.push_back(my_Resource(i, j, i - 1, j));
                    continue;
                }
                else if (map[i][j + 1] == THUAI7::PlaceType::Space || map[i][j + 1] == THUAI7::PlaceType::Shadow)
                {
                    resource_vec.push_back(my_Resource(i, j, i, j + 1));
                    continue;
                }
                else if (map[i][j - 1] == THUAI7::PlaceType::Space || map[i][j - 1] == THUAI7::PlaceType::Shadow)
                {
                    resource_vec.push_back(my_Resource(i, j, i, j - 1));
                    continue;
                }
            }
            else if (map[i][j] == THUAI7::PlaceType::Construction)
            {
                if (map[i + 1][j] == THUAI7::PlaceType::Space || map[i + 1][j] == THUAI7::PlaceType::Shadow)
                {
                    construction_vec.push_back(my_Construction(i, j, i + 1, j));
                    continue;
                }
                else if (map[i - 1][j] == THUAI7::PlaceType::Space || map[i - 1][j] == THUAI7::PlaceType::Shadow)
                {
                    construction_vec.push_back(my_Construction(i, j, i - 1, j));
                    continue;
                }
                else if (map[i][j + 1] == THUAI7::PlaceType::Space || map[i][j + 1] == THUAI7::PlaceType::Shadow)
                {
                    construction_vec.push_back(my_Construction(i, j, i, j + 1));
                    continue;
                }
                else if (map[i][j - 1] == THUAI7::PlaceType::Space || map[i][j - 1] == THUAI7::PlaceType::Shadow)
                {
                    construction_vec.push_back(my_Construction(i, j, i, j - 1));
                    continue;
                }
            }
  
        }
    }

    int size = construction_vec.size();
    int temp = 1000;
    int number = -1;
    for (int i = 0; i < size; i++)
    {
        if (construction_vec[i].home_dist < temp)
        {
            temp = construction_vec[i].home_dist;
            number = i;
        }
    }
    if (construction_vec[number].home_dist < 64)
    {
        closest_2_home = construction_vec[number];
        api.Print("Get Consturction Point Closest to Home !\n Can Build Fort In This Place");
    }
    api.Print("The Map Has Already Got !");
    return Map_grid;
}

bool GoPlace(IShipAPI& api, int des_x, int des_y)
{
    if (des_x < 0)
        return GoPlace(api, 0, des_y);
    if (des_x >= 50)
        return GoPlace(api, 49, des_y);
    if (des_y < 0)
        return GoPlace(api, des_x, 0);
    if (des_y >= 50)
        return GoPlace(api, des_x, 49);
    api.Wait();
    auto selfinfo = api.GetSelfInfo();
    int cur_gridx = selfinfo->x;
    int cur_gridy = selfinfo->y;
    int cur_x = api.GridToCell(cur_gridx);
    int cur_y = api.GridToCell(cur_gridy);
    double speed = 3.0;
    if (selfinfo->shipType == THUAI7::ShipType::CivilianShip)
    {
        speed = SPEED_CIVIL_MS;
    }
    else if (selfinfo->shipType == THUAI7::ShipType::FlagShip)
    {
        speed = SPEED_FLAG_MS;
    }
    else if (selfinfo->shipType == THUAI7::ShipType::MilitaryShip)
    {
        speed = SPEED_MILIT_MS;
    }

    if (cur_x == des_x && cur_y == des_y)
    {
        api.Print("Have Already Reached the Place! ");
        return true;
    }

    Point start = {cur_x, cur_y};
    Point end = {des_x, des_y};

    std::vector<Point> path = findShortestPath(Map_grid, start, end, api);
    int path_size = path.size();
    // 注释的是用于判断寻路算法稳定性的代码
    // std::string str = std::to_string(path_size);
    // api.Print("This is the Size of the Path");
    // api.Print(str);
    for (int i = 0; i < path_size - 1; i++)
    {
        direction[i] = Point{
            path[i + 1].x - path[i].x,
            path[i + 1].y - path[i].y
        };
    }
    // api.Print("Got the Direction of the PATH !");
    // api.Wait();
    bool temp = false;
    for (int j = 0; j < path_size - 1; j++)
    {
        if (j % 6 == 0)
        {  // 每移动六次进行一次GoCell
            GoCell(api);
        }
        if (direction[j].x == -1)
        {
            api.MoveUp(1000 / speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        }
        else if (direction[j].x == 1)
        {
            api.MoveDown(1000 / speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        }
        else if (direction[j].y == -1)
        {
            api.MoveLeft(1000 / speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        }
        else
        {
            api.MoveRight(1000 / speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        }
    }
    GoCell(api);
    selfinfo = api.GetSelfInfo();
    cur_gridx = selfinfo->x;
    cur_gridy = selfinfo->y;
    cur_x = api.GridToCell(cur_gridx);
    cur_y = api.GridToCell(cur_gridy);
    if (cur_x == des_x && cur_y == des_y)
    {
        return true;
    }
    return false;
}

void GoPlace_Loop(IShipAPI& api, int des_x, int des_y)
{
    bool temp = false;
    int count = 0;
    while (temp == false && count < 10)
    {
        temp = GoPlace(api, des_x, des_y);
        count++;
    }
    if (temp == true)
    {
        api.Print("Arrived! Loop Terminated! ");
    }
    else
    {
        api.Print("Not Arrived Yet! Loop Terminated! ");
    }
}

bool isValid(IShipAPI& api, int x, int y)
{
    return (x >= 0 && x < map_size  && y >= 0 && y < map_size );
}

const std::vector<Point> findShortestPath(const std::vector<std::vector<int>>& grid, Point start, Point end, IShipAPI& api)
{
    // 记录每个点是否被访问过
    std::vector<std::vector<bool>> visited(map_size, std::vector<bool>(map_size, false));
    // 记录每个点的前驱节点，用于最后反向回溯路径
    std::vector<std::vector<Point>> parent(map_size, std::vector<Point>(map_size, {-1, -1}));
    // 创建一个队列用于BFS
    std::queue<Point> q;

    // 将起点标记为已访问，并加入队列
    visited[start.x][start.y] = true;
    q.push(start);

    // 创建一个路径vector
    std::vector<Point> Path;

    // BFS
    while (!q.empty())
    {
        Point current = q.front();
        q.pop();

        // 如果当前点是终点，说明已找到路径，停止搜索
        if ((current.x == end.x) && (current.y == end.y))
        {
            break;
        }

        // 遍历当前点的四个方向
        for (int i = 0; i < 4; ++i)
        {
            int newX = current.x + dx[i];
            int newY = current.y + dy[i];

            // 如果新坐标在地图范围内且未访问过且不是障碍物，则加入队列
            if (isValid(api, newX, newY) && !visited[newX][newY] && (grid[newX][newY] == 0))
            {
                visited[newX][newY] = true;
                parent[newX][newY] = {current.x, current.y};
                q.push({newX, newY});
                //std::string temp = "(" + std::to_string(newX) + "," + std::to_string(newY) + ")\n";
                //api.Print(temp);
            }
        }
    }

    // 回溯路径
    if (visited[end.x][end.y])
    {
        Point current = end;
        while (current.x != start.x || current.y != start.y)
        {
            Path.push_back(current);
            current = parent[current.x][current.y];
           // std::string temp = "Call back : (" + std::to_string(current.x) + "," + std::to_string(current.y) + ")\n";
           // api.Print(temp);
        }
        Path.push_back(start);
        reverse(Path.begin(), Path.end());
        api.Print("The Path Have Already Got!");
    }
    else 
    {
        api.Print("No path found! ");
        std::vector<Point> temp;
        return temp;
    }
    //std::string path_size = std::to_string(Path.size());
    //api.Print(path_size);
    return Path;
}
bool Path_Release(std::vector<Point> Path, IShipAPI& api, int count)
{
    int Path_size = Path.size();
    if (count >= Path_size - 1)
    {
        return true;
    }
    int delta_x = Path[count + 1].x - Path[count].x;
    int delta_y = Path[count + 1].y - Path[count].y;
    bool judge = false;

    if (delta_x == -1)
    {
        api.MoveUp(1000 / SPEED_CIVIL_MS);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
    }
    else if (delta_x == 1)
    {
        api.MoveDown(1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
    }
    else if (delta_y == 1)
    {
        api.MoveRight(1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
    }
    else if (delta_y == -1)
    {
        api.MoveLeft(1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
    }
    return false;
}

void Get_Resource(IShipAPI& api)
{  // 进行全自动资源开采，直到把本方地图上所有资源开采完为止
    auto selfinfo = api.GetSelfInfo();
    int gridx = selfinfo->x;
    int gridy = selfinfo->y;
    int cellx = api.GridToCell(gridx);
    int celly = api.GridToCell(gridy);

    int size = resource_vec.size();
    if (size == 0)
    {
        api.Print("Please Get Resource ! ");
        return;
    }
    int distance;
    int temp;
    int order;

    for (int i = 0; i < size; i++)
    {
        int x = resource_vec[i].x;
        int y = resource_vec[i].y;

        if ((cellx <= 25 && x <= 25) || (cellx >= 27 && x >= 27))
        {
            GoPlace_Loop(api, resource_vec[i].x_4p, resource_vec[i].y_4p);
            int state = api.GetResourceState(x, y);
            api.Print(std::to_string(state));
            int count = 0;
            while (state > 0 && attack(api))
            {  // 只要还有剩余资源就开采
                api.Produce();
                count++;
                if (count % 10 == 0)
                {
                    // 每十次进行一次判断与返回
                    api.Wait();
                    state = api.GetResourceState(x, y);
                    resource_vec[i].HP = state;
                    count = 0;
                }
            }
        }
    }
}

void Greedy_Resource(IShipAPI& api)
{
    auto selfinfo = api.GetSelfInfo();
    int gridx = selfinfo->x;
    int gridy = selfinfo->y;
    int cellx = api.GridToCell(gridx);
    int celly = api.GridToCell(gridy);

    int size = resource_vec.size();
    if (size == 0)
    {
        api.Print("Please Get Resource ! ");
        return;
    }
    // minimum表示路径最小值
    int minimum = 1000;
    // order表示最小对应的编号
    int order = -1;
    int x;
    int y;
    for (int i = 0; i < size; i++)
    {
        x = resource_vec[i].x;
        y = resource_vec[i].y;

        // Greedy算法找到离自己最近的资源
        if ((cellx <= 25 && x <= 25) || (cellx >= 27 && x >= 27) && resource_vec[i].produce == false)
        {
            auto path = findShortestPath(Map_grid, {cellx, celly}, {resource_vec[i].x_4p, resource_vec[i].y_4p}, api);
            int size = path.size();
            if (size < minimum && size > 0)
            {
                minimum = size;
                order = i;
            }
        }
    }
    if (order == -1)
    {
        // order==-1表示未发现符合要求的
        api.Print("Finished!");
        return;
    }

    // 前往开采
    x = resource_vec[order].x;
    y = resource_vec[order].y;
    GoPlace_Loop(api, resource_vec[order].x_4p, resource_vec[order].y_4p);
    int state = api.GetResourceState(x, y);

    int count = 0;
    while (state > 0 && attack(api))
    {  // 只要还有剩余资源就开采
        api.Produce();
        count++;
        if (count % 10 == 0)
        {
            // 每十次进行一次判断与返回
            api.Wait();
            state = api.GetResourceState(x, y);
            resource_vec[order].HP = state;
            count = 0;
        }
    }

    if (state == 0)
    {
        resource_vec[order].produce = true;
        Greedy_Resource(api);
    }
    else
    {
        api.Print("Abrupted!");
        return;
    }
}


void Build_ALL(IShipAPI& api, THUAI7::ConstructionType type)
{
    auto selfinfo = api.GetSelfInfo();
    int gridx = selfinfo->x;
    int gridy = selfinfo->y;
    int cellx = api.GridToCell(gridx);
    int celly = api.GridToCell(gridy);

    int IntendedHp = 8000;
    int size = construction_vec.size();
    if (size == 0)
    {
        api.Print("NO VALID CONSTRUCTION POINT!");
        return;
    }
    if (type == THUAI7::ConstructionType::Factory)
    {
        IntendedHp = 8000;
    }
    else if (type == THUAI7::ConstructionType::Fort)
    {
        IntendedHp = 12000;
    }
    else if (type == THUAI7::ConstructionType::Community)
    {
        IntendedHp = 6000;
    }
    for (int j = 0; j < 10; j++)
    {
        // 由于测试中Construct操作会消耗Energy，而Energy不够会导致终止
        // 因此我们采用十次循环，使得尽可能多建设
        for (int i = 0; i < size; i++)
        {
            if (((cellx <= 25 && construction_vec[i].x <= 25) || (cellx >= 27 && construction_vec[i].x >= 27)) && construction_vec[i].HP < IntendedHp)
            {
                GoPlace_Loop(api, construction_vec[i].x_4c, construction_vec[i].y_4c);
                int Hp = api.GetConstructionHp(construction_vec[i].x, construction_vec[i].y);
                if (Hp < IntendedHp)
                {
                    int count = 0;
                    while (Hp < IntendedHp && attack(api))
                    {
                        api.Construct(type);
                        count++;
                        if (count % 10 == 0)
                        {
                            api.Wait();
                            Hp = api.GetConstructionHp(construction_vec[i].x, construction_vec[i].y);
                            count = 0;
                            construction_vec[i].HP = Hp;
                        }
                    }
                }
                if (Hp >= IntendedHp / 2)
                {
                    construction_vec[i].build = true;
                }
            }
        }
    }
}

void Construct_Module(ITeamAPI& api, int shipno, int type)
{
    auto ships = api.GetShips();
    int size = ships.size();
    if (shipno > size || size == 0)
    {
        return;
    }

    auto constructtype = ships[shipno-1]->constructorType;

    int energy = api.GetEnergy();
    if (energy < 4000 || (ships[shipno-1]->shipType != THUAI7::ShipType::CivilianShip && ships[shipno-1]->shipType != THUAI7::ShipType::FlagShip))
    {
        return;
    }
    switch (type)
    {
        case 2 :
            if (energy > 4000)
            {
                api.InstallModule(shipno, THUAI7::ModuleType::ModuleConstructor2);
            }

            break;

        case 3 :
            if (energy > 8000)
            {
                api.InstallModule(shipno, THUAI7::ModuleType::ModuleConstructor3);
            }

            break;
    }
    return;
}

void Produce_Module(ITeamAPI& api, int shipno, int type)
{
    auto ships = api.GetShips();
    int size = ships.size();
    if (shipno > size || size == 0)
    {
        return;
    }
    if (ships[shipno - 1] == nullptr || (ships[shipno-1]->shipType != THUAI7::ShipType::CivilianShip&& ships[shipno-1]->shipType != THUAI7::ShipType::FlagShip))
    {
        return;
    }
    auto producetype = ships[shipno-1]->producerType;
    int energy = api.GetEnergy();
    switch (type)
    {
        case 2:
            if (producetype == THUAI7::ProducerType::Producer1 && energy > 4000)
            {   
                api.InstallModule(shipno, THUAI7::ModuleType::ModuleProducer2);
            }
        case 3:
            if (producetype != THUAI7::ProducerType::Producer3 && energy > 8000)
            {
                api.InstallModule(shipno, THUAI7::ModuleType::ModuleProducer3);
            }
    }
    return;
}

void Build_Ship(ITeamAPI& api, int shipno, int birthdes)
{
    auto selfinfo = api.GetSelfInfo();
    int energy = selfinfo->energy;
    auto ships = api.GetShips();
    int size = ships.size();
    if (shipno > size || size == 0)
    {
        return;
    }

    if (shipno <= size && ships[shipno - 1]->hp > 0)
    {
        api.Print("Ship Already Existed! ");
        return;
    }

    if (ShipTypeDict[shipno-1] == THUAI7::ShipType::CivilianShip)
    {
        if (energy > 4000)
        {
            api.BuildShip(THUAI7::ShipType::CivilianShip, birthdes);
        }
        else
        {
            api.Print("No Enough Money!");
        }
        return;
    }
    else if (ShipTypeDict[shipno-1] == THUAI7::ShipType::MilitaryShip)
    {
        if (energy > 12000)
        {
            api.BuildShip(THUAI7::ShipType::MilitaryShip, birthdes);
        }
        else
        {
            api.Print("No Enough Money!");
        }
    }
    else if (ShipTypeDict[shipno-1] == THUAI7::ShipType::FlagShip)
    {
        if (energy > 50000)
        {
            api.BuildShip(THUAI7::ShipType::FlagShip, birthdes);
        }
        else
        {
            api.Print("No Enough Money!");
        }
    }
    return;
}
void Build_Specific(IShipAPI& api, THUAI7::ConstructionType type, my_Construction construction)
{
    // 如果这个建筑物已经建成
    int hp = construction.HP;
    if (construction.type == THUAI7::ConstructionType::Community)
    {
        if (hp >= 4000)
        {
            return;
        }
    }
    else if (construction.type == THUAI7::ConstructionType::Factory)
    {
        if (hp >= 4000)
        {
            return;
        }
    }
    else if (construction.type == THUAI7::ConstructionType::Fort)
    {
        if (hp >= 6000)
        {
            return;
        }
    }


    // 否则运行到此处
    GoPlace_Loop(api, construction.x_4c, construction.y_4c);
    bool temp;
    int count = 0;
    int IntendedHp = 6000;
    hp = api.GetConstructionHp(construction.x, construction.y);
    if (type == THUAI7::ConstructionType::Factory)
    {
        IntendedHp = 8000;
    }
    else if (type == THUAI7::ConstructionType::Fort)
    {
        IntendedHp = 12000;
    }
    else if (type == THUAI7::ConstructionType::Community)
    {
        IntendedHp = 6000;
    }
    while (hp < IntendedHp)
    {
        count++;
        api.Construct(type);
        if (count % 10 == 0)
        {
            api.Wait();
            hp = api.GetConstructionHp(construction.x, construction.y);
            count = 0;
        }
    }
    construction.type = type;
    if (hp == IntendedHp)
    {
        api.Print("Construct Successfully Finished !");
    }
    return;
}

void Decode_Me(IShipAPI& api)
{
    // 舰船解码message
    if (api.HaveMessage())
    {
        auto message = api.GetMessage();
        int from = message.first;
        if (from == 0)
        {
            // 回防基地
            GoPlace_Loop(api, home_vec[0].x_4p, home_vec[0].y_4p);
        }
        else
        {
            auto info = message.second;
            if (info[0] == '1')
            {
                auto info1 = info.c_str();
                int place_x = 10 * (info1[1] - '0') + (info[2] - '0');
                int place_y = 10 * (info1[3] - '0') + (info[4] - '0');
                // 友军有难，不动如山（bushi）
                GoPlace_Loop(api, place_x, place_y);
            }
        }
    }
}

void Send_Me(IShipAPI& api, std::string str)
{
    // 非常简陋，主要是到底发送什么消息需要在Attack/Hide里面调用才合适
    if (str.c_str()[0] == '0')
    {
        api.SendBinaryMessage(0, str);
    }
    else
    {
        api.SendBinaryMessage(str.c_str()[0] - '0', str);
    }
    return;
}

void Send_Me(ITeamAPI& api)
{
    int hp = api.GetHomeHp();
    api.Wait();
    int hp1 = api.GetHomeHp();
    if (hp1 < hp)
    {
        api.SendBinaryMessage(1, "0");
        api.SendBinaryMessage(2, "0");
        api.SendBinaryMessage(3, "0");
        api.SendBinaryMessage(4, "0");
    }
    return;
}

void Decode_Me(ITeamAPI& api)
{
    // 基地解码
    if (api.HaveMessage())
    {
        auto message = api.GetMessage();
        int number = message.first;
        auto info = message.second.c_str();
        if (info[0] == '0')
        {
            switch (info[1] - '0')
            {
                case 1:
                    api.InstallModule(number, THUAI7::ModuleType::ModuleShield1);
                    break;
                case 2:
                    api.InstallModule(number, THUAI7::ModuleType::ModuleShield2);
                    break;
                case 3:
                    api.InstallModule(number, THUAI7::ModuleType::ModuleShield3);
                    break;
            }
        }
        else if (info[0] == '1')
        {
            switch (info[1] - '0')
            {
                case 1:
                    api.InstallModule(number,THUAI7::ModuleType::ModuleArmor1);
                    break;
                case 2:
                    api.InstallModule(number, THUAI7::ModuleType::ModuleArmor2);
                    break;
                case 3:
                    api.InstallModule(number, THUAI7::ModuleType::ModuleArmor3);
                    break;
            }
        }
        else if (info[0] == '2')
        {
            switch (info[1] - '0')
            {
                case 1:
                    
                    api.InstallModule(number, THUAI7::ModuleType::ModuleLaserGun);
                    break;
                case 2:
                    api.InstallModule(number, THUAI7::ModuleType::ModulePlasmaGun);
                    break;
                case 3:
                    api.InstallModule(number, THUAI7::ModuleType::ModuleShellGun);
                    break;
                case 4:
                    api.InstallModule(number, THUAI7::ModuleType::ModuleMissileGun);
                    break;
                case 5:
                    api.InstallModule(number, THUAI7::ModuleType::ModuleArcGun);
                    break;
            }
        }
    }
}
