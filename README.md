<b>さらに改造した<a href=https://github.com/quenze-hub/QZ-GamePod>QZ-GamePod</a>があります</b>
<br>
<br>
# BM-GamePod
BM-GamePod Mods &amp; software

このゲームは、ブレッドボード上で動く簡単ゲーム機 BREAD MAKER(Yugi Tech Lab) で動作します。
システム構成、材料、作り方の詳細は [BREAD MAKER ブレッドボードで簡単ゲーム作成！！ | ProtoPedia](https://protopedia.net/prototype/7594)を参照してください。

[青学つくまなラボの石原様](https://github.com/champierre/bread_maker_samples)を改造して音を出すようにしました。<BR>

落ちゲーを縦で遊ぶためにボタン追加の改造と穴あけが必要です。<br>
<img src=tetoris/image2.jpg width='600'><br>
同じ形のボタンを追加<br>
A3とボタンを接続<br>
GNDから分岐してボタンに接続<br>
スピーカーの音量を下げるため手元にあった330Ωをを追加（音が小さいのでもうちょっと抵抗値が低いほうがいいかも）<br>

<b>落ちゲー</b><br>
ゲームの操作方法<br>
左ボタン: 左に移動  ゲーム開始<br>
右ボタン: 右に移動 <br>
同時に左右ボタン　ブロック回転<br>
右上ボタン　ブロック回転<br>
素直に作ると曲のサイズが大きくて、コンパイルは問題ないのにメモリ不足でうまく動きませんでした。そのためソースが汚くなっています。<br>
https://www.youtube.com/watch?v=LWtzs1KUkg4

<b>侵略ゲー</b><br>
ゲームの操作方法<br>
左ボタン: 左に移動  ゲーム開始<br>
右ボタン: 右に移動 <br>
左ボタン+右ボタン:発射 だんだん速くなります<br>
https://youtu.be/SxKb1W25aGQ
<br>
<br>
<br>
さらに改造　<b>電池パックでどこでも遊べる！</b><br>
配線<br>
マイナスをGNDに接続（GNDであればどこでもいい）<br>
プラスをVINに接続（電圧をいい具合にしてくれるらしい。ちなみにこの電池は古いので3つで3.4Vくらいしか出ていませんが動きます。）<br>
右側にプラスの線を通すスペースがないので、スピーカーにつながる線を左側を通すようにしました<br>
<img src=battery/2.JPG width='600'><br>
スイッチ付きのケースにしたのでON/OFFできます。<br>
<img src=battery/3.JPG width='600'><br>
裏側に両面テープで固定<br>
<img src=battery/1.JPG width='300'><br>
PCからのUSBケーブルが不要になり遊びやすい。

