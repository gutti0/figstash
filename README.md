# フィギュア外箱寸法計測 PoC

フィギュア外箱の寸法を測定・管理し、収納場所の検討につなげるプロジェクトです。現在のファームウェアは M5Stack ATOM Lite、PaHub、ToF4M 3台で X / Y / Z の距離を読み取り、PCへ JSON Lines 形式で出力します。距離の単位はミリメートルです。

## 使用ハードウェア

- M5Stack ATOM Lite
- M5Stack Unit PaHub × 1（初期 I2C アドレス `0x70`）
- M5Stack Unit ToF4M（VL53L1X）× 3
- Grove ケーブル × 4
- ATOM Lite と PC を接続する USB-C ケーブル

## 接続

ATOM Lite の Grove 端子に PaHub の入力端子を接続します。各 ToF4M は PaHub の次のポートにつないでください。センサーの向きが X / Y / Z の対応になります。

| 距離 | PaHub ポート |
| --- | --- |
| X | 0 |
| Y | 1 |
| Z | 2 |

ATOM Lite 側は GPIO26 が SDA、GPIO32 が SCL です。PaHub の I2C アドレスを変更した場合は、[src/main.cpp](src/main.cpp) の `PAHUB_ADDRESS` も合わせて変更してください。

## ビルドと書き込み

PlatformIO Core または PlatformIO IDE でこのフォルダーを開き、次を実行します。

```sh
pio run
pio run --target upload
```

初回は ESP32 Arduino 環境と `pololu/VL53L1X` ライブラリが取得されます。ポートを明示する場合は、Windows のデバイス マネージャーで「ポート (COM と LPT)」を開き、ATOM Lite に割り当てられた `COMx` を確認してから実行します。

```sh
pio run --target upload --upload-port COMx
```

## 動作確認

シリアルモニタを 115200 bps で開きます。

```sh
pio device monitor --port COMx --baud 115200
```

約 100 ms ごとに JSON 1行が出力されます。起動時の説明文は出力されないため、各行をそのまま JSON として読み取れます。`errors` は軸ごとの状態で、正常時は `null` です。

```json
{"type":"distance","x":412,"y":537,"z":681,"errors":{"x":null,"y":null,"z":null}}
```

各センサーの前に順に物体を置き、対応する軸の値だけが変わることを確認してください。センサーを外す、初期化に失敗する、測距に失敗するなどの場合は、その軸の距離が `null` になり、`errors` に理由が入ります。

```json
{"type":"distance","x":412,"y":null,"z":681,"errors":{"x":null,"y":"sensor_missing","z":null}}
```

主な理由は `channel_select_failed`（PaHub と通信不可）、`sensor_missing`（ToF4M と通信不可）、`init_failed`、`config_failed`、`measurement_io_failed`、`measurement_timeout`、`range_invalid` です。初回の測定待ちは `waiting_for_measurement` と表示します。通信が途切れた軸は約 2 秒ごとに再初期化を試みます。

3台を使った実機確認では、軸の対応と、1台だけ外したときに他の2台が継続することも確認してください。

## 今後

箱寸法への換算やキャリブレーション、商品情報の管理、収納空間への配置計算は後続の開発で扱います。
