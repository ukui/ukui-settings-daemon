## ukui-setting-daemon 重构

### 目前任务
解决９个有问题中的三个
- housekeeping:商晓阳，5.9完成此插件（目标：功能全部测试可用，代码构建完成，对模块和功能全面理解）
- media-key：丁敬，暂时在专用机，回到项目后两周完成media-key插件.（目标：功能全部测试可用，代码构建完成，对模块和功能全面理解）

- datetime: 跟长沙李天智沟通之后，已经确定丢弃该插件
- sound: 二次改动较少，已经修改并提交，已经验证其功能(发现系统/用户的声音主题更改后调用pulseaudio库进行缓存更新)

### 项目安装依赖

1. git clone ...
2. 修改 /etc/apt/sources.list, 把 deb-src 之前的注释去掉，并执行 apt-get update
2. 执行 apt build-dep ukui-settings-daemon 会安装项目所有依赖

### 项目运行

1. 项目打包

- 根目录下执行如下命令，在上层目录生成 `deb` 文件

```shell
debuild -D
```

2. 项目安装

在生成 `deb` 前提下，先卸载系统上的项目再执行 `sudo apt install ./ukui-settings-daemon_xxxx.deb`

3. 项目运行

`ukui-settings-daemon --replace`

### 插件进度

> 选中表示确定可正常运行
- [ ] a11y-keyboard
- [x] a11y-settings
- [x] background
- [x] clipboard
- [x] common
- [x] datetime
- [x] dummy
- [ ] housekepping
- [x] keybindings
- [x] keyboard
- [ ] media-keys
- [x] mouse
- [x] mpris
- [ ] sound
- [ ] smartcard
- [x] typing-break
- [ ] xrandr
- [x] xrdb
- [o] xsettings

### 有问题的插件

| 插件 | 问题 | 负责人 |
| --- | --- | --- |
| a11y-keyboard | 未完成 | 商晓阳 |
| housekeeping | 未完成 | 刘彤 |
| datetime | 需要测试是否可用，运行未报错 | 闫焕章 |
| media-key | 未完成| 丁敬 |
| smartcard | 如果检测到硬件，内部段错误 | 商晓阳 |
| sound | 运行有报错:空链表 | 闫焕章 |
| xrdb | 运行报错:有未定义的接口，父类代码需要调整| 刘彤 |
| xsettings | 初步重构完成，测试过字体部分可用| 刘彤 |


