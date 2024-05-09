# 5/1 Update

1.避障/移动函数更新，通过调用GoCell，以及Move(1000),保证只整格移动，新增对于Asteriod/Resource作为障碍物的判断  
2.Judge函数更新


# 5/2 Update

### 1.Major Update：  
GoPlace 函数 返回Bool 类型变量  
True == 已经到达指定位置  
False == 未到达指定位置


### 2.由于不能出现死循环，修改了GoPlace函数
while -> if


### 3.GoCell 函数改进
通过api.Print(string)返回了Ship当前的x/y cell坐标信息


### 4.Obstacle函数重构(Not Finished Yet)

##### (1)三个参数
Obstacle(api,direct_x,direct_y,type)

其中direct_x direct_y表示希望前进的方向  
type 则用于表示在哪个方向遇到了障碍

显然这个参数表不够合理，并且重构后还是有bug

##### (2)新增判断
由于  
`THUAI7::PlaceType::Ruin`  
 `THUAI7::PlaceType::Resource`  
  `THUAI7::PlaceType::Aseroid`  
均无法穿过  
我们将其均设置为障碍物


### 5.Judge函数重构

将增改综合在AI5.2.cpp中

# 5/3 Major Update!

### 1.GoPlace 新架构

在认真学习了`数据结构与算法`(其实就是看CSDN + Github + ChatGPT) 之后  
并且发现之前比赛大家貌似都使用一些算法  
于是我改变了Obstacle 局部避障 + MoveUp/Down 的方法
* 该方法非常容易导致来回绕 
* 不能保证能够到达指定区域  

#### BFS 广度优先搜索算法
详见
##### ·https://www.geeksforgeeks.org/breadth-first-search-or-bfs-for-a-graph/
##### ·https://cp-algorithms.com/graph/breadth-first-search.html

#### Function API you might use

#### `GoPlace_bfs(api,des_x,des_y)`
* api:无需多言
* des_x:指定的目标x(cell)
* des_y:指定的目标y(cell)
  
#### `isValid(api,x,y)`
* api
* x:cell x 坐标
* y:cell y 坐标
  
#### `vector<vector<int>> Get_Map(api)`
得到整个地图，0表示可以经过，1表示不可经过

#### `vector<Point> findShortestPath(const std::vector<std::vector<int>>& grid, Point start, Point end, IShipAPI& api)`

* vector是模板类哦 (hyf:数算课要用哦~)
* Point类是结构体，定义如下qaq:
```
struct Point
{
    int x, y;
};
```
* 这个函数返回“最短路径”，以一个`Point`元素 `vector`容器的参数返回
* 这个函数为`GoPlace`的核心算法

### 2.Existing Problems

#### 开发组~
由于奇怪的设定，有概率卡在墙边上，但是我们的GoPlace函数会再次调用，也就是会重新规划路径qaq  
但是会浪费时间

#### 请不要直接导航到Resoure/Construction
因为这些地方到不了，它会返回
```~ No Path Found ! ~```

#### 鲁棒性有待进一步考证&Update


### 3.SPEED_OF_SHIPS
由于一些神奇的原因，之前我一直没有发现，原来这个`api.MoveUp(1000)`表示的是移动`1000 ms`而非`1000grid`
因此我们需要考虑一下不同船只具有的不同速度
```
const double SPEED_CIVIL_MS = 3;
const double SPEED_MILIT_MS = 2.8;
const double SPEED_FLAG_MS = 2.7;
```


### 4.测试运动性能
总体上还不错  
仍有一个问题
那就是`1000/3`这个玩意是有误差的，并且会不断累积，所以我们需要进行  
`GoCell(api)`
操作，这也就是说，我们需要在适当的时候使得这个误差足够小  
(虽然`500/3`还是有误差，开发组真的，就不能速度设成4吗？？？？？？)


>! ~~我是菜鸡qwq
  

# 5/4 Update

## 1.更新/修改了GoPlace算法
我们修改GoPlace算法，减少了其无用消耗时间和卡住的可能性。
* 采用GoCell每5次移动执行一次的策略，使得其不太可能总是卡墙
* GoPlace算法返回bool类型，用于while循环
* 修改了Path/Direction的循环次数，提高能正确按照Path执行的可能性
  
### 2.提供了两个全新类型的接口
我们提供了
`Get_Resource(api)`  
作为高度集成的函数，用于采集
`本区域内``所有`资源  

提供了`Bulid_ALL(api,type)`  
用于`本区域内` `所有`建筑均为本类的情况

### 3.针对浮点数特性进行的调整

* GoCell函数优化：与中心点的差值在30以内即视为GoCell完成！

### 4.Something U can do

#### 提供一些有关`建造` `采集`的策略 
#### 构建特定点位 特定类型的建筑函数
#### 提供从Produce/Construct退出的判断函数 

# 5/5 Update

## 1.GameInfo 地图上固定点位

大本营、虫洞点位是固定的  

### 大本营:  

右上(3,46)  左下(46,3)  

### 虫洞: 

(23,10) -- (23,11)  
(26,10) -- (26,11)  


(23,24) -- (23,25)  
(26,24) -- (26,25)  


(23,38) -- (23,39)  
(26,38) -- (26,39)

## 2.新增Base管理函数
```
void Produce_Module(ITeamAPI& api, int shipno,int type=3);
void Construct_Module(ITeamAPI& api, int shipno, int type = 3);
```

Argument Explaination

- shipno:所操作的船只编号  
- type:安装模组的种类(2/3)

## 3.my_Home my_Construction my_Resource my_Wormhole 自定义类
```
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

class my_Construction
{
public:
    THUAI7::ConstructionType type=THUAI7::ConstructionType::NullConstructionType;
    int x;
    int y;

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
    }
};

class my_Home
{
public:
    int x;
    int y;
    int HP=10000;

    // 1己方 2敌方
    int group=0;

    my_Home(int i, int j, int gp)
    {
        x = i;
        y = j;
        group = gp;
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
```


## 4.Get_Map函数升级
现在Get_Map函数具有许多的新特性/功能

* 1.对于虫洞的Hp进行判定，并具备判断虫洞是否可经过的能力
* 2.对于Resource / Construction 进行分析，具备判断资源是否可开采的能力
* 3.得到己方/敌方的Home并储存
* 4.以上的信息全部储存在`home_vec` `construction_vec` `resource_vec` `wormhole_vec`
等vector中，作为全局变量供大家调用

调用方式变化：  
默认在第一帧时调用Get_Map进行信息采集

## 5.Build_Ship函数

```
void Build_Ship(ITeamAPI& api, int shipno, int birthdes);
```

表示在出生点birthdes处建造编号为shipno的船只  
注意：shipno对应的船只类型是在文件开头 `ShipTypeDict` 中确定的

## 6.GoPlace_Loop函数

为了避免GoPlace一次调用无法实现到达目的地的问题  
经过实践，确定了GoPlace_Loop函数  
核心：调用GoPlace十次，如果十次还未到达目的地，则返回错误
```
void GoPlace_Loop(IShipAPI& api,int des_x,int des_y);
```
实践证明一般十次都可以到达目的地

# 5/6 Update

## 1.更新Get_Map函数

* 修复了BUG：关于虫洞的判定
* 通过teamID来判断自己的家在哪
### 更新 my_Home
* 新设定了两个位置 x_4p y_4p 用来回防的点位
* 会获取离Base最近且距离小于8000的点位 这个点位可以用来建造Fort
  
## 2.Build_Specific
更新，在选定的位置建设选定建筑物
## 3.Produce_Module / Construct_Module
更新，删去了limit参数，改为函数内部判断能否安装模组

## 4.my_closet_2_home
my_Construction类，用于存放距离Base最近的construction类型，并且距离在8以内，以用于建造Fort

# 5/7 Update

## 1.得到GetResourceState是否与视野有关
有关，若在视野外，则为0

## 2.Build功能实现

## 3.Get-Resource Optimize

## 4.SLEEP_FOR 450ms

# 5/8 Update

## 1.修改了 Build_Specific
使得其能够正常退出
## 2.修改了 Construction 类
加入了 bool Build 表示是否建造过这个建筑  
目的是避免Build_ALL反复建造

## 3.删去了不必要的Print
增加运行效率

## 4.Debug 关于 ITEAM API 
`Base_Operate`  
`Build_Ship`
`Produce_Module`
`Construct_Module`
分别更新，使得不再出现Vector越界的问题

## 5.新增Greedy_Resource
使得能够在最近的位置进行采集  
进行了更新，避免这个函数一直执行

# 5/9 Update
## 1.修复了已知的bugs
* 调整了Build_Specific的参数(index),保证统一性，避免重复建造
* 对于Build类的总体函数都加入了建造轮数round进行控制(开发组bug)

## 2.更新了Greedy_Build & Greedy_Resource

## 3.可能的：
1.军船策略  
2.避免采集资源时两个船在一块挤
