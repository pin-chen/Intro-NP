# Lab 7

## Challenge #1 - Play with the Oracles

Server 提供兩個 clients 讓我們進行操作，但程式當中產生 secret 的方式會蓋掉其他 client 的已產生的。因此先讓兩個 client 都生成 secret ，再讓其中一個把答案 show 出來，即為另一個 client 的答案。

## Challenge #2 - Play with the Oracles (patch1)

與前一個 server 一樣但有重要記憶體位置，因此兩個 client 的 secret 會不一樣，但 seed 的不小於 0 時，不更新，因此使兩個 client 都生成 secret ，會使使用相同 seed 的狀況，先將其中一個把 seed show 出來，即可以自己使用該 seed 產生第二個 client 的答案。

## Challenge #3 - Web Crawler

目標在 localhost/10000 ，但該 server 不支援對 localhost 連線。可以進行連線的狀況會嘗試兩次，且在該 function 中的 ip 會因為再被 call 一次而蓋掉。可以先 request 一個可以允許但不存在的位置，再 request localhost/10000 ，在前者 request 第一次嘗試 time out 後，第二次嘗試時 ip 、 port 已經被改成 127.0.0.1/10000 ，即可取得答案。