1. What was the overall delivery rate over the entire interval? In other words: how many echo replies were received, divided by how many echo requests were sent? (Note: ping on GNU/Linux doesn’t print any message about echo replies that are not received. You’ll have to identify missing replies by looking for missing sequence numbers.) <br/>
只丢包8个，丢包率8/42015=0.000190408
2. What was the longest consecutive string of successful pings (all replied-to in a row)?<br/>
最长连续成功ping的次数是22814次
3. What was the longest burst of losses (all not replied-to in a row)?<br/>
只有1次
4. How independent or correlated is the event of “packet loss” over time? In other words: 
 * Given that echo request #N received a reply, what is the probability that echo request #(N+1) was also successfully replied-to?<br/>
 $\frac{42015-8-8}{42015-8}\approx 0.999809556$
 * Given that echo request #N did not receive a reply, what is the probability that echo request #(N+1) was successfully replied-to? <br/>
 1
 * How do these figures (the conditional delivery rates) compare with the overall “unconditional” packet delivery rate in the first question? How independent or bursty were the losses?<br/>
 二者之差大于0.000190408，说明丢包是bursty的

5. What was the minimum RTT seen over the entire interval? (This is probably a 
reasonable approximation of the true MinRTT...) <br/>
197ms
6. What was the maximum RTT seen over the entire interval? <br/>
848ms
7. Make a graph of the RTT as a function of time. Label the x-axis with the actual time of day (covering the 2+-hour period), and the y-axis should be the number of milliseconds of RTT. <br/>
![RTT as a function of time](../check4-1.png)
8. Make a histogram or Cumulative Distribution Function of the distribution of RTTs 
observed. What rough shape is the distribution? <br/>
![RTT distribution](../check4-2.png)
9.  make a graph of the correlation between “RTT of ping #N” and “RTT of ping #N+1”. 
The x-axis should be the number of milliseconds from the first RTT, and the y-axis should be the number of milliseconds from the second RTT. How correlated is the RTT over time? <br/>
![](../check4-4.png)<br/>
RTT之间一般是比较接近的，从图上可以看到大部分在x=y的直线上
10.    What are your conclusions from the data? Did the network path behave the way you 
were expecting? What (if anything) surprised you from looking at the graphs and summary statistics? <br/>
网络路径的表现和预期的差不多，但是丢包率比较低，RTT的分布比较集中，RTT之间的相关性比较高

### 补充
我的数据中有许多的DUP（650个），比丢包还多，但是这个报告没有让我分析DUP的情况，我认为这是不合理的。这里放一张DUP随时间的累积图：
![DUP随时间的累积图](../check4-3.png)
