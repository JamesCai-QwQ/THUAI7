#include <vector>
#include <thread>
#include <array>
#include <queue>
#include <map>
#include "AI.h"
#include "constants.h"
#include <math.h>

#define pi 3.141592653589
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
    my_Resource(int x_, int y_,int x4,int y4)
    {
        x = x_;
        y = y_;
        x_4p = x4;
        y_4p = y4;
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

// 在选定的位置建设选定的建筑物
void Build_Specific(IShipAPI& api, THUAI7::ConstructionType type, my_Construction construction);

// 在所有位置建设选定的建筑物
void Build_ALL(IShipAPI& api, THUAI7::ConstructionType type);

// 以下是大本营管理相关函数

// 安装开采模组
void Produce_Module(ITeamAPI& api, int shipno,int type=3);

// 安装建造模组
void Construct_Module(ITeamAPI& api, int shipno,int type = 3);

// 建船函数
void Build_Ship(ITeamAPI& api, int shipno, int birthdes);





void AI::play(IShipAPI& api)
{
    if (api.GetFrameCount() == 1)
    {
        // 当且仅当 帧数为 1  调用Get_Map 
        // 节省运算时间
        Get_Map(api);
    }
    if (this->playerID == 1)
    {
        // 1号民船定位 挖矿
        Get_Resource(api);
        api.PrintSelfInfo();
    }
    else if (this->playerID == 2)
    {
        // 2号民船定位 建工厂
        Build_Specific(api, THUAI7::ConstructionType::Fort, closest_2_home);
        api.PrintSelfInfo();
    }
    else if (this->playerID == 3)
    {
        // 3号军船 ？？？
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
    Produce_Module(api, 1, 3);
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
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else if (Gcellx > gridx)
        {
            api.MoveDown((Gcellx - gridx)/speed);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (Gcelly < gridy)
        {
            api.MoveLeft((gridy - Gcelly)/speed);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else if (Gcelly > gridy)
        {
            api.MoveRight((Gcelly - gridy)/speed);
            std::this_thread::sleep_for(std::chrono::seconds(1));
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
    else
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
        std::this_thread::sleep_for(std::chrono::seconds(1));
        api.InstallModule(number, THUAI7::ModuleType::ModuleArmor2);
    }
    else if (type == 2)
    {
        // 二号类型装备(Construct)
        api.InstallModule(number, THUAI7::ModuleType::ModuleArmor1);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        api.InstallModule(number, THUAI7::ModuleType::ModuleConstructor2);
    }
    else if (type == 3)
    {
        // 三号类型装备(Comprehensive)
        api.InstallModule(number, THUAI7::ModuleType::ModuleArcGun);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        api.InstallModule(number, THUAI7::ModuleType::ModuleArmor1);
        std::this_thread::sleep_for(std::chrono::seconds(1));
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
    auto Enemy0x = Enemys[0]->x;
    auto Enemy0y = Enemys[0]->y;
    int dis0x = Enemy0x - gridx;
    int dis0y = Enemy0y - gridy;
    double angle0;
    if (dis0x == 0)
        if (dis0y > 0)
            angle0 = pi / 2;
        else
            angle0 = -pi / 2;
    else if (dis0x > 0)
        angle0 = atan(dis0y / dis0x);
    else
        angle0 = atan(dis0y / dis0x) + pi;
    double distance0 = dis0y * dis0y + dis0x * dis0x;

    // 两个船都在视野内
    if (size != 1)
    {
        auto Enemy1x = Enemys[1]->x;
        auto Enemy1y = Enemys[1]->y;
        int dis1x = Enemy1x - gridx;
        int dis1y = Enemy1y - gridy;
        double angle1;
        if (dis1x == 0)
            if (dis1y > 0)
                angle1 = pi / 2;
            else
                angle1 = -pi / 2;
        else if (dis1x > 0)
            angle1 = atan(dis1y / dis1x);
        else
            angle1 = atan(dis1y / dis1x) + pi;
        double distance1 = dis1x * dis1x + dis1y * dis1y;

        // 优先攻击临近的船只
        if (distance1 <= distance0)
        {
            api.Attack(angle1);
        }
        else
        {
            api.Attack(angle0);
        }
    }
    else
    {
        // 只有一个船
        api.Attack(angle0);
    }
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
    // 下面是丝血隐蔽（进shadow）
    for (int i = cellx - 10 < 0 ? 0 : cellx - 10; i < (cellx + 11 > 50 ? 50 && map[cellx][celly]!=THUAI7::PlaceType::Shadow : cellx + 11); i++)  // 需要加逃离敌人+到视野外判断
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
                if (size && map[cellx][celly] != THUAI7::PlaceType::Shadow)  // 或者underattack，在想判定
                    return hide(api);
                else
                    return;
            }
        }
    }
    if(size)
    {
        auto Enemy1x = enemyships[0]->x;
        auto Enemy1y = enemyships[0]->y;
        int dis1x = Enemy1x - gridx;
        int dis1y = Enemy1y - gridy;
        double angle1 = atan(dis1y / dis1x);
        double distance1 = dis1x * dis1x + dis1y * dis1y;
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
                if (map[i][j] == THUAI7::PlaceType::Space || map[i][j] == THUAI7::PlaceType::Shadow)
                {
                    Map_grid[i][j] = 0;
                    continue;
                }
                auto hp = api.GetWormholeHp(i, j);
                if (j == 24 || j == 25)
                {
                    if (hp == -1)
                    {
                        wormhole_vec.push_back(my_Wormhole(i, j, 18000));
                        Map_grid[i][j] == 0;
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
                std::string str_1 = "WormHole Found : (" + std::to_string(i) + "," + std::to_string(j) + ")\n";
                std::string str_2 = "WormHole HP :" + std::to_string(hp) + "\n";
                api.Print(str_1);
                api.Wait();
                api.Print(str_2);
            }
            else if (map[i][j] == THUAI7::PlaceType::Resource)
            {
                if (map[i+1][j] == THUAI7::PlaceType::Space || map[i+1][j] == THUAI7::PlaceType::Shadow)
                {
                    resource_vec.push_back(my_Resource(i, j, i + 1,j));
                    continue;
                }
                else if (map[i - 1][j] == THUAI7::PlaceType::Space || map[i - 1][j] == THUAI7::PlaceType::Shadow)
                {
                    resource_vec.push_back(my_Resource(i, j, i - 1, j));
                    continue;
                }
                else if (map[i][j + 1] == THUAI7::PlaceType::Space || map[i][j + 1] == THUAI7::PlaceType::Shadow)
                {
                    resource_vec.push_back(my_Resource(i, j, i , j+1));
                    continue;
                }
                else if (map[i][j - 1] == THUAI7::PlaceType::Space || map[i][j - 1] == THUAI7::PlaceType::Shadow)
                {
                    resource_vec.push_back(my_Resource(i, j, i , j-1));
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
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else if (direction[j].x == 1)
        {
            api.MoveDown(1000 / speed);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else if (direction[j].y == -1)
        {
            api.MoveLeft(1000 / speed);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else
        {
            api.MoveRight(1000 / speed);
            std::this_thread::sleep_for(std::chrono::seconds(1));
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
            //std::string temp = "Call back : (" + std::to_string(current.x) + "," + std::to_string(current.y) + ")\n";
            //api.Print(temp);
        }
        Path.push_back(start);
        reverse(Path.begin(), Path.end());
        api.Print("The Path Have Already Got!");
    }
    else
    {
        api.Print("No path found! ");
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
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    else if (delta_x == 1)
    {
        api.MoveDown(1000);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    else if (delta_y == 1)
    {
        api.MoveRight(1000);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    else if (delta_y == -1)
    {
        api.MoveLeft(1000);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return false;
}

void Get_Resource(IShipAPI& api)
{   // 进行全自动资源开采，直到把本方地图上所有资源开采完为止
    // 不足之处：以x为变量遍历，可能耗时较长，但是sort重排也不方便
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
    for (int i = 0; i < size; i++)
    {
        int x = resource_vec[i].x;
        int y = resource_vec[i].y;
        if ((cellx <= 25 && x <= 25) || (cellx >= 27 && x >= 27))
        {
            GoPlace_Loop(api, resource_vec[i].x_4p, resource_vec[i].y_4p);
            int state = api.GetResourceState(x, y);
            int count = 0;
            while (state > 0&&attack(api))
            {   // 只要还有剩余资源就开采
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
    for (int i = 0; i < size; i++)
    {
        if ((cellx <= 25 && construction_vec[i].x <= 25) || (cellx >= 27 && construction_vec[i].x >= 27))
        {
            GoPlace_Loop(api, construction_vec[i].x_4c, construction_vec[i].y_4c);
            int Hp = api.GetConstructionHp(construction_vec[i].x, construction_vec[i].y);
            if (Hp < IntendedHp)
            {
                int count = 0;
                while (Hp < IntendedHp&&attack(api))
                {
                    api.Construct(type);  
                    count++;
                    if (count % 10 == 0)
                    {
                        api.Wait();
                        Hp = api.GetConstructionHp(construction_vec[i].x, construction_vec[i].y);
                        count = 0;
                    }
                } 
            }
        }
    }
}

void Construct_Module(ITeamAPI& api, int shipno, int type)
{
    auto ships = api.GetShips();
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
    GoPlace_Loop(api, construction.x_4c, construction.y_4c);
    bool temp;
    int count = 0;
    int IntendedHp = 6000;
    int hp = api.GetConstructionHp(construction.x, construction.y);
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
    if (hp == IntendedHp)
    {
        api.Print("Construct Successfully Finished !");
    }
    return;
}
