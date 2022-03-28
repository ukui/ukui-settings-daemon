# ukui-settings-daemon
## 1	引言
 `ukui-settings-daemon` 是`UKUI`桌面环境下的底层守护程序；
 负责设置`UKUI`会话的各种参数以及运行的应用程序。
 本文档为用户配置服务的介绍，针对组件的结构，模块设计，程序接口进行介绍。
 ### 项目安装依赖

1. 执行: `sudo mk-build-deps -ir debian/control` 会安装项目所有依赖

### 项目运行

1. 项目打包

- 根目录下执行如下命令，在上层目录生成 `deb` 文件

```shell
debuild -D
```

2. 项目安装

上层目录生成 `deb` 二进制文件后，进入上层目录，执行 `sudo dpkg -i ./ukui-settings-daemon_xxxx.deb`

3. 项目运行

安装后运行：`ukui-settings-daemon --replace`

## 模块介绍
| 模块 | 功能 | 插件名称 |
|--|--|--|
| 磁盘监控 | 实现系统中磁盘空间的监听报警功能。并提醒用户清空回收站 |housekeeping|
|远程服务|开启、关闭本地远程服务|sharing|
|光标提醒|在登录系统后，根据设置按下ctrl时以圆圈的方式提醒光标位置|usd-locate-pointer|
|系统快捷键|实现系统快捷键、自定义快捷键|media-key|
|用户自定义快捷键|实现用户自定义快捷键|keybindings|
|媒体按键|向多媒体播放器传递播放信号|mpris|
|定位器参数管理|实时设置鼠标、触摸板的参数，并在用户登录时恢复至用户使用的参数|mouse|
|键盘参数管理|实时设置键盘的参数，并在用户登录时恢复至用户使用的参数|keyboard|
|显示模式切换模块|通过ui交互获取用户选择的显示模式，并进行显示|kds|
|显示器管理模块|登录或者插拔显示器时，恢复用户配置|xrandr|
|色温管理|依据参数在特定时刻调整显示器色温值|color|
## 接口介绍
控制面板后台部分使用的接口分为gsettings与DBus，提供给内外部进行使用。
### gsettings
应用程序可以使用gsettings来保存用户配置信息并且确保用户隔离。程序在运行中进行设置、修改gsettings的已有的键值，但是不能通过程序代码创建新的键值，gsettings在一个叫做schema的规范文件中创建，schema（主题）文档其实是一个规范的xml文档。
应用程序通过信号槽来监视文件改动，并根据改动的key处理对应事件。

控制面板后台使用到如下schema：

 - org.ukui.peripherals-keyboard 
 - org.ukui.peripherals-mouse
  - org.ukui.peripherals-touchpad
  - org.ukui.SettingsDaemon.plugins.color
  - org.ukui.SettingsDaemon.plugins.housekeeping
  - org.ukui.SettingsDaemon.plugins.media-keys
### org.ukui.peripherals-keyboard
#### **repeat**
取值范围：true，false。
用户打开控制面板，设置按键重复设置时，即设置此值。值为true时，长按按键重复输出按键，值为false时，长按按键不重复输出按键。
#### **rate**
取值范围10-110。
repeat为true时，此值为连续输出字符的速度。
#### **delay**
取值范围200-2100
当repeat为true时，延迟此值时间后，按照rate设置的速度连续输出字符。
#### **capslock-state**
实时记录用户设置的大写锁定按键，在开机时恢复为该值的设置。
#### **numlock-state**
实时记录用户设置的数字按键，在开机时恢复为该值的设置。
### **org.ukui.peripherals-mouse**
#### **left-handed**
设置鼠标主按键位置，当用户设置为左键时，左键为选择按键，当用户设置为右键时，右键为选择按键。
#### **wheel-speed**
实时响应该键值的数值，提取键值设置为鼠标滚轮速度。
#### **double-click**
实时响应该键值的数值，提取键值设置为双击间隔时长。
#### **motion-acceleration**
实时响应该键值的数值，提取键值设置为指针速速度。
#### **mouse-accel**
实时响应该键值的数值，提取键值设置为指针加速度。
#### **locate-pointer**
实时响应该键值的数值，并根据该键值具体情况，按下ctrl显示鼠标具体位置。
### **org.ukui.peripherals-touchpad**
#### **disable-on-external-mouse**
取值范围：true，false。
功能：
当用户开启插入鼠标时禁用触摸板，则在鼠标接入时，触摸板将不会生效。
#### **disable-while-typing**
取值范围：true，false。
功能：当用户开启打字时禁用触摸板，则打字期间触摸板将不会有任何影响。
#### **vertical-two-finger-scrolling**
取值范围：true，false。
功能：用户开选择垂直双指滑动时，该键值为ture，双指在触摸板滑动时，滚动条可根据情况进行滑动。
#### **horizontal-two-finger-scrolling**
取值范围：true，false。
功能：用户开选择水平双指滑动时，该键值为ture，双指在触摸板滑动时，滚动条可根据情况进行滑动。
#### **horizontal-edge-scrolling**
取值范围：true，false。
功能：用户开选择水平边际滑动时，该键值为ture，手指在触摸板边缘滑动时，滚动条可根据情况进行滑动。
#### **vertical-edge-scrolling**
取值范围：true，false。
功能：用户开选择水平垂直滑动时，设置该键值为ture，手指在触摸板边缘滑动时，滚动条可根据情况进行滑动。
### **org.ukui.SettingsDaemon.plugins.color**
#### **night-light-enabled**
取值范围：true，false。
夜间模式打开时，该值设置为true，夜间模式关闭时此值为false。
#### **night-light-allday**
取值范围：true，false。
时间选择全天时，该键值为true，跟随日出日落，或者自定义时，该值为false。
#### **night-light-temperature**
取值范围：1100~6500k(暖->冷)
设置根据该键值，在特定时间依据该键值设置色温。
#### **night-light-schedule-automatic**
取值范围：true，false。
设置为true时，根据night-light-schedule-automatic-from，night-light-schedule-automatic-to自动计算出时间段进入夜间模式。
#### night-light-schedule-automatic-from**
取值范围：00~23.98
夜间模式开始时间。
#### night-light-schedule-automatic-to**
取值范围：00~23.98
夜间模式结束时间。
#### **night-light-****schedule-from**
取值范围：00~23.98
自定义夜间模式开始时间。
#### **nig****ht-light-schedule****-to**
取值范围：00~23.98
自定义夜间模式结束时间。
## **DBus**

D-BUS是一种消息总线系统，是进程相互通信（IPC）的一种简单方式。除了进程间通信，D-BUS还有助于协调进程生命周期；它使编写“单实例”应用程序或守护程序变得简单而可靠，并在需要应用程序和守护程序的服务时按需启动它们。
BUS同时提供系统守护进程（用于“添加新硬件设备”或“打印机队列更改”等事件）和每个用户登录会话守护进程（用于用户应用程序之间的一般IPC需求）。此外，消息总线构建在通用的一对一消息传递框架之上，任何两个应用程序都可以使用该框架直接通信（无需通过消息总线守护进程）。目前，通信应用程序位于一台计算机上，或者通过适合在防火墙后与共享NFS主目录一起使用的未加密TCP/IP进行通信。（需要更好的远程传输方面的帮助-传输机制具有良好的抽象性和可扩展性）。

### **接口**

#### **/org/ukui/SettingsDaemon/MediaKeys**

##### **方法**

mediaKeyForOtherApp(int32 action, String appName)

##### **信号**

#### **/org/ukui/SettingsDaemon/Sharing**

##### **方法**

DisableService(String serviceName)

EnableService(String serviceName)

##### **信号**

#### **/org/ukui/SettingsDaemon/xrandr**

##### **方法**

getScreenMode(String appName)

参数：使用此接口的进程名称

返回值：类型int

0：第一屏模式

1：镜像屏幕模式

2：拓展屏幕模式

3：其他屏幕模式

功能：获取当前屏幕模式

getScreenParam(String appName)

参数：使用此接口的进程名称

返回值：json格式的字符串

功能：获取当前屏幕参数

setScreenMode(String modeName, String appName)

参数：

modeName：模式名称

“firstScreenMode”：第一屏模式

“cloneScreenMode”：镜像屏幕模式

“extendScreenMode”：拓展屏幕模式

“secondScreenMode”：其他屏幕模式

appName：调用此接口的程序名（后续考虑加密）

返回值：

功能：设置当前屏幕模式

setScreensParam(String screensParam, String appName)

> 参数：

screensParam：Json格式的屏幕参数

appName：调用此接口的进程名称（后续考虑加密）

返回值：无

功能：设置当前屏幕参数
参数示例
```c++
[
{
"enabled": true,
"id": "e2add05191c5c70db7824c9cd76e19f5",
"metadata": {
"fullname": "xrandr-LEN LI2224A-U5619HB8",
"name": "DP-2"
},
"mode": {
"refresh": 59.93387985229492,
"size": {
"height": 1080,
"width": 1920
}
},
"pos": {
"x": 0,
"y": 0
},
"primary": false,
"rotation": 1,
"scale": 1
}
]
```
##### **信号**
##### screenModeChanged(int32)
参数
- 0：第一屏模式
- 1：镜像屏幕模式
- 2：拓展屏幕模式
- 3：其他屏幕模式
屏幕模式发生改变时发送信号
##### screensParamChanged(String)
屏幕信息发生改变时发送信号
> 参数
Json格式的屏幕参数

## 详细设计
### 加载流程
main函数启动后，通过遍历包含settings-daemon关键字段的gsettings，并找出后缀名称并根据后缀名称加载对应的so文件。
### **函数说明**

#### **main.cpp**

int main (int argc, char* argv[])
参数

argc 参数个数，

argv 参数地址

返回值：int

功能：程序入口，调用**PluginManager**类激活各个功能。

void handler(int no)

参数

no 异常代码

返回值：无

功能：捕获kill信号，释放资源

#### plugin-manager.cpp

功能：遍历加载控制面板后台所有功能组件，并逐个初始化，激活。

PluginManager::**PluginManager**

参数：无

功能：构造函数，初始化资源

PluginManager::**~PluginManager**

参数：无

功能：析构函数，释放资源

PluginManager* PluginManager::**getInstance**

参数：无
返回值：PluginManager*

功能：获取实例

bool PluginManager::**managerStart**

参数:无

功能：用于初始化后，开始装载模块插件，并进行激活各个功能模块。

void PluginManager::**managerStop**

参数：无

功能：程序退出时释放插件资源。

static bool register_manager

参数:PluginManager

功能：检查dbus服务中是否有org.ukui.settingsdaemon服务，并以此判断ukui-settings-daemon是否已经启动。

#### plugin-interface.h

所有模块基于此类，暴露出activate接口与deactivate接口。功能模块通过实现activate执行自身业务逻辑，通过实现**deactivate**  退出模块并主动释放资源。

virtual **~PluginInterface**_() {};

虚析构函数

void **activate** () = 0;

纯虚函数，需要功能模块自己实现功能，实现自身业务逻辑。

 **deactivate** () = 0;

纯虚函数，需要功能模块自己实现功能，退出模块，并主动释放资源
## 磁盘容量监控（housekeeping）设计说明

### **程序模块说明**

磁盘容量监控根据用户设置的报警阈值，在登录后，以及每隔两分钟检测磁盘剩余，当满足设置阈值时弹窗告知用户当前剩余容量，用户可选择清空回收站释放一定资源空间
### **函数说明**

#### **housekeeping-plugin.cpp**

PluginInterface *createSettingsPlugin

参数：无

返回值：PluginInterface *

功能：创建插件接口

PluginInterface *HousekeepingPlugin::getInstance

参数：无

返回值：PluginInterface *

功能：获取实例

HousekeepingPlugin::activate

参数：无

返回值：无

功能：激活插件功能（由main加载参数时遍历处理）

HousekeepingPlugin::deactivate

参数：无

返回值：无

功能：注销插件，释放资源

#### **housekeeping-manager.cpp**

HousekeepingManager::HousekeepingManager
参数：无

返回值：无

功能：构造方法，初始化资源

HousekeepingManager::~HousekeepingManager
参数：无

返回值：无

功能：注销插件，释放资源

HousekeepingManager::HousekeepingManagerStart

参数：无

返回值：无

功能：检测当前容量，开启定时器固定时间触发功能。

HousekeepingManager::HousekeepingManagerStop

参数：无

返回值：无

功能：注销插件，释放资源

#### **ldsm-trash-empty.cpp**

实现容量监控报警弹窗，并根据功能清空回收站。
LdsmTrashEmpty::**LdsmTrashEmpty**(QWidget *parent) :
QDialog(parent),
ui(new Ui::LdsmTrashEmpty)
LdsmTrashEmpty::~**_LdsmTrashEmpty_**()
void LdsmTrashEmpty::**checkButtonCancel**()
执行取消按钮
void LdsmTrashEmpty::**checkButtonTrashEmpty**()
执行清空回收站
void LdsmTrashEmpty::**deleteContents**(const QString path)
void LdsmTrashEmpty::**connectEvent**()
建立信号槽绑定按钮与事件的关系
void LdsmTrashEmpty::**usdLdsmTrashEmpty**()
功能：判断回收站是否为空，不为空则显示清空回收站按钮。
void LdsmTrashEmpty::**windowLayoutInit**()
容量监控弹窗初始化
## **模式切换**（**kds**）设计说明

### **模块说明**

从显示器参数管理提供的DBus接口中获取当前屏幕数量，并将用户设置的模式通过DBus发送到显示器参数管理模块。

### **函数说明**

#### **main.cpp**

main()

#### **widget.cpp**

Widget::**Widget**(QWidget *parent) :QWidget(parent),ui(new Ui::Widget)

参数：parent父窗口

返回值：无

功能：构造函数

Widget::~**_Widget_**()

参数：无

返回值：无

功能：析构函数

void Widget::**screensParamChangedSignal**(QString screensParam)

参数：显示器模式

返回值：无

功能：该方法为链接xrandr提供的屏幕模式改变信号的槽函数，当接收到该信号后关闭页面，并退出。

void Widget::**beginSetup**()

参数：无

返回值：无

功能：初始化窗口参数，窗口资源，并建立信号的链接。

void Widget::**setupComponent**()

参数：无

返回值：无

功能：调用xrandr提供的DBus接口，查询当前屏幕参数，查询当前屏幕模式，并设置初始值。

void Widget::**setupConnect**()

参数：无

返回值：无

功能：链接界面按钮的click事件，并实现具体选项的功能

QString Widget::**getScreensParam**()

参数：无

返回值：无

功能：获取屏幕参数，被**setupComponent**()使用

int Widget::**getCurrentStatus**()

参数：无

返回值：无

功能：获取当前屏幕所处模式，传递给**initCurrentStatus**使用

void Widget::**initCurrentStatus**(int id)

参数：初始屏幕模式

返回值：无

功能：窗口打开标记当前所处的模式，

void Widget::**setScreenModeByDbus**(QString modeName)

参数：需要设置的屏幕模式

返回值：无

功能：当用户选需要设置的显示模式时，调用xrandr提供的DBus接口：setScreenMode，将用户选择的模式传递给xrandr组件，由xrandr组件实现具体功能。
## **键盘参数管理****（****keyboard****）设计说明**

### **模块说明**

针对控制面板的键盘设置项：按键重复，延迟，速度，按键提示提供接口，当用户操作控制面板时，由控制面板设置对应的参数，后台监控到参数改动时设置对应参数。

并且记录用户对与大小写，数字键的操作，当用户按下按键时，记录状态，重启后恢复用户的操作习惯。

### **算法及流程**

### **keyboard-manager.cpp**

static KeyboardManager *KeyboardManagerNew()

参数：无

返回值：无

功能：工厂模式，实例化一个mKeyboardManager对象，调用其他所需函数接口。

bool KeyboardManager::KeyboardManagerStart()

参数：无

返回值：无

功能：keyboard的启动函数，调用相关初始化函数进行启动项设置。

void KeyboardManager::numlock_install_xkb_callback ()

参数：无

返回值：无

功能：keyboard键盘大小写设置，开机检测是否开启大小写键，回调其函数调用x事件进行相关设定。

void KeyboardManager::start_keyboard_idle_cb ()

参数：无

返回值：无

功能：键盘设置的初始化函数，启动监盘时从此处开始进行，调用gsettings的相关配置文件，进行读取初始化。

void apply_settings (QString);

参数：str

返回值：无

功能：读取系统下的gsettings配置文件，进行读取设置相关参数与键盘功能是否开启，如大小写等。

void XkbEventsFilter(int keyCode);

参数：int

返回值：无

功能：在x服务事件下的，一些键值事件的处理。

void KeyboardManager::KeyboardManagerStop()

参数：无

返回值：无

功能：keyboard组件的disactive之前的操作，通过KeyboardManagerStop函数对其相关功能的关闭。

#### **keybindings-plugin.cpp**

PluginInterface *createSettingsPlugin

参数：无

返回值：PluginInterface *

功能：创建插件接口

PluginInterface *KeyboardPlugin::getInstance

参数：无

返回值：PluginInterface *

功能：获取实例交于main.cpp的方法使用

KeyboardPlugin::activate

参数：无

返回值：无

功能：激活插件功能（由main加载参数时遍历处理）

KeyboardPlugin::deactivate

参数：无

返回值：无

功能：注销插件，释放资源

KeyboardManager::KeyboardManager(QObject * parent)

参数：父窗口指针，当父窗口被delete时，会自动释放其关联的子窗口。

返回值：无

功能：构造函数

KeyboardManager::~KeyboardManager()

参数：无

返回值：无

功能：析构函数

KeyboardManager *KeyboardManager::KeyboardManagerNew()

参数：无

返回值：无

功能：实例化键盘管理

bool KeyboardManager::KeyboardManagerStart()

参数：无

返回值：无

功能：开启定时器，并绑定超时信号到槽方法start_keyboard_idle_cb。

void KeyboardManager::KeyboardManagerStop()

参数：无

返回值：无

功能：释放资源。

void KeyboardManager::start_keyboard_idle_cb ()

参数：无

返回值：无

功能：依次调用usd_keyboar_manager_apply_settings应用当前参数配置，并针对org.ukui.peripherals-keyboard的改变信号与槽函数apply_settings建立关联。

void KeyboardXkb::usd_keyboard_xkb_init(KeyboardManager* kbd_manager)

参数：无

返回值：无

功能：针对org.mate.peripherals-keyboard-xkb.general、org.mate.peripherals-keyboard-xkb.kbd建立相关槽函数，当发生改变时借由mate进行实现。

void numlock_xkb_init (KeyboardManager *manager)

参数：无

返回值：无

功能：字母灯状态初始化

usd_keyboard_manager_apply_settings (this);

参数：无

返回值：无

功能：应用当前所有配置

void KeyboardManager::apply_settings (QString keys)

参数：无

返回值：无

功能：并针对org.ukui.peripherals-keyboard的改变信号的槽函数，当产生改变信号后由此方法实现改变的键值。

## **系统快捷键****（****media-keys****）设计说明**

### **模块说明**

快捷键，又叫快速键或热键，指通过某些特定的按键、[按键](https://baike.baidu.com/item/%E6%8C%89%E9%94%AE/7194192)顺序或按键组合来完成一个操作，很多快捷键往往与如 Ctrl 键、Shift 键、Alt 键、Fn 键、 Enter键等配合使用。利用快捷键可以代替[鼠标](https://baike.baidu.com/item/%E9%BC%A0%E6%A0%87/122323)做一些工作，用户可以方便快速的完成一些操作。

### **算法及流程**



### **mediakey-manager.cpp**

bool MediaKeysManager::mediaKeysStart(GError*)

参数：GError

返回值：无

功能：mediakey激活之后的启动函数，此处开始调用相关的快捷键初始化接口，对其相关功能与配置进行初始化操作。

void mediaKeysStop();

参数：无

返回值：无

功能：mediakey组件相关功能的注销函数，此时相关功能处理挂起失效状态，还未注销其后台进程。

void initScreens();

参数：无

返回值：无

功能：初始化screen，调用函数gdk_display_get_default与gdk_display_get_default_screen(display)接口。

void MediaKeysManager::initXeventMonitor()

参数：无

返回值：无

功能：x服务下对keypress的监控响应与keyrelease事件的释放功能。

bool MediaKeysManager::getScreenLockState()

参数：无

返回值：无

功能：函数内部写有dbus接口，通过调用回调的mDbusScreensaveMessage获取当前的ScreenLockState的状态。

void doMicSoundAction()

参数：无

返回值：无

功能：键盘中声音快捷键设置的函数入口，通过键盘音量键，设置音量的大小与提示框的ui可视化显示。

void doBrightAction(int)

参数：int

返回值：无

功能：键盘中可视化亮度快捷键设置的函数入口，键盘的亮度调节功能，设置亮度的变化与可视化ui窗口的显示。

void doPowerOffAction();

参数：无

返回值：无

功能：对屏幕键盘的快捷键函数接口，按下电源power键，对屏幕的息屏相关操作的处理。

void doTouchpadAction(int);

参数：int

返回值：无

功能：对笔记本触摸板的相关设置，通过控制面板，调用读取触摸板的gesttings的配置文件org.ukui.peripherals-touchpad接口进行读与属性的设置。

#### **keybindings-plugin.cpp**

PluginInterface *createSettingsPlugin

参数：无

返回值：PluginInterface *

功能：创建插件接口

PluginInterface *HousekeepingPlugin::getInstance

参数：无

返回值：PluginInterface *

功能：获取实例

KeybindingsPlugin::activate

参数：无

返回值：无

功能：激活插件功能（由main加载参数时遍历处理）

KeybindingsPlugin::deactivate

参数：无

返回值：无

功能：注销插件，释放资源

## **定位器参数管理****（****mouse****）设计说明**

### **模块说明**

鼠标设置，对鼠标的相关的设置，设置一些鼠标相关的属性，如鼠标速度的响应，鼠标的左右手操作，与前进后退的鼠标快捷键功能等。

### **算法及流程**


图 12  mouse设置流程

### **mouse-manager.cpp**

void MouseManager::MouseManagerIdleCb( )

参数：无

返回值：无

功能：鼠标事件的初始化函数接口，函数的开始入口，初始化其相关功能与读取设置个gsettings的配置文件属性。

void MouseCallback(QString keys)

参数：str

返回值：无

功能：mouse的回调函数，设置一些鼠标相关的事件功能，如鼠标的点击回调，鼠标滚轮的速度等。

void SetLeftHandedAll (bool mouse_left_handed, bool touchpad_left_handed)

参数：bool

返回值：无

功能：打开控制面板，控制面板此功能的ui按钮，通过信号监控调用相关联的函数接口设置鼠标的左右手操作。

void SetMouseWheelSpeed (int speed)

参数：int

返回值：无

功能：对鼠标滚轮功能设置的函数接口，对滚轮速度的设置与限制（不能小于0），对鼠标前进后退功能的设置。

void SetMouseSettings()

参数：int

返回值：无

功能：获取配置文件的gesttings设置的KEY_LEFT_HANDED鼠标的值，并传值给其他功能接口作为参数调用。

void SetMotionLibinput (XDeviceInfo *device_info)

参数：XDeviceInfo *device_info

返回值：无

功能：x事件对输入设备的监控，通过传入设备的id号与name名称，对其进行属性的挂载。

#### **keybindings-plugin.cpp**

PluginInterface *createSettingsPlugin

参数：无

返回值：PluginInterface *

功能：创建插件接口

PluginInterface *HousekeepingPlugin::getInstance

参数：无

返回值：PluginInterface *

功能：获取实例

KeybindingsPlugin::activate

参数：无

返回值：无

功能：激活插件功能（由main加载参数时遍历处理）

KeybindingsPlugin::deactivate

参数：无

返回值：无

功能：注销插件，释放资源

## **多媒体快捷键****（****mpris****）设计说明**

### **模块说明**

特殊媒体快捷按键，可以控制一些媒体播放器的快捷键,如： 播放、暂停、停止等。

### **mpris-manager.cpp**

bool MprisManagerStart(GError **error)

参数：GError

返回值：无

功能：媒体快捷键的函数启动初始化入口，实现了对dbus信号等的接收，对象的实例化，与回调的过程。

void MprisManagerStop()

参数：无

返回值：无

功能：mpris快捷键功能资源的释放与stop过程。

void MprisManager::serviceRegisteredSlot(const QString& service)

参数：const str

返回值：无

功能：A media player was just run and should be added to the head of @mPlayerQuque.。

void MprisManager::serviceUnregisteredSlot(const QString& service)

参数：const str

返回值：无

功能：if anyone dbus service that from @busNames quit,this func will be called。

void keyPressed(QString,QString)

参数：str

返回值：无

功能：after catch MediaPlayerKeyPressed() signal, this function will be called。

## **声音提醒****（**sound**）设计说明**

### **模块说明**

提示音设置，listen for org.mate.sound。

### **sound-manager.cpp**

bool SoundManagerStart(GError **error)

参数：GError

返回值：无

功能：Starting sound manager begin，use gsettings that We listen for change of the selected theme。

void SoundManagerStop()

参数：无

返回值：无

功能：Stopping sound manager，释放了一些所需要的object资源。

void SoundManager::trigger_flush ()

参数：无

返回值：无

功能：We delay the flushing a bit so that we can coalesce multiple changes into a single cache flush 。

bool SoundManager::register_directory_callback (const QString path, GError **error)

参数：const str，GError

返回值：bool

功能：对注册服务之后收到事件的回调，，并建立其对file_monitor_changed_cb接口的connect。

#### **keybindings-plugin.cpp**

PluginInterface *createSettingsPlugin

参数：无

返回值：PluginInterface *

功能：创建插件接口

PluginInterface *HousekeepingPlugin::getInstance

参数：无

返回值：PluginInterface *

功能：获取实例

KeybindingsPlugin::activate

参数：无

返回值：无

功能：激活插件功能（由main加载参数时遍历处理）

KeybindingsPlugin::deactivate

参数：无

返回值：无

功能：注销插件，释放资源

## **光标提醒****（****usd-locate-pointer****）设计说明**

### **模块说明**

对光标事件的设置，使用gdk的应用对光标的start、pause、rewind、fps、duration、running等可视化事件的处理配置。


参数：无

返回值：PluginInterface *

功能：创建插件接口

PluginInterface *HousekeepingPlugin::getInstance

参数：无

返回值：PluginInterface *

功能：获取实例

KeybindingsPlugin::activate

参数：无

返回值：无

功能：激活插件功能（由main加载参数时遍历处理）

KeybindingsPlugin::deactivate

参数：无

返回值：无

功能：注销插件，释放资源

## **热插拔设备管理****（****xinput****）设计说明**

### **模块说明**

xinput对输入设备的管理，主要通过预定设计的配置文件去管理与驱动一些input设备，可以降低出现问题的概率与多次热插拔之后响应速率延时的问题。

### **算法及流程**



### **xinputmanager.cpp**

void updateButtonMap()

参数：无

返回值：无

功能：更新触控笔映射

void XinputManager::SetPenRotation(int device_id)

参数：int

返回值：无

功能：触控笔函数的处理接口，通过对输入设备id的获取与识别，对其进行相关的屏幕触控映射处理。

int EventSift(XIHierarchyEvent *event, int flag)

参数：XIHierarchyEvent ，int

返回值：int

功能：筛选出发生事件的设备ID，所有设备的事件信息与当前发生的事件，若失败返回-1，成功则返回获取到的id值。

void ListeningToInputEvent()

参数：无

返回值：无

功能：ListeningToInputEvent此函数主要对其所有输入设备的事件进行监控。

#### **keybindings-plugin.cpp**

PluginInterface *createSettingsPlugin

参数：无

返回值：PluginInterface *

功能：创建插件接口

PluginInterface *HousekeepingPlugin::getInstance

参数：无

返回值：PluginInterface *

功能：获取实例

KeybindingsPlugin::activate

参数：无

返回值：无

功能：激活插件功能（由main加载参数时遍历处理）

KeybindingsPlugin::deactivate

参数：无

返回值：无

功能：注销插件，释放资源

## **显示器参数管理****（****xrandr****）设计说明**

### **模块说明**

该模块记录用户设置的每组显示器参数，当接入已配置过的显示器时通过《显示参数恢复功能》恢复用户设置的参数，当显示器从未接入过，或者配置文件损坏时按照《缺省显示规则_》_进行设置，参数设置成功后，按照触摸设备映射规则进行重新映射触摸屏、触控笔与显示器的关系。

#### **显示参数恢复功能**

该功能将接入过的显示器参数保存至唯一码命名的配置文件，并根据当前的显示模式保存至：实时参数文件夹，第一屏参数文件夹，其他屏幕参数文件夹，镜像屏参数文件夹，通过此种方法可恢复用户在任意模式下的屏幕参数并进行恢复，无需用户再次针对特定屏幕组合进行设置，从而达到一次设置终身使用的效果。

#### **唯一码生成方式**

根据显示器的edid信息生成MD5-hash编码，当有多个显示器接入时采用多个显示器的信息生成显示器组MD5-hash编码。

#### **缺省显示规则**

第一屏
最大分辨率显示首个识别到的屏幕，设置为主屏并放置到(0,0)点坐标进行显示。

其他屏
最大分辨率显示器最后一个识别到的屏幕，设置为主屏并放置到(0,0)点坐标进行显示。

镜像屏幕
取出多个屏幕中共同存在的最大分辨率，并同时设置到（0，0）点进行显示。

拓展屏幕
将第一个识别到的屏幕设置为主屏，并放到（0，0）点，剩余的屏幕放到（第一个屏幕宽，0）坐标进行显示。

#### **触摸设备****映射****规则**

当插件启动以及显示参数（分辨率、角度）发生变化时会触发此功能，对触摸设备进行重新映射。

**映射规则:**

首先读取控制面板生成的触摸配置文件，按照配置内取到的映射关系进行映射。

当配置文件内的映射关系全部处理完毕后，仍有部分设备没有映射时则按照如下规则进行映射：

按照屏幕绝对尺寸和触摸设备尺寸误差2%进行匹配映射先进行映射。对于剩余设备直接映射。

触摸屏与触控笔可映射到同一显示器。



#### **启动流程**


软件启动时，获取当前的屏幕唯一码然后根据配置文件存在情况进行设置：如存在则读取恢复，如不存在则设置为缺省参数进行设置。

#### **事件循环流程**


当用户修改屏幕参数时，根据当前屏幕模式将参数实时保存至对应的文件夹内。

当用户切换模式时，取出该模式下的对应参数进行设置，如对应参数文件夹的配置不存在时，则按照缺省规则进行显示，并将参数文件复制一份到实时参数文件夹。

当接入屏幕时，首先识别出唯一码，而后读取实时参数文件夹下以唯一码命名的参数文件并设置显示器，如文件不存在则按照缺省规则进行显示。

### **xrandr-manager.cpp**

void XrandrManager::StartXrandrIdleCb()

参数：无

返回值：无

功能：屏幕事件相关的初始化回调函数，对screen设备的读取配置信息，模式的设置等相关入口。

void XrandrManager::applyConfig()

参数：无

返回值：无

功能：函数apply的入口，设置了autoRemapTouchscreen的功能与sendScreenModeToDbus的信号发送。

void XrandrManager::outputConnectedWithoutConfigFile(KScreen::Output *newOutput, char outputCount)

参数：KScreen::Output *newOutput, char outputCount

返回值：无

功能：接入时没有配置文件的处理函数，即处理预定设置，单屏位最优分辨率，多屏幕为镜像模式。

void XrandrManager::setScreenModeToFirst(bool isFirstMode)

参数：bool

返回值：无

功能：设置屏幕为第一屏的函数入口，通过遍历input设备找到第一个屏幕（默认为内屏）。

void XrandrManager::RotationChangedEvent(const QString &rotation)

参数：const str

返回值：无

功能：获取status-manager发来的信号的dbus值，通过对pc与rotation信号的解析，设置屏幕旋转的角度与模式。

void XrandrManager::SaveConfigTimerHandle()

参数：无

返回值：无

功能：处理来自控制面板操作的接口，获取控制面板中对screen参数的设定，写入相应的配置文件进行保存映射。

void XrandrManager::autoRemapTouchscreen()

参数：无

返回值：无

功能：处理触摸屏的配置映射关系，读取配置文件映射关系映射，配置文件不满足所有设备时，进行按照设备物理尺寸匹配进行自动映射。

#### **keybindings-plugin.cpp**

PluginInterface *createSettingsPlugin

参数：无

返回值：PluginInterface *

功能：创建插件接口

PluginInterface *HousekeepingPlugin::getInstance

参数：无

返回值：PluginInterface *

功能：获取实例

KeybindingsPlugin::activate

参数：无

返回值：无

功能：激活插件功能（由main加载参数时遍历处理）

KeybindingsPlugin::deactivate

参数：无

返回值：无

功能：注销插件，释放资源


## **GDK参数设置**（**xsettings**）设计说明**

### **模块说明**

设置底层系统主题变量，设定了一些光标等ui事件的gdk主题。



