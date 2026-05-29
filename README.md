# MSI 笔记本赛睿键盘灯光控制精简控制器 / MSI SteelSeries Custom Light Controler

  一个用于替代臃肿的 SteelSeries GG 官方驱动的、极其轻量化的 C 语言控制工具。

  An extremely lightweight C-based control tool designed to replace the bloated official SteelSeries GG driver.

---

## 1. 项目简介与免责声明 / Project Introduction & Disclaimer

### 这是干嘛的？ / What is this for?

  本项目通过直接调用 `hidapi` 库与赛睿的 HID 端点进行通信，实现对笔记本物理键盘、灯条以及Logo区域的灯光控制，取代官方的臃肿软件SteelSeriesGG，直接通过底层数据包下发修改颜色。

  This project communicates directly with the SteelSeries HID endpoints by calling the `hidapi` library to achieve lighting control over the laptop's physical keyboard, lightbar, and Logo area. It replaces the official bloated SteelSeries GG software by directly issuing underlying data packets to modify colors.

### 免责声明 (重要) / Disclaimer (Important)

**本程序仅在本人的电脑上进行了完整测试并确认可用。** **This program has only been fully tested and confirmed working on my own computer.**
* **型号 / Model**：泰坦18 Pro 锐龙版 2025-050CN(Raider A18 HX A9WIG-050CN)，Windows 10 专业工作站版（是的，Win11是坨屎），版本号	22H2，OS 内部版本	19045.6466
* **Model**: Titan 18 Pro Ryzen Edition 2025-050CN (Raider A18 HX A9WIG-050CN), Windows 10 Pro for Workstations (yes, Win11 is a piece of s--t), Version 22H2, OS Build 19045.6466
* **注意**：由于微星和赛睿在不同代际、不同芯片平台（Intel/AMD）的机型上使用的控制芯片 PID 或灯区布局可能存在偏移，**原版代码保证在其他机型上不能完美运行，并对可能造成的损失概不负责**。但通过对报文内容的探索，其他机型有共用报文内容的可能性。如有兴趣可以自行探索。我是用WireShark和USBpacp插件截取报文内容并进行分析得到的以下结论。
* **Note**: Due to potential offsets in control chip PIDs or lighting zone layouts used by MSI and SteelSeries across different generations and chip platforms (Intel/AMD), **the original code is guaranteed NOT to run perfectly on other models, and no responsibility is assumed for any potential losses**. However, through exploration of the packet content, there is a possibility that other models share common packet structures. If you are interested, feel free to explore on your own. The following conclusions were derived by intercepting and analysing packet content using WireShark with the USBPcap plugin.

---

## 2. 通信协议与数据包格式说明 / Communication Protocol & Packet Format Specification

  硬件通过 USB 总线挂载两个独立的端点，所有控制指令通过 USB HID `SET_REPORT` (Feature Report) 下发。数据包物理有效载荷长度固定为 **524 字节**。在调用 `hidapi` 时，由于 Windows 规范要求，首位需预留 1 字节作为 Report ID (`0x00`)，故发送缓冲区总长度构建为 **525 字节**。

  键盘灯和装饰灯都支持**即时同步**，没有复杂的板载模式锁定或者握手，接收到 `0x0C` 画图报文后会立即刷新物理灯效。

  The hardware mounts two independent endpoints via the USB bus, and all control commands are issued via USB HID `SET_REPORT` (Feature Report). The physical payload length of the data packet is fixed at **524 bytes**. When calling `hidapi`, due to Windows specification requirements, the first byte must be reserved as the Report ID (`0x00`), so the total length of the transmit buffer is constructed as **525 bytes**.

  Both the keyboard lights and decorative lights support **instant synchronisation** without complex on-board mode locking or handshaking. The physical lighting effects refresh immediately upon receiving the `0x0C` drawing packet.

### A. 主键盘区 (VID: `0x1038`, PID: `0x1122`) / Main Keyboard Zone (VID: `0x1038`, PID: `0x1122`)

* `Byte 0`: Report ID (`0x00`)
* `Byte 1`: 指令类型 (`0x0C`) / Command Type (`0x0C`)
* `Byte 2`: 固定 `0x00` / Fixed `0x00`
* `Byte 3`: 按键总量 (`0x66`，即十进制 102 颗键) / Total Keys (`0x66`, i.e. 102 keys in decimal)
* `Byte 4`: 固定 `0x00` / Fixed `0x00`
* `Byte 5 ~ 412`: 102 组 4 字节的 RGB 数据阵列，线性排列结构为：`[KeyID] [R] [G] [B]` / 102 groups of 4-byte RGB data arrays, linearly arranged as: `[KeyID] [R] [G] [B]`
  * `Fn`的`key ID`是`0xf0`，其余按键都遵循国际标准的 **USB HID Keyboard Usage ID** 规范，直接查阅标准的 USB 扫描码表即可对号入座。`电源`键是`0x66`（也在规范中）。
  </br>The `KeyID` for the `Fn` key is `0xF0`, while all other keys strictly adhere to the international standard **USB HID Keyboard Usage ID specification**. They can be mapped directly by referencing a standard USB scan code table. The `Power` key is `0x66` (which is also defined within the specification).
* `Byte 413 ~ 524`: 尾部零填充对齐 / Tail zero-padding alignment

### B. 灯条与 Logo 区 (VID: `0x1038`, PID: `0x1161`) / Lightbar & Logo Zone (VID: `0x1038`, PID: `0x1161`)

* `Byte 0`: Report ID (`0x00`)
* `Byte 1`: 指令类型 (`0x0C`) / Command Type (`0x0C`)
* `Byte 2`: 固定 `0x00` / Fixed `0x00`
* `Byte 3`: 分区数量 (`0x04`) / Number of Zones (`0x04`)
* `Byte 4`: 固定 `0x00` / Fixed `0x00`
* `Byte 5 ~ 20`: 4 组 4 字节的灯区 RGB 数据阵列：`[ZoneID] [R] [G] [B]` / 4 groups of 4-byte lighting zone RGB data arrays: `[ZoneID] [R] [G] [B]`
* `ZoneID 0x00`: 灯条左段 / Lightbar Left Section
* `ZoneID 0x01`: 灯条中段 / Lightbar Middle Section
* `ZoneID 0x02`: 灯条右段 / Lightbar Right Section
* `ZoneID 0x03`: Logo 灯区 / Logo Zone


* `Byte 21 ~ 524`: 尾部零填充对齐 / Tail zero-padding alignment

---

## 3. 示例代码效果 / Example Code Effects

仓库中的 `go.c` 在成功编译运行后，会呈现以下灯光效果：（**不**需要管理员权限）

Once `go.c` in the repository is successfully compiled and run, it will present the following lighting effects: (**No** administrator privileges required)

**主键盘区**：
* `Esc` 键：蓝色 (`#0000FF`)
* `Power` (电源) 键：红色 (`#FF0000`)
* `Fn` 键：绿色 (`#00FF00`)
* 其余所有按键：统一保持中等亮度的浅灰色 (`#888888`)。


**Main Keyboard Zone**:
* `Esc` key: Blue (`#0000FF`)
* `Power` key: Red (`#FF0000`)
* `Fn` key: Green (`#00FF00`)
* All other keys: Uniformly kept at a medium-brightness light grey (`#888888`).


**灯条与 Logo 区**：
* 灯条（左、中、右三段）为红、绿、蓝，背部 Logo：为紫色 (`#FF00FF`)。


**Lightbar & Logo Zone**:
* Lightbar (Left, Middle, Right sections) will be Red, Green, and Blue, and the back Logo: Purple (`#FF00FF`).



---

## 4. 依赖库与 License 声明 / Dependent Libraries & License Declaration

  本项目依赖第三方底层开源库 [hidapi](https://github.com/libusb/hidapi) 来处理跨平台的 USB HID 通信。

  This project relies on the third-party low-level open-source library [hidapi](https://github.com/libusb/hidapi) to handle cross-platform USB HID communication.

### 关于 License 的合规性核对： / License Compliance Check:

1. **hidapi 的授权**：`hidapi` 采用的是极其宽松的 **GPLv3 / BSD-3-Clause / MIT** 多重授权（由使用者任选其一）。
1. **hidapi Licensing**: `hidapi` uses an extremely permissive **GPLv3 / BSD-3-Clause / MIT** multi-licensing scheme (chosen at the user's discretion).
2. **本项目的授权**：本项目采用 **Apache License 2.0** 协议发布。
2. **Project Licensing**: This project is released under the **Apache License 2.0**.
3. **合规性结论**：完全合规，未违反任何依赖库的 License。Apache 2.0 允许自由修改和分发，商业友好，且包含了严谨的互不侵犯专利条款。你可以在遵守 Apache 2.0 协议的前提下，自由地将本项目的代码拿去修改、集成或二次开发。我的目的只是为了帮大家节省逆向试错的时间。
3. **Compliance Conclusion**: Fully compliant, violating no licenses of any dependent libraries. Apache 2.0 allows free modification and distribution, is business-friendly, and includes rigorous patent non-infringement clauses. You are free to modify, integrate, or secondary-develop the code of this project provided you comply with the Apache 2.0 license. My sole purpose is to save everyone time spent on reverse-engineering trial and error.

---

## 5. 未来计划 (Todo List) / Future Plans (Todo List)

* [x] 开发一个带有 GUI 交互界面的简单配置程序，支持可视化点击按键并自定义颜色。
* [x] Develop a simple configuration program with a GUI interactive interface, supporting visual key clicks and custom color definitions.
* [ ] 联动系统底层 API，使键盘按键颜色能够根据 CPU 温度、内存占用等系统信息进行动态映射显示。
* [ ] Integrate with low-level system APIs to allow dynamic mapping and display of keyboard colors based on system information such as CPU temperature and memory usage.

---

## FAQ (常见问题解答) / FAQ (Frequently Asked Questions)

* **Q：为什么搞这个项目？**
  * **A**：我只想把默认的那个亮瞎眼的彩虹跑马灯关掉或者调成舒服的单色，结果官方非得逼我下载一个在后台常驻四个进程、塞满了一大堆我八辈子用不到的功能的臃肿软件，感觉极度不爽，于是决定自己动手把它给逆向替换掉。写个小而美的软件不好吗？整不了微信我还整不了你？[doge]
* **Q: Why did you create this project?**
  * **A**: I just wanted to turn off the default blinding rainbow wave effect or adjust it to a comfortable solid color. Instead, the official choice forces me to download a bloated software that keeps four processes running constantly in the background, packed with a ton of features I would never use in a lifetime. Feeling extremely annoyed, I decided to reverse-engineer and replace it myself.
* **Q：退出程序或者电脑重启之后，灯光效果会保留吗？**
  * **A**：静态效果会保留。写入后芯片会锁存当前的 RGB 状态。动态效果当然就停啦。
* **Q: Will the lighting effects persist after exiting the program or restarting the computer?**
  * **A**: Static effects will persist. Once written, the chip latches the current RGB state. Dynamic effects will, of course, stop.
* **Q：后续准备做点酷炫的特效吗（比如彩虹呼吸、音乐律动、粒子效果）？**
  * **A**：不会。键盘是用来打字输入和生产力的，不是用来当显示屏蹦迪的。如果你极度喜欢那些酷炫的杀马特特效，请出门左转下载官方的 SteelSeries GG。
* **Q: Are there plans to add flashy effects later (like rainbow breathing, music visualizer, or particle effects)?**
  * **A**: No. Keyboards are meant for typing and productivity, not for acting as a disco display. If you absolutely love those flashy, gaudy effects, please close the door outsede, turn left and download the official SteelSeries GG.
* **Q：会对别的微星笔记本机型做适配吗？**
  * **A**：如果你愿意直接给我提供、或者借我一台对应的机器用于实际测试，那我就做。理论上来说把设备PID改了就行了，你自己试试？出错的话键盘啥的*可能*会全红，所以别拿红色测试。
* **Q: Will you adapt this for other MSI laptop models?**
  * **A**: If you are willing to provide or lend me a corresponding machine for actual testing, then I will. Theoretically, just changing the device PID should work; why not try it yourself? If an error occurs, the keyboard and other parts *might* turn entirely red, so don't use red for testing.
* **Q：我自己试会搞废我的电脑吗？**
  * **A**：反正在编程过程中我也没少犯错，但我的电脑还活着。
* **Q: Will trying it myself brick my computer?**
  * **A**: Well, I made plenty of mistakes during development, but my computer is still alive.
* **Q：代码里有的地方看不懂，我该怎么找对应的键位定义/逻辑/我的设备的PID？**
  * **A**：直接问谷歌或者把代码直接扔给 Gemini，人家看一眼数据结构秒懂，比我解释得清楚。
* **Q: I don't understand some parts of the code. How can I find the corresponding key definitions / logic / my device's PID?**
  * **A**: Ask Google directly or just throw the code to Gemini. It understands the data structure in a split second and explains it much better than I can.
* **Q：程序运行出 bug 了！/ 键盘没亮！**
  * **A**：不知道啊，反正它在我的泰坦18 Pro上跑得挺好使的。
* **Q: The program ran into a bug! / The keyboard didn't light up!**
  * **A**: No idea, but it runs perfectly fine on my Titan 18 Pro anyway.
* **Q：你能……？**
  * **A**：看心情。
* **Q: Can you...?**
  * **A**: Depends on my mood.

## 感谢声明 / Acknowledgements

  特别感谢 Google Gemini。这个项目的大部分数据包分析工作都是 Gemini 做的。虽然中途我也花了很多时间跟 Gemini 反复 battle 和纠错。底层的核心逻辑和主体 C 代码基本都是 Gemini 写的。爱你 Gemini。

  Special thanks to Google Gemini. Most of the packet analysis for this project was done by Gemini, even though I spent a lot of time battling and bug-fixing with Gemini along the way. The underlying core logic and main C code were basically written by Gemini. Love you, Gemini.
