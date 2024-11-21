## 前言
本次实验实际需要思考的地方不多，主要是各种规范。
## 实现的方法
#### 总体介绍
本次要求实现的函数有3个，分别为send_datagram、recv_frame、tick。除此之外，我还自己写了辅助函数get_ethernet_address和process_on_learnt。

我自己新声明的变量有以下3个：
```
std::vector<std::pair<EthernetFrame, Address>> frames_to_send{};
std::map<uint32_t, std::pair<EthernetAddress, int32_t>> ethernet_adr_table{};
std::map<uint32_t, std::pair<EthernetAddress, int32_t>> query_adr_table{};
```
第一个变量用于存储等待发送的包，第二个变量用于存储已经学习到的Ethernet地址，第三个变量用于存储正在查询的Ethernet地址，防止5秒内重复查询。

#### `send_datagram`
这个函数还算比较好写。分两种情况，如果已经知道了对方的Ethernet地址，则直接发送即可。

如何判断知不知道对方的Ethernet地址呢？我采用的方法是直接判断map返回的pair的第二个元素是否大于0。如果大于0，则说明已经知道了对方的Ethernet地址。如果小于等于0，有两种情况：
1. 查询过，但是已经expire了
2. 没有查询过，C++会给map不存在的key返回一个默认值，这个默认值对int来说是0。
这两种情况都是需要等待查询的。此时还需要判断是否已经在查询中，如果在查询中的5秒内，就不需要再次发送查询。

查询时，首先需要新建一个 `EthernetFrame arp_frame` 和 `ARPMessage arp`。这里我遇到的最大问题就是如何将 `arp` 作为 `arp_frame` 的数据部分。我翻看了各个类都没有看到功能，在群里询问了助教才知道相关的提示是写在了PPT里面（而我一直是在看手册），可以直接调用 `serialize(arp)` 来得到 `arp` 的序列化结果。这样就可以将 `arp` 作为 `arp_frame` 的数据部分了。此外还需要将原来需要发送的frame加入到等待队列中，等得到了对方的Ethernet地址后再发送。

#### `recv_frame`
我一开始的写法是只有接收到回复我的ARP才将它的Ethernet地址信息加入到 `ethernet_adr_table` 中。但是这样会导致一些问题，比如别人进行广播，其实我也能从这个广播包中得到对方的Ethernet地址。因此我将这个判断条件改为只要收到ARP包，就将其Ethernet地址加入到 `ethernet_adr_table` 中，这样就可以保证不会漏掉任何一个对方的Ethernet地址（这个是在learn from ARP request测试点中发现的）。当加入时，还要处理 `send_datagram` 中的等待队列，将等待队列中的包发送出去。这里两个功能我是一起用一个函数 `process_on_learnt` 来实现的。
另外，如果接收到的是IPV4包，那么直接push到上层即可。

#### `tick`
这个函数比较简单，只需要遍历 `ethernet_adr_table` 中的所有记录，将其时间减少即可。

此外，query_adr_table中的记录也要相应减少，这样只有记录的时间小于等于0时才会重新发送查询APR。

#### `get_ethernet_address`
这个只是我定义的测试辅助函数，用于将ethernet地址变成包含冒号的字符串。

#### `process_on_learnt`
首先将对应的 `sender_ip_address` 和 `sender_ethernet_address` 加入到 `ethernet_adr_table` 中，时间设置为要求的30秒。然后遍历 `frames_to_send` 中的所有包，将其中地址匹配的包发送出去。

## 运行结果
由于已经通过了所有测试，这里直接列出运行结果（可能在下一页）：

![](../check5-1.png)