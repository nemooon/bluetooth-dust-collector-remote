# Bluetooth連動集じん機リモコン

HiKOKIのBluetooth連動対応集じん機をボタンで操作する、非公式のArduinoスケッチです。

動作を確認したハードウェアはRaytac MDBT50Q-CX-40 USBドングルです。R3640DAとRP80YD(SC)の両機種で接続・運転開始・停止を確認しました。Seeed XIAO nRF52840（非Sense）向けのプロファイルも含みますが、XIAO実機での動作確認は今後の作業です。

**HiKOKI公式の製品・ソフトウェアではありません。** Bluetoothの広告内容と通信形式は実機で観察した動作に基づきます。使用する機器やファームウェアの組み合わせによって動作は変わる可能性があります。

## 動作確認の状況

以下はRaytacドングルでの確認結果です。

| 確認項目 | R3640DA | RP80YD(SC) |
| --- | --- | --- |
| 接続・運転開始・停止 | 確認済み | 確認済み |
| リモコンの電源喪失時の停止 | 確認済み | 確認済み |

RaytacドングルへのUSB DFU書き込み、ボタン、LEDも確認済みです。

## Raytacドングルで試すための準備

### 物理的に必要なもの

- [Raytac MDBT50Q-CX-40開発ドングル（スイッチサイエンス）](https://www.switch-science.com/products/10014)
- Bluetooth連動対応の集じん機（R3640DA、RP80YD(SC)で動作確認済み）
- ドングルをUSB-Cで接続できるmacOS搭載のPC
- ドングルを動作させる電源（PCやモバイルバッテリー）

### インストールするもの

- [Arduino CLI](https://arduino.github.io/arduino-cli/latest/installation/)
- [nRF Util (nrfutil)](https://www.nordicsemi.com/Products/Development-tools/nrf-util) の`nrf5sdk-tools`
- [Seeed nRF52 Boards 1.1.13](https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json)：nRF52840向けのArduinoボードパッケージ
- [Python 3](https://formulae.brew.sh/formula/python)：初回書き込み用のHEXからS140を抽出するために使用

macOSでは、Arduino CLIと`nrfutil`をHomebrewでインストールし、`nrf5sdk-tools`を追加します。

```sh
brew install arduino-cli
brew install --cask nrfutil
brew install python3
nrfutil install nrf5sdk-tools
```

続いて、Seeed nRF52 Boards 1.1.13をArduino CLIでインストールします。これでnRF52840向けのビルド環境とBluefruitライブラリが使えるようになります。

```sh
arduino-cli core update-index \
  --additional-urls https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
arduino-cli core install Seeeduino:nrf52@1.1.13 \
  --additional-urls https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
```

SeeedのパッケージはArduinoコア、Bluefruitライブラリ、初回書き込みに必要なS140（nRF52840用のBluetooth通信ソフトウェア）の入手元として使います。Raytac固有のピン配置やビルド設定は、このリポジトリのボード定義で指定します。SeeedのブートローダーはRaytacドングルに書き込みません。

## Raytacドングルへの書き込み（macOS）

この手順は**MDBT50Q-CX-40開発ドングル**向けです。基板のDFUボタン（P1.06）を操作ボタンとして使い、基板上のD1 LED（P0.06）を表示に使います。USB給電のため電池測定は行いません。

出荷状態のドングルに初めて書き込む場合は、上のインストールを済ませ、リポジトリのルートで次の手順を順番に実行します。最後の書き込みコマンドだけ、Macに表示されたポート名へ置き換えます。

1. このリポジトリのRaytac用ボード定義をArduino CLIのユーザーディレクトリへコピーし、認識されたことを確認します。

   ```sh
   ARDUINO_USER_DIR="$(arduino-cli config get directories.user)"
   mkdir -p "$ARDUINO_USER_DIR/hardware/hikoki"
   cp -R hardware/hikoki/nrf52 "$ARDUINO_USER_DIR/hardware/hikoki/"
   arduino-cli board listall 'Raytac MDBT50Q-CX-40 Development Dongle'
   ```

   `hikoki:nrf52:raytacDevDongle`が表示されれば準備完了です。

2. アプリケーションをビルドします。

   ```sh
   arduino-cli compile --fqbn hikoki:nrf52:raytacDevDongle \
     --build-path build/raytac \
     --build-property recipe.objcopy.zip.pattern=/usr/bin/true \
     --build-property recipe.objcopy.uf2.pattern=/usr/bin/true \
     HiKokiRemote
   ```

3. 出荷状態のドングルへ初めて書き込む場合は、Seeedパッケージ内のHEXからMBRとS140を抽出し、アプリと一緒にDFUパッケージを作ります。Raytacのブートローダー領域は含めません。

   ```sh
   SEEED_HEX="$(arduino-cli config get directories.data)/packages/Seeeduino/hardware/nrf52/1.1.13/bootloader/Seeed_XIAO_nRF52840/Seeed_XIAO_nRF52840_bootloader-0.6.2_s140_7.3.0.hex"
   python3 tools/extract_s140.py "$SEEED_HEX" build/raytac/s140.hex
   nrfutil nrf5sdk-tools pkg generate --hw-version 52 \
     --sd-req 0x00 --sd-id 0x0123 \
     --softdevice build/raytac/s140.hex \
     --application build/raytac/HiKokiRemote.ino.hex \
     --application-version 4 firmware/Raytac_MDBT50Q_CX40_DFU.zip
   ```

4. ドングルのボタンを押したままUSB-Cに挿し、DFUモードにします。`ls /dev/cu.usbmodem*`でポートを確認し、下の`XXXXXXXX`を実際の文字列に置き換えて書き込みます。

   ```sh
   nrfutil nrf5sdk-tools dfu usb-serial \
     -pkg firmware/Raytac_MDBT50Q_CX40_DFU.zip \
     -p /dev/cu.usbmodemXXXXXXXX
   ```

書き込み後はドングルを挿し直して通常起動し、ボタンの長押しでBluetooth広告を開始します。集じん機側の接続操作は使用する機種の取扱説明書に従ってください。

### アプリだけを更新する場合

初回書き込み後にスケッチを変更した場合は、手順2で新しいアプリをビルドし、アプリだけの更新ZIPを作ります。`--sd-req 0x0123`はインストール済みのS140 7.3.0を指定し、`--application-version`には前回より大きい番号を付けます。S140は更新しないので`--softdevice`は付けません。以下はアプリバージョン4から5へ更新する場合の例です。

```sh
nrfutil nrf5sdk-tools pkg generate --hw-version 52 \
  --sd-req 0x0123 \
  --application build/raytac/HiKokiRemote.ino.hex \
  --application-version 5 firmware/Raytac_MDBT50Q_CX40_Update_v5.zip
```

更新時はDFUコマンドの`-pkg`に、この更新パッケージを指定します。書き込み後にシリアルポートが消えるのは、アプリがUSBシリアルを使用しないためです。再書き込み時はボタンを押したまま挿し直してください。

## 操作

以下はRaytacドングルでの操作です。

| 状態 | 短押し | 800 ms以上の長押し |
| --- | --- | --- |
| 待機 | 変化なし | Bluetooth広告を開始 |
| 接続待ち | 変化なし | 広告を停止して待機 |
| 接続中・停止 | 集じんを開始 | 切断して待機 |
| 接続中・運転 | 停止信号を送信 | 停止信号を約350 ms通知してから切断 |

接続待ちは30秒で終了します。LEDは広告中に点滅し、接続中は点灯、待機中は消灯します。Raytacでは起動時に2回短く点滅します。通信が切れたら待機に戻ります。

## Bluetooth通信について

広告名は`HiKOKI BSL36A18BX`です。独自サービス`ffff1948-ffff-ffff-efcd-ab8967452301`を公開し、状態特性`ffff11f1-ffff-ffff-efcd-ab8967452301`に`01`（運転）または`02`（停止）を設定します。接続中は約100 msごとに状態を通知します。詳細は`HiKokiRemote/Protocol.cpp`を参照してください。

## XIAO nRF52840（今後の検証）

[Seeed XIAO nRF52840（非Sense、Seeed公式ストア）](https://jp.seeedstudio.com/Seeed-XIAO-BLE-nRF52840-p-5201.html)向けのプロファイルと配線案を収録しています。接続・運転開始・停止、待機電流、電池電圧、充電は実機で未確認です。書き込み手順は実機検証後に追記します。

生成済みファームウェアは配布していません。Raytac向けの生成物をXIAOに書き込まないでください。

電池残量警告の閾値`kLowBatteryMv`は初期値0（無効）です。使用する電池とXIAOの電圧を測定してから設定してください。USBシリアルで電圧を確認する場合は`HiKokiRemote/Config.h`の`HIKOKI_DEBUG`を1にし、115200 bpsで読み取ります。

### XIAOの配線

| 用途 | 接続 |
| --- | --- |
| ボタン | ノーマルオープンのモーメンタリスイッチをD1とGNDの間に接続。内部プルアップを使用 |
| LED | D2 → 1 kΩ → 青LEDのアノード、カソード → GND。HIGHで点灯 |
| 電池 | 裏面のBAT+ / BAT−に保護回路付き1セルLiPoを接続。実物の極性を必ず確認 |
| 充電 | USB-C。スケッチはP0.13を入力に設定し、低い方の充電電流設定を選択 |

電池電圧は内蔵の分圧回路で読み取ります。使用する電池の許容充電電流・温度、P0.31の電圧、充電電流、待機電流は実機で確認してください。Seeed nRF52 Boards 1.1.13では、スケッチが始まる前にP0.14が一時的にHIGHになります。Seeedは充電中のP0.14=HIGHによるP0.31の過電圧リスクを注意しています。電池とUSBを接続した状態で確認するまで、電池駆動の完成品として扱わないでください。

## リポジトリの構成

| パス | 内容 |
| --- | --- |
| `HiKokiRemote/` | スケッチ、Bluetooth通信、基板プロファイル、設定 |
| `hardware/hikoki/nrf52/` | Raytac用のArduinoボード定義 |
| `tools/extract_s140.py` | SeeedのHEXからMBRとS140を抽出するツール |
| `firmware/` | ローカルで生成するファームウェアの出力先（生成物はGit管理対象外） |

## ライセンス

このリポジトリのソースコードとドキュメントは[MIT License](LICENSE)で公開します。別途インストールするArduinoコア、ライブラリ、ツールには、それぞれのライセンスが適用されます。

## 参考資料

- [Seeed XIAO nRF52840の仕様とArduino設定](https://wiki.seeedstudio.com/XIAO_BLE/)
- [SeeedのBluetooth使用例](https://wiki.seeedstudio.com/XIAO-BLE-Sense-Bluetooth_Usage/)
- [RaytacのUSB DFU手順](https://raytac.blog/2024/07/10/firmware-coding-dfu-onto-mdbt50q-rxuser-manual-of-mdbt50q-cx-nrf52840-usb-c-dongle/)
