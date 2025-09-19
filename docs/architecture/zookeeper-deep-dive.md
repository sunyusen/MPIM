# ZooKeeper 深入解析与在 MPIM 中的实践（占位）

## 1. 核心原理
- ZAB 共识协议（Leader/Follower/Observer）
- 数据模型（ZNode：持久/临时、顺序型）
- 会话与临时节点的关系
- Watch 机制与一次性通知

## 2. 在 MPIM 的应用
- 服务注册：/Service 固定持久节点；/Service/Method 实例临时节点（data=ip:port）
- 服务发现：GetData / GetChildren + 本地缓存 + 可选 Watch
- 故障感知：会话失效临时节点自动删除

## 3. 实践建议
- 命名规范、权限、安全与 ACL
- 批量与幂等、抖动保护
- 连接与超时、重试与指数退避

（后续补充完整内容）
