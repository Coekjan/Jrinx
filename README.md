# Jrinx

## 什么是 Jrinx？

Jrinx 是一个多分区、多核操作系统，其命名来源于 Arinc 与 Jos，也具有 **J**ust **R**un **I**n jri**NX** 的意义。

## 未来可能需要做的事情

- 使用伙伴系统算法管理 `vmm_setup_kern` 之前使用的内存空间（目前是 `bare_alloc` 线性分配，无法回收）
- 实现 RISC-V64 下的异常处理机制
- 设计 Jrinx OS 的启动参数，可用于运行单元测试、启动特定的分区等
- 设计 Arinc653 系统配置文件的格式，主要涉及分区定义、进程定义
