Digibest系チューナー用BDASpecialプラグイン
  PX-MLT5PE/PX-Q3PE4/PX-W3PE4/PX-Q3U4/PX-W3U4/DTV02-5T-P/DTV02-1T1S-U内蔵カードリーダー用WinSCard.dll付属
  PX-Q3U4/PX-W3U4付属リモコン用TVTestプラグイン付属

【これは何？】
Plex社製/e-Better社製USB接続(Box型/PCI-Ex型)3波チューナー専用のBonDriver_BDA用BDASpecialプラグインです。
BonDriver_BDA.dllと組み合わせて使用します。
また、付属のWinSCard.dllを使用することにより、内臓カードリーダーを任意のアプリで利用することができます。

【動作環境】
Windows XP/Vista/7/8/8.1/10 (x86/x64)

【対応チューナー】
下記のチューナーに対応しています（たぶん）。
  Plex PX-MLT5PE
  Plex PX-Q3PE4
  Plex PX-W3PE4
  Plex PX-Q3U4
  Plex PX-W3U4
  e-Better DTV02-5T-P
  e-Better DTV02-1T1S-U
Plex社製/e-Better社製でもDigibest社OEM製品以外のもの、IT9300系が使用されていないものはこのBDASpecialプラグインの対象外です。

【使い方】
1. BonDriver_BDAの入手
下記URLより、最新バージョンのBonDriver_BDAを入手してください。
https://github.com/radi-sh/BonDriver_BDA/releases
※ BonDriver_BDA改-20190202より前のバージョンでは動作しませんのでご注意ください。

2. 使用するファイル（32ビット用/64ビット用、Windows XP用/Windows Vista以降用、通常版/ランタイム内蔵版）の選択
BonDriver_BDA付属のReadme-BonDriver_BDAを参考に、BonDriver_BDAと同じ種類のものを選択してください。

3. Visual C++ 再頒布可能パッケージのインストール
BonDriver_BDA付属のReadme-BonDriver_BDAを参考に、インストールしてください。

4. BonDriverとiniファイル等の配置
・使用するアプリのBonDriver配置フォルダに、BonDriver_BDA.dllをリネームしたコピーを配置
  通常、ファイル名が"BonDriver_"から始まる必要がありますのでご注意ください。
・用意したdllと同じ名前のiniファイルを配置
  下記のサンプルiniファイルを基に作成してください。
    -BonDriver_PX_MLT5PE.ini   (PX-MLT5PE 地デジ&衛星用)
    -BonDriver_PX_x3U4_T.ini   (PX-Q3PE4/W3PE4/Q3U4/W3U4 地デジ用)
    -BonDriver_PX_x3U4_S.ini   (PX-Q3PE4/W3PE4/Q3U4/W3U4 衛星用)
    -BonDriver_5T.ini          (DTV02-5T-P 地デジ用)
    -BonDriver_1T1S.ini        (DTV02-1T1S-U 地デジ&衛星用)
・IT35.dllファイルを配置
  IT35.dllファイルはリネームせずに1つだけ配置すればOKです。

(配置例)
  BonDriver_PX_x3U4_S0.dll
  BonDriver_PX_x3U4_S0.ini
  BonDriver_PX_x3U4_S1.dll
  BonDriver_PX_x3U4_S1.ini
  BonDriver_PX_x3U4_T0.dll
  BonDriver_PX_x3U4_T0.ini
  BonDriver_PX_x3U4_T1.dll
  BonDriver_PX_x3U4_T1.ini
  IT35.dll

5. WinSCard.dllとiniファイルの配置
内蔵のカードリーダを使用しない場合はこの作業は必要ありません。
・使用するアプリのexeがあるフォルダに、WinSCard.dllとWinSCard.iniを配置
・必要に応じてWinSCard.iniの内容を変更
  付属のiniファイルはPX-W3U4用ですので他の機種で使用する場合はiniファイルの変更が必要です。

6. TVTest用プラグインの配置
TVTestでPX-Q3U4/PX-W3U4付属リモコンを使用する場合以外はこの作業は必要ありません。
・TVTestのPluginsフォルダに、x3U4Remocon.tvtpとx3U4Remocon.iniを配置
・必要に応じてx3U4Remocon.iniの内容を変更
  付属のiniファイルはPX-W3U4用ですのでPX-Q3U4で使用する場合はiniファイルの変更が必要です。
・TVTestを起動し、プラグインの有効化とリモコンボタンの割り当て設定を行う

【サポートとか】
・最新バージョンとソースファイルの配布場所
https://github.com/radi-sh/BDASpecial-PlexPX/releases

・不具合報告等
専用のサポート場所はありません。
5chのDTV板で該当スレを探して書込むとそのうち何か反応があるかもしれません。
作者は多忙を言い訳にあまりスレを見ていない傾向に有りますがご容赦ください。

【免責事項】
BDASpecialプラグインおよびBonDriver_BDAや付属するもの、ドキュメントの記載事項などに起因して発生する損害事項等の責任はすべて使用者に依存し、作者・関係者は一切の責任を負いません。

【謝辞みたいなの】
・このBDASpecialプラグインは「Bon_SPHD_BDA_PATCH_2」を基に改変したものです。
・上記の作者様、その他参考にさせていただいたDTV関係の作者様、ご助言いただいた方、不具合報告・使用レポートをいただいた方、全ての使用していただいた方々に深く感謝いたします。

