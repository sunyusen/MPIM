# MPIM系统整体架构 - 增强版

## 架构概述

MPIM是一个基于C++的分布式即时通讯系统，采用微服务架构设计，支持高并发、高可用的消息传输服务。系统采用分层架构，每层都有明确的职责和边界。

## 分层架构图

### 1. 整体分层架构

```mermaid
graph TB
    subgraph "🌐 客户端层 (Client Layer)"
        direction TB
        subgraph "多端客户端"
            C1[📱 移动客户端<br/>Android/iOS<br/>原生应用]
            C2[💻 桌面客户端<br/>Windows/macOS/Linux<br/>跨平台应用]
            C3[🌐 Web客户端<br/>浏览器/WebSocket<br/>实时通信]
        end
    end
    
    subgraph "⚖️ 负载均衡层 (Load Balancer Layer)"
        direction LR
        LB[🔄 Nginx负载均衡器<br/>• SSL终端处理<br/>• 会话保持机制<br/>• 健康检查监控<br/>• 故障自动转移<br/>• 请求分发策略]
    end
    
    subgraph "🚪 接入层 (Gateway Layer)"
        direction TB
        subgraph "网关服务集群"
            G1[🚪 网关服务-1<br/>gateway-001:6000<br/>• Muduo网络库<br/>• 连接池管理<br/>• 协议解析<br/>• 用户认证<br/>• 会话管理]
            G2[🚪 网关服务-2<br/>gateway-002:6000<br/>• Muduo网络库<br/>• 连接池管理<br/>• 协议解析<br/>• 用户认证<br/>• 会话管理]
            G3[🚪 网关服务-N<br/>gateway-N:6000<br/>• Muduo网络库<br/>• 连接池管理<br/>• 协议解析<br/>• 用户认证<br/>• 会话管理]
        end
    end
    
    subgraph "🔧 服务层 (Service Layer)"
        direction TB
        subgraph "微服务集群"
            U[👤 用户服务<br/>im-user:6000<br/>• 用户注册/登录<br/>• 好友关系管理<br/>• 用户信息维护<br/>• 认证授权<br/>• 密码加密]
            P[📍 在线状态服务<br/>im-presence:6000<br/>• 用户路由管理<br/>• 在线状态维护<br/>• 连接映射<br/>• 状态同步<br/>• 故障检测]
            M[💬 消息服务<br/>im-message:6000<br/>• 消息存储转发<br/>• 离线消息处理<br/>• 消息持久化<br/>• 消息路由<br/>• 消息去重]
            G[👥 群组服务<br/>im-group:6000<br/>• 群聊管理<br/>• 成员管理<br/>• 权限控制<br/>• 群组信息<br/>• 群组状态]
        end
    end
    
    subgraph "🔗 中间件层 (Middleware Layer)"
        direction TB
        subgraph "RPC通信框架"
            RPC[🔗 自研RPC框架<br/>• protobuf序列化<br/>• 连接池管理<br/>• 负载均衡<br/>• 服务发现<br/>• 故障转移<br/>• 超时重试]
        end
        
        subgraph "服务治理中心"
            ZK[🐘 ZooKeeper集群<br/>• 服务注册发现<br/>• 配置管理<br/>• 分布式协调<br/>• Leader选举<br/>• 数据一致性<br/>• 故障检测]
        end
        
        subgraph "消息中间件"
            MQ[📨 Redis消息队列<br/>• Pub/Sub实时推送<br/>• Stream可靠消息<br/>• 在线消息路由<br/>• 消息持久化<br/>• 消费组管理]
        end
        
        subgraph "监控日志系统"
            LOG[📊 异步日志系统<br/>• 多线程异步写入<br/>• 分级日志管理<br/>• 文件轮转<br/>• 性能监控<br/>• 错误追踪]
        end
    end
    
    subgraph "🗄️ 缓存层 (Cache Layer)"
        direction TB
        subgraph "Redis缓存集群"
            CACHE1[🗄️ Redis缓存-1<br/>• 用户信息缓存<br/>• 好友关系缓存<br/>• 会话状态缓存<br/>• 配置信息缓存<br/>• 热点数据缓存]
            CACHE2[🗄️ Redis缓存-2<br/>• 群组数据缓存<br/>• 在线状态缓存<br/>• 消息缓存<br/>• 计数器缓存<br/>• 分布式锁]
        end
    end
    
    subgraph "🗃️ 数据层 (Data Layer)"
        direction TB
        subgraph "数据库集群"
            MYSQL1[(🗃️ MySQL主库<br/>• 写操作处理<br/>• 事务管理<br/>• 数据一致性<br/>• 用户数据<br/>• 消息历史<br/>• 群组信息)]
            MYSQL2[(🗃️ MySQL从库<br/>• 读操作处理<br/>• 数据备份<br/>• 查询优化<br/>• 负载分担<br/>• 高可用<br/>• 故障恢复)]
        end
    end
    
    %% 连接关系 - 客户端到负载均衡
    C1 -.->|HTTPS/WSS<br/>加密传输| LB
    C2 -.->|TCP/WebSocket<br/>长连接| LB
    C3 -.->|WebSocket<br/>实时通信| LB
    
    %% 负载均衡到网关
    LB -->|负载均衡<br/>会话保持| G1
    LB -->|负载均衡<br/>会话保持| G2
    LB -->|负载均衡<br/>会话保持| G3
    
    %% 网关到RPC
    G1 -->|RPC调用<br/>服务发现| RPC
    G2 -->|RPC调用<br/>服务发现| RPC
    G3 -->|RPC调用<br/>服务发现| RPC
    
    %% RPC到服务
    RPC -->|服务发现<br/>负载均衡| U
    RPC -->|服务发现<br/>负载均衡| P
    RPC -->|服务发现<br/>负载均衡| M
    RPC -->|服务发现<br/>负载均衡| G
    
    %% 服务到ZooKeeper
    U -.->|服务注册<br/>配置获取| ZK
    P -.->|服务注册<br/>配置获取| ZK
    M -.->|服务注册<br/>配置获取| ZK
    G -.->|服务注册<br/>配置获取| ZK
    
    %% 服务到缓存
    U -->|缓存读写<br/>数据预热| CACHE1
    P -->|状态缓存<br/>路由缓存| CACHE1
    M -->|消息缓存<br/>热点数据| CACHE2
    G -->|群组缓存<br/>成员缓存| CACHE2
    
    %% 服务到数据库
    U -->|数据持久化<br/>事务处理| MYSQL1
    M -->|消息存储<br/>历史记录| MYSQL1
    G -->|群组数据<br/>关系数据| MYSQL1
    
    %% 数据库主从同步
    MYSQL1 -.->|主从同步<br/>数据复制| MYSQL2
    
    %% 消息队列连接
    P -->|消息投递<br/>状态通知| MQ
    G1 -->|订阅消息<br/>实时推送| MQ
    G2 -->|订阅消息<br/>实时推送| MQ
    G3 -->|订阅消息<br/>实时推送| MQ
    
    %% 日志连接
    U -.->|日志输出<br/>性能监控| LOG
    P -.->|日志输出<br/>性能监控| LOG
    M -.->|日志输出<br/>性能监控| LOG
    G -.->|日志输出<br/>性能监控| LOG
    G1 -.->|日志输出<br/>性能监控| LOG
    G2 -.->|日志输出<br/>性能监控| LOG
    G3 -.->|日志输出<br/>性能监控| LOG
    
    %% 样式定义
    classDef clientLayer fill:#e3f2fd,stroke:#0277bd,stroke-width:3px,color:#000
    classDef loadBalancerLayer fill:#f3e5f5,stroke:#7b1fa2,stroke-width:3px,color:#000
    classDef gatewayLayer fill:#e8f5e8,stroke:#388e3c,stroke-width:3px,color:#000
    classDef serviceLayer fill:#fff3e0,stroke:#f57c00,stroke-width:3px,color:#000
    classDef middlewareLayer fill:#fce4ec,stroke:#c2185b,stroke-width:3px,color:#000
    classDef cacheLayer fill:#f1f8e9,stroke:#689f38,stroke-width:3px,color:#000
    classDef dataLayer fill:#e0f2f1,stroke:#00695c,stroke-width:3px,color:#000
    
    class C1,C2,C3 clientLayer
    class LB loadBalancerLayer
    class G1,G2,G3 gatewayLayer
    class U,P,M,G serviceLayer
    class RPC,ZK,MQ,LOG middlewareLayer
    class CACHE1,CACHE2 cacheLayer
    class MYSQL1,MYSQL2 dataLayer
```

### 2. 数据流向时序图

```mermaid
sequenceDiagram
    participant C as 📱 客户端
    participant LB as ⚖️ 负载均衡器
    participant G as 🚪 网关服务
    participant RPC as 🔗 RPC框架
    participant S as 🔧 微服务
    participant ZK as 🐘 ZooKeeper
    participant CACHE as 🗄️ Redis缓存
    participant DB as 🗃️ MySQL数据库
    participant MQ as 📨 消息队列
    
    Note over C,MQ: 🔐 用户登录流程
    C->>+LB: 1. 发送登录请求<br/>(用户名/密码)
    LB->>+G: 2. 负载均衡转发<br/>(SSL终端处理)
    G->>+RPC: 3. RPC调用用户服务<br/>(服务发现)
    RPC->>+ZK: 4. 查询服务地址<br/>(服务注册表)
    ZK-->>-RPC: 5. 返回服务地址<br/>(im-user:6000)
    RPC->>+S: 6. 调用用户服务<br/>(Login方法)
    S->>+CACHE: 7. 查询用户缓存<br/>(用户信息)
    CACHE-->>-S: 8. 返回缓存数据<br/>(命中/未命中)
    S->>+DB: 9. 验证用户信息<br/>(密码校验)
    DB-->>-S: 10. 返回用户数据<br/>(用户信息)
    S->>CACHE: 11. 更新缓存<br/>(用户信息)
    S-->>-RPC: 12. 返回登录结果<br/>(成功/失败)
    RPC-->>-G: 13. 返回响应<br/>(登录状态)
    G-->>-LB: 14. 返回响应<br/>(会话信息)
    LB-->>-C: 15. 返回登录成功<br/>(JWT Token)
    
    Note over C,MQ: 💬 消息发送流程
    C->>+LB: 1. 发送消息<br/>(接收者/内容)
    LB->>+G: 2. 转发到网关<br/>(会话验证)
    G->>+RPC: 3. RPC调用消息服务<br/>(SendMessage)
    RPC->>+S: 4. 调用消息服务<br/>(消息处理)
    S->>+DB: 5. 存储消息<br/>(消息持久化)
    DB-->>-S: 6. 存储确认<br/>(消息ID)
    S->>+MQ: 7. 发布到消息队列<br/>(在线推送)
    MQ->>+G: 8. 推送给在线用户<br/>(实时消息)
    G-->>-C: 9. 实时消息推送<br/>(消息内容)
    S-->>-RPC: 10. 返回发送结果<br/>(成功确认)
    RPC-->>-G: 11. 返回响应<br/>(发送状态)
    G-->>-LB: 12. 返回响应<br/>(处理完成)
    LB-->>-C: 13. 返回发送成功<br/>(消息确认)
```

### 3. 组件交互关系图

```mermaid
graph LR
    subgraph "🔗 核心交互关系"
        direction TB
        
        subgraph "📡 网络通信层"
            direction LR
            NET1[客户端连接<br/>WebSocket/TCP]
            NET2[服务间通信<br/>RPC调用]
            NET3[数据存储<br/>数据库连接]
        end
        
        subgraph "🔄 数据流转层"
            direction LR
            FLOW1[请求处理<br/>负载均衡]
            FLOW2[服务调用<br/>RPC框架]
            FLOW3[数据访问<br/>缓存+数据库]
        end
        
        subgraph "⚡ 性能优化层"
            direction LR
            PERF1[连接池<br/>复用连接]
            PERF2[缓存层<br/>减少DB访问]
            PERF3[异步处理<br/>提高并发]
        end
        
        subgraph "🛡️ 可靠性保障层"
            direction LR
            REL1[服务发现<br/>故障转移]
            REL2[数据备份<br/>主从同步]
            REL3[监控告警<br/>日志追踪]
        end
    end
    
    %% 连接关系
    NET1 --> FLOW1
    NET2 --> FLOW2
    NET3 --> FLOW3
    
    FLOW1 --> PERF1
    FLOW2 --> PERF2
    FLOW3 --> PERF3
    
    PERF1 --> REL1
    PERF2 --> REL2
    PERF3 --> REL3
    
    %% 样式
    classDef networkLayer fill:#e1f5fe,stroke:#01579b,stroke-width:2px
    classDef flowLayer fill:#f3e5f5,stroke:#4a148c,stroke-width:2px
    classDef perfLayer fill:#e8f5e8,stroke:#1b5e20,stroke-width:2px
    classDef relLayer fill:#fff3e0,stroke:#e65100,stroke-width:2px
    
    class NET1,NET2,NET3 networkLayer
    class FLOW1,FLOW2,FLOW3 flowLayer
    class PERF1,PERF2,PERF3 perfLayer
    class REL1,REL2,REL3 relLayer
```

## 架构特点分析

### 1. 分层设计原则

- **职责单一**: 每层只负责特定的功能
- **松耦合**: 层与层之间通过接口交互
- **高内聚**: 同层组件紧密协作
- **可扩展**: 支持水平扩展和垂直扩展

### 2. 技术选型理由

- **C++**: 高性能、低延迟、内存控制精确
- **Muduo**: 高性能网络库、事件驱动、多线程
- **Redis**: 高性能缓存、消息队列、数据结构丰富
- **MySQL**: 关系型数据库、ACID特性、成熟稳定
- **ZooKeeper**: 分布式协调、服务发现、配置管理
- **mprpc**: [自研RPC框架](mprpc-framework-complete.md) - 基于Protobuf、支持服务发现、连接池管理

### 3. 性能优化策略

- **连接池**: 复用网络连接，减少建立连接开销
- **缓存层**: 减少数据库访问，提高响应速度
- **异步处理**: 提高并发处理能力
- **负载均衡**: 分散请求压力，提高系统吞吐量

### 4. 可靠性保障

- **服务发现**: 自动故障检测和转移
- **数据备份**: 主从同步，数据不丢失
- **监控告警**: 实时监控系统状态
- **日志追踪**: 问题定位和性能分析

## 相关文档

### RPC框架详细分析

- [mprpc RPC框架完整分析](mprpc-framework-complete.md) - mprpc RPC框架的完整技术分析
- [mprpc调用序列图](mprpc-call-sequence.md) - RPC调用的详细流程图和序列图

### 其他架构文档

- [用户服务架构](user-service-architecture.md) - 用户服务的详细设计
- [消息服务架构](message-service-architecture.md) - 消息服务的详细设计
- [群组服务架构](group-service-architecture.md) - 群组服务的详细设计

## 总结

MPIM系统采用现代化的微服务架构，通过分层设计实现了高性能、高可用、可扩展的即时通讯服务。系统各层职责明确，技术选型合理，性能优化到位，可靠性保障充分，是一个典型的分布式系统架构实践案例。
