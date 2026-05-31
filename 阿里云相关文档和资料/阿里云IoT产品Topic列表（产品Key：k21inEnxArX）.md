# 阿里云IoT产品Topic列表（产品Key：YOUR_PRODUCT_KEY）

|功能分类|Topic类地址|操作权限|功能描述|
|---|---|---|---|
|OTA 升级|/ota/device/inform/YOUR_PRODUCT_KEY/$\{deviceName\}|发布|设备上报固件升级信息|
||/ota/device/upgrade/YOUR_PRODUCT_KEY/$\{deviceName\}|订阅|固件升级信息下行推送|
||/ota/device/progress/YOUR_PRODUCT_KEY/$\{deviceName\}|发布|设备上报固件升级进度|
||/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/ota/firmware/get|发布|设备主动拉取固件升级信息|
|设备标签|/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/deviceinfo/update|发布|设备上报标签数据|
||/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/deviceinfo/update\_reply|订阅|云端响应标签上报结果|
||/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/deviceinfo/delete|订阅|接收设备删除标签信息指令|
||/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/deviceinfo/delete\_reply|发布|云端响应标签删除结果|
|时钟同步|/ext/ntp/YOUR_PRODUCT_KEY/$\{deviceName\}/request|发布|设备发起NTP时钟同步请求|
||/ext/ntp/YOUR_PRODUCT_KEY/$\{deviceName\}/response|订阅|接收云端NTP时钟同步响应数据|
|设备影子|/shadow/update/YOUR_PRODUCT_KEY/$\{deviceName\}|发布|设备上报、更新设备影子数据|
||/shadow/get/YOUR_PRODUCT_KEY/$\{deviceName\}|订阅|设备接收云端设备影子变更数据|
|配置更新|/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/config/push|订阅|接收云端主动下推的设备配置信息|
||/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/config/get|发布|设备主动查询云端配置信息|
||/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/config/get\_reply|订阅|接收云端返回的设备配置查询结果|
|广播|/broadcast/YOUR_PRODUCT_KEY/$\{identifier\}|订阅|设备订阅云端广播消息，identifier为用户自定义字符串|
|属性上报|/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/event/property/post|发布|设备上报自身状态、参数等属性数据|
||/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/event/property/post\_reply|订阅|接收云端对设备属性上报的响应结果|
|属性设置|/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/service/property/set|订阅|接收云端下发的设备属性设置指令|
|事件上报|/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/event/$\{tsl\.event\.identifier\}/post|发布|设备上报自定义事件、告警、动作等事件数据|
||/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/event/$\{tsl\.event\.identifier\}/post\_reply|订阅|接收云端对设备事件上报的响应结果|
|服务调用|/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/service/$\{tsl\.service\.identifier\}|订阅|接收云端下发的设备服务调用指令|
||/sys/YOUR_PRODUCT_KEY/$\{deviceName\}/thing/service/$\{tsl\.service\.identifier\}\_reply|发布|设备端执行服务后，响应云端调用结果|
|自定义用户Topic|/YOUR_PRODUCT_KEY/$\{deviceName\}/user/get|订阅|设备接收云端自定义查询指令|
||/YOUR_PRODUCT_KEY/$\{deviceName\}/user/update|发布|设备上报自定义更新数据至云端|
||/YOUR_PRODUCT_KEY/$\{deviceName\}/user/update/error|发布|设备上报自定义数据更新异常信息|

> （注：文档部分内容可能由 AI 生成）
DeviceName：YOUR_DEVICE_NAME1，YOUR_DEVICE_NAME2，YOUR_DEVICE_NAME3