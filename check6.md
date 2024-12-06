## 前言
本次实验内容太简单了，没什么好写的。
## 实现的方法
#### 总体介绍
本次要求实现的函数有2个，分别为add_route、route。我自己额外添加了2个结构体router_key、router_value，用于存储路由表的key和value。此外还有函数add_map、longest_prefix_match。

结构体router_key包含route_prefix和prefix_length用于router查询（还定义了大小比较函数，用于map的key排序）；结构体router_value包含interface和next_hop（optional）作为返回值。

#### add_route
只是作为一个接口，只包含一个输出语句和调用add_map函数。

#### route
遍历每一个interface_i，如队列非空，则依次取出元素，减少ttl并重新计算checksum，然后调用longest_prefix_match函数，找到最长匹配的路由表项，如果找到了，就在对应的interface(N)上发送数据包，否则丢弃。

#### add_map
就只是直接设置map[key] = value。

#### longest_prefix_match
从i=32开始，逐渐减小到1，查找是否有对应的路由表项，如果有就返回对应的value。如果没有找到，看一下key={0,0}的路由表项是否存在，如果存在就返回对应的value（这一步是因为上一步中如果i=0就会给mask左移32位，这会报错的）。如果还没有找到，则返回optional的空值nullopt。

## 运行结果
由于已经通过了所有测试，这里直接列出运行结果（可能在下一页）：

![](../check6-1.png)