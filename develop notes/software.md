# Software
Editor: Lan HUANG Dec 2024

## MQTT
### Topics
Message Types:
1. Switch
2. Command
3. Response


`/GatewayID/ControllerID/Node/Pro`

#### 网关注册/注销

- 添加控制器: {gateway_id}/controller/add
  - payload: {controller_id}
- 删除控制器: {gateway_id}/controller/remove
  - payload: {controller_id}
- 控制器状态: {gateway_id}/{controller_id}/status
  - payload: {status}

#### 开关绑定/解绑

- 进入绑定模式：{gateway_id}/{controller_id}/{light_id}/pairing/add
- 删除所有开关：{gateway_id}/{controller_id}/{light_id}/pairing/remove

- 预留payload给开关的ID，暂时不用
- 预留light_id给多路开关

#### 灯控制/状态
- 开关控制：{gateway_id}/{controller_id}/{light_id}/power/set
  - payload: 01/00
- 开关状态：{gateway_id}/{controller_id}/{light_id}/power
  - payload: 01/00

#### 控制器重置

- 控制器重置：{gateway_id}/{controller_id}/reset

#### 命令执行结果
- 控制器结果响应: {gateway_id}/{controller_id}/response




---
#### Add/Remove Node
Request
- Topic: `/easylight/add` or `/easylight/remove`
- Payload: `36F98D` (Node ID)

---
#### Power Management
Request
- Topic: `/easylight/36F98D/light1/power/set`
- Payload: `01` or `00` (on or off)

Response
- Topic: `/easylight/36F98D/light1/power`
- Payload: `01` or `00` (on or off)

#### Binding
Request
- Topic: `/easylight/36F98D/light1/switch/bind`
- Payload: `01` or `02` (bind or unbind)

#### Reset
Request
- Topic: `/Easylight/36F98D/Reset


