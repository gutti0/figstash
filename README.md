# フィギュア外箱寸法計測 PoC

フィギュア外箱の寸法を測定・管理し、収納場所の検討につなげるプロジェクトです。
このファームウェアは最初の PoC として、M5Stack ATOM Lite に ToF4M を1台だけ接続し、距離をPCへシリアル出力します。

## 将来の構成

計測装置では ATOM Lite と PaHub、ToFセンサー3台を使い、箱の幅・高さ・奥行きを取得する予定です。
アプリ側では商品情報や箱データを管理し、収納空間への配置計算につなげます。

## 使用ハードウェア

- M5Stack ATOM Lite
- M5Stack Unit ToF4M（VL53L1X）× 1
- Groveケーブル
- ATOM Lite とPCを接続するUSB-Cケーブル

## 配線

ToF4M を ATOM Lite の Grove 端子へ直接接続します。

| ATOM Lite Grove | ToF4M |
| --- | --- |
| GND | GND |
| 5V | 5V |
| GPIO26 | SDA |
| GPIO32 | SCL |

## ビルド

PlatformIO Core または PlatformIO IDE を用意し、このフォルダーを PlatformIO プロジェクトとして開きます。
ターミナルでは次を実行します。

```sh
pio run
```

初回は ESP32 Arduino環境と `pololu/VL53L1X` ライブラリが自動で取得されます。

## COMポートの確認（Windows）

1. ATOM LiteをUSB-CでPCへ接続します。
2. Windowsのスタートボタンを右クリックし、「デバイス マネージャー」を開きます。
3. 「ポート (COM と LPT)」を展開します。
4. 接続前後で増えた「USB Serial Port (COMx)」などの項目を確認します。括弧内の `COMx` が指定するポートです。

このPCでは現在、ATOM Liteが `USB Serial Port (COM3)` として認識されています。別のUSBポートやPCでは番号が変わることがあります。

## 書き込み

ATOM Lite をUSB-CでPCへ接続し、次を実行します。

```sh
pio run --target upload
```

ポートを明示する場合は、デバイスマネージャーで確認した番号を指定します。

```sh
pio run --target upload --upload-port COMx
```

`COMx` は確認したポート名に置き換えてください。たとえば `COM3` の場合は `--upload-port COM3` とします。

## シリアルモニタ

```sh
pio device monitor --baud 115200
```

ポートを指定する場合:

```sh
pio device monitor --port COMx --baud 115200
```

こちらも `COMx` を確認したポート名に置き換えてください。

センサーが初期化されると、約100msごとに次の形式で距離をミリメートル単位で表示します。

```text
Distance: 342 mm
```

センサー初期化の失敗、測距エラー、500ms以上測定値が届かない場合は `ERROR:` で始まるメッセージを表示します。

## ファイル

- `platformio.ini` — ATOM Lite、Arduino framework、ライブラリの設定
- `src/main.cpp` — I2C初期化、連続測距、シリアル出力

現在はToF4Mを1台だけ使用する構成です。I2Cピンとセンサー設定を定数にまとめてあり、後で複数センサー構成へ拡張する際に変更箇所を追いやすくしています。
